#include <debase_internals.h>
#include <hacks.h>

static VALUE mDebase;                 /* Ruby Debase Module object */
static VALUE cContext;
static VALUE cDebugThread;

static VALUE verbose = Qfalse;
static VALUE locker = Qnil;
static VALUE contexts;
static VALUE catchpoints;
static VALUE breakpoints;
static VALUE file_filter_enabled = Qfalse;

static VALUE tpLine;
static VALUE tpCall;
static VALUE tpReturn;
static VALUE tpRaise;

static VALUE idAlive;
static VALUE idAtLine;
static VALUE idAtBreakpoint;
static VALUE idAtCatchpoint;
static VALUE idFileFilter;
static VALUE idAccept;

static int started = 0;

static void
print_debug(const char *message, ...)
{
  va_list ap;

  if (verbose == Qfalse) return;
  va_start(ap, message);
  vfprintf(stderr, message, ap);
  va_end(ap);
}

static inline int
check_stop_frame(debug_context_t *context) {
  return context->calced_stack_size == context->stop_frame && context->calced_stack_size >= 0;
}

static VALUE
is_path_accepted(VALUE path) {
  VALUE filter;
  if (file_filter_enabled == Qfalse) return Qtrue;
  filter = rb_funcall(mDebase, idFileFilter, 0);
  return rb_funcall(filter, idAccept, 1, path);
}

static VALUE
Debase_thread_context(VALUE self, VALUE thread)
{
  VALUE context;

  context = rb_hash_aref(contexts, thread);
  if (context == Qnil) {
    context = context_create(thread, cDebugThread);
    rb_hash_aset(contexts, thread, context);
  }
  return context;
}

static VALUE
Debase_current_context(VALUE self)
{
  return Debase_thread_context(self, rb_thread_current());
}

static int
set_recalc_flag(VALUE thread, VALUE context_object, VALUE ignored)
{
  debug_context_t *context;

  Data_Get_Struct(context_object, debug_context_t, context);
  CTX_FL_SET(context, CTX_FL_UPDATE_STACK);

  return ST_CONTINUE;
}

static int
can_disable_trace_points(VALUE thread, VALUE context_object, VALUE result)
{
  debug_context_t *context;

  Data_Get_Struct(context_object, debug_context_t, context);

  if (-1 != context->dest_frame
      || context->stop_line >= 0
      || -1 != context->stop_next
      || context->stop_reason != CTX_STOP_NONE
      || context->thread_pause != 0)
  {
    print_debug("can_disable_tp: %d %d %d %d\n", context->dest_frame, context->stop_line,
                                                 context->stop_next, context->stop_reason);
    *((VALUE *) result) = Qfalse;
    return ST_STOP;
  }
  return ST_CONTINUE;
}

static void
try_disable_trace_points()
{
  VALUE can_disable = Qtrue;

  if (RARRAY_LEN(breakpoints) != 0) return;
  print_debug("disable_tps: no breakpoints\n");
  if (!RHASH_EMPTY_P(catchpoints)) return;
  print_debug("disable_tps: no catchpoints\n");
  if (rb_tracepoint_enabled_p(tpLine) == Qfalse) return;
  print_debug("disable_tps: tps are enabled\n");

  rb_hash_foreach(contexts, can_disable_trace_points, (VALUE)&can_disable);
  if (can_disable == Qfalse) return;
  print_debug("disable_tps: can disable contexts\n");

  rb_tracepoint_disable(tpLine);
  rb_tracepoint_disable(tpCall);
  rb_tracepoint_disable(tpReturn);
  rb_tracepoint_disable(tpRaise);
  rb_hash_foreach(contexts, set_recalc_flag, 0);
}

extern VALUE
enable_trace_points()
{
  print_debug("enable_tps: \n");
  if (rb_tracepoint_enabled_p(tpLine) == Qtrue) return Qtrue;
  print_debug("enable_tps: need to enable\n");

  rb_tracepoint_enable(tpLine);
  rb_tracepoint_enable(tpCall);
  rb_tracepoint_enable(tpReturn);
  rb_tracepoint_enable(tpRaise);

  return Qfalse;
}

static VALUE
Debase_enable_trace_points(VALUE self)
{
  return enable_trace_points();
}

static int
remove_dead_threads(VALUE thread, VALUE context, VALUE ignored)
{
  return (IS_THREAD_ALIVE(thread)) ? ST_CONTINUE : ST_DELETE;
}

static void 
cleanup(debug_context_t *context)
{
  VALUE thread;

  context->stop_reason = CTX_STOP_NONE;

  clear_stack(context);

  /* release a lock */
  locker = Qnil;
  
  /* let the next thread to run */
  thread = remove_from_locked();
  if(thread != Qnil)
    rb_thread_run(thread);

  try_disable_trace_points();
}

static int
check_start_processing(debug_context_t *context, VALUE thread)
{
  /* return if thread is marked as 'ignored'.
    debugger's threads are marked this way
  */
  if(CTX_FL_TEST(context, CTX_FL_IGNORE)) return 0;

  while(1)
  {
    /* halt execution of the current thread if the debugger
       is activated in another
    */
    while(locker != Qnil && locker != thread)
    {
      add_to_locked(thread);
      rb_thread_stop();
    }

    /* stop the current thread if it's marked as suspended */
    if(CTX_FL_TEST(context, CTX_FL_SUSPEND) && locker != thread)
    {
      CTX_FL_SET(context, CTX_FL_WAS_RUNNING);
      rb_thread_stop();
    }
    else break;
  }

  /* return if the current thread is the locker */
  if(locker != Qnil) return 0;

  /* only the current thread can proceed */
  locker = thread;

  /* ignore a skipped section of code */
  if(CTX_FL_TEST(context, CTX_FL_SKIPPED)) {
    cleanup(context);
    return 0;
  }
  return 1;
}

static inline const char*
symbol2str(VALUE symbol)
{
  VALUE id;
  static const char* nil_str= "nil";
  if (symbol == Qnil) {
    return nil_str;
  }
  id = SYM2ID(symbol);
  if (symbol == Qnil) {
    return nil_str;
  }
  return rb_id2name(id);
}

static inline void
print_event(rb_trace_point_t *tp, debug_context_t *context)
{  
  VALUE locations;
  VALUE path;
  VALUE line;
  VALUE event;
  VALUE mid;
  VALUE rb_cl;
  VALUE rb_cl_name;
  const char *defined_class;

  if (verbose == Qtrue) {
    path = rb_tracearg_path(tp);
    line = rb_tracearg_lineno(tp);
    event = rb_tracearg_event(tp);
    mid = rb_tracearg_method_id(tp);
    rb_cl = rb_tracearg_defined_class(tp);
    rb_cl_name = NIL_P(rb_cl) ? rb_cl : rb_mod_name(rb_cl);
    defined_class = NIL_P(rb_cl_name) ? "" : RSTRING_PTR(rb_cl_name);

    fprintf(stderr, "[#%d] %s@%s:%d %s#%s\n",
            context->thnum,
            symbol2str(event),
            path == Qnil ? "" : RSTRING_PTR(path),
            FIX2INT(line),
            defined_class,
            mid == Qnil ? "(top level)" : symbol2str(mid)
    );
    locations = rb_funcall(context->thread, rb_intern("backtrace_locations"), 1, INT2FIX(1));
    fprintf(stderr, "    calced_stack_size=%d, stack_size=%d, real_stack_size=%d\n",
            context->calced_stack_size, context->stack_size,
            locations != Qnil ? RARRAY_LENINT(locations) : 0);
  }
}

static VALUE
fill_stack_and_invoke(const rb_debug_inspector_t *inspector, void *data)
{
  debug_context_t *context;
  VALUE context_object;

  context_object = *(VALUE *)data;
  Data_Get_Struct(context_object, debug_context_t, context);
  fill_stack(context, inspector);

  return Qnil;
}

static VALUE
start_inspector(VALUE data)
{
  return rb_debug_inspector_open(fill_stack_and_invoke, &data);
}

static VALUE
stop_inspector(VALUE data)
{
  return Qnil;
}

static int
remove_pause_flag(VALUE thread, VALUE context_object, VALUE ignored)
{
  debug_context_t *context;

  Data_Get_Struct(context_object, debug_context_t, context);
  context->thread_pause = 0;

  return ST_CONTINUE;
}

static void 
call_at_line(debug_context_t *context, char *file, int line, VALUE context_object)
{
  context->hit_user_code = 1;

  rb_hash_foreach(contexts, remove_pause_flag, 0);
  CTX_FL_UNSET(context, CTX_FL_STEPPED);
  CTX_FL_UNSET(context, CTX_FL_FORCE_MOVE);
  context->last_file = file;
  context->last_line = line;
  rb_funcall(context_object, idAtLine, 2, rb_str_new2(file), INT2FIX(line));
}

int count_stack_size() {
    rb_thread_t *thread = ruby_current_thread;
    rb_control_frame_t *last_cfp = TH_CFP(thread);
    const rb_control_frame_t *start_cfp = (rb_control_frame_t *)RUBY_VM_END_CONTROL_FRAME(TH_INFO(thread));
    const rb_control_frame_t *cfp;

    ptrdiff_t size, i;

    start_cfp =
      RUBY_VM_NEXT_CONTROL_FRAME(
      RUBY_VM_NEXT_CONTROL_FRAME(start_cfp)); /* skip top frames */

    if (start_cfp < last_cfp) {
        size = 0;
    }
    else {
        size = start_cfp - last_cfp + 1;
    }

    int stack_size = 0;
    for (i=0, cfp = start_cfp; i<size; i++, cfp = RUBY_VM_NEXT_CONTROL_FRAME(cfp)) {
        if (cfp->iseq && cfp->pc) {
            stack_size++;
        }
    }

    return stack_size;
}

static void
process_line_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE context_object;
  VALUE breakpoint;
  debug_context_t *context;
  rb_trace_point_t *tp;
  char *file;
  int line;
  int moved;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, rb_thread_current())) return;

  tp = TRACE_POINT;
  path = rb_tracearg_path(tp);

  if (is_path_accepted(path)) {

    lineno = rb_tracearg_lineno(tp);
    file = RSTRING_PTR(path);
    line = FIX2INT(lineno);

    update_stack_size(context);
    print_event(tp, context);

    if(context->init_stack_size == -1) {
        context->stack_size = count_stack_size();
        context->init_stack_size = context->stack_size;
    }

    if (context->thread_pause) {
      context->stop_next = 1;
      context->dest_frame = -1;
      moved = 1;
    }
    else {
      moved = context->last_line != line || context->last_file == NULL ||
              strcmp(context->last_file, file) != 0;
    }

    if (context->dest_frame == -1 || context->calced_stack_size == context->dest_frame)
    {
      if (moved || !CTX_FL_TEST(context, CTX_FL_FORCE_MOVE))
        context->stop_next--;
      if (context->stop_next < 0)
        context->stop_next = -1;
      if (moved || (CTX_FL_TEST(context, CTX_FL_STEPPED) && !CTX_FL_TEST(context, CTX_FL_FORCE_MOVE)))
      {
        context->stop_line--;
        CTX_FL_UNSET(context, CTX_FL_STEPPED);
      }
    }
    else if(context->calced_stack_size < context->dest_frame)
    {
      context->stop_next = 0;
    }
    if(check_stop_frame(context))
    {
      context->stop_next = 0;
      context->stop_frame = -1;
    }

    breakpoint = breakpoint_find(breakpoints, path, lineno, trace_point);
    if (context->stop_next == 0 || context->stop_line == 0 || breakpoint != Qnil) {
      rb_ensure(start_inspector, context_object, stop_inspector, Qnil);
      if(context->stack_size <= context->init_stack_size && context->hit_user_code) {
        context->script_finished = 1;
      }
      if(!context->script_finished) {
        context->stop_reason = CTX_STOP_STEP;
        if (breakpoint != Qnil) {
          context->stop_reason = CTX_STOP_BREAKPOINT;
          rb_funcall(context_object, idAtBreakpoint, 1, breakpoint);
        }
        reset_stepping_stop_points(context);
        call_at_line(context, file, line, context_object);
      }
    }
  }
  cleanup(context);
}

static void
process_return_event(VALUE trace_point, void *data)
{
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, rb_thread_current())) return;

  --context->calced_stack_size;
  update_stack_size(context);
  /* it is important to check stop_frame after stack size updated
     if the order will be changed change Context_stop_frame accordingly.
  */
  if(check_stop_frame(context))
  {
    context->stop_next = 1;
    context->stop_frame = -1;
  }

  print_event(TRACE_POINT, context);
  cleanup(context);
}

static void
process_call_event(VALUE trace_point, void *data)
{
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, rb_thread_current())) return;

  ++context->calced_stack_size;
  update_stack_size(context);
  print_event(TRACE_POINT, context);
  cleanup(context);
}

static void
process_raise_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE context_object;
  VALUE hit_count;
  VALUE exception_name;
  debug_context_t *context;
  rb_trace_point_t *tp;
  char *file;
  int line;
  int c_hit_count;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, rb_thread_current())) return;

  update_stack_size(context);
  tp = TRACE_POINT;
  if (catchpoint_hit_count(catchpoints, rb_tracearg_raised_exception(tp), &exception_name) != Qnil) {
    rb_ensure(start_inspector, context_object, stop_inspector, Qnil);
    path = rb_tracearg_path(tp);

    if (is_path_accepted(path) == Qtrue) {
      lineno = rb_tracearg_lineno(tp);
      file = RSTRING_PTR(path);
      line = FIX2INT(lineno);
      /* On 64-bit systems with gcc and -O2 there seems to be
         an optimization bug in running INT2FIX(FIX2INT...)..)
         So we do this in two steps.
        */
      c_hit_count = FIX2INT(rb_hash_aref(catchpoints, exception_name)) + 1;
      hit_count = INT2FIX(c_hit_count);
      rb_hash_aset(catchpoints, exception_name, hit_count);
      context->stop_reason = CTX_STOP_CATCHPOINT;
      rb_funcall(context_object, idAtCatchpoint, 1, rb_tracearg_raised_exception(tp));
      call_at_line(context, file, line, context_object);
    }
  }

  cleanup(context);
}

static VALUE
Debase_setup_tracepoints(VALUE self)
{
  if (started) return Qnil;
  started = 1;

  contexts = rb_hash_new();
  breakpoints = rb_ary_new();
  catchpoints = rb_hash_new();

  tpLine = rb_tracepoint_new(Qnil, RUBY_EVENT_LINE, process_line_event, NULL);
  rb_global_variable(&tpLine);

  tpReturn = rb_tracepoint_new(Qnil, RUBY_EVENT_RETURN | RUBY_EVENT_B_RETURN | RUBY_EVENT_C_RETURN | RUBY_EVENT_END,
                               process_return_event, NULL);
  rb_global_variable(&tpReturn);

  tpCall = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_B_CALL | RUBY_EVENT_C_CALL | RUBY_EVENT_CLASS,
                             process_call_event, NULL);
  rb_global_variable(&tpCall);

  tpRaise = rb_tracepoint_new(Qnil, RUBY_EVENT_RAISE, process_raise_event, NULL);
  rb_global_variable(&tpRaise);

  return Qnil;
}

static VALUE
Debase_remove_tracepoints(VALUE self)
{
  started = 0;

  if (tpLine != Qnil) rb_tracepoint_disable(tpLine);
  if (tpReturn != Qnil) rb_tracepoint_disable(tpReturn);
  if (tpCall != Qnil) rb_tracepoint_disable(tpCall);
  if (tpRaise != Qnil) rb_tracepoint_disable(tpRaise);

  return Qnil;
}

static VALUE
debase_prepare_context(VALUE self, VALUE file, VALUE stop)
{
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(self);
  Data_Get_Struct(context_object, debug_context_t, context);

  if(RTEST(stop)) {
    context->stop_next = 1;
    Debase_enable_trace_points(self);
  }
  ruby_script(RSTRING_PTR(file));
  return self;
}

static VALUE
Debase_prepare_context(VALUE self)
{
  Debase_current_context(self);

  return self;
}

static VALUE
Debase_debug_load(int argc, VALUE *argv, VALUE self)
{
  VALUE file, stop, increment_start;
  int state;

  if(rb_scan_args(argc, argv, "12", &file, &stop, &increment_start) == 1) 
  {
      stop = Qfalse;
      increment_start = Qtrue;
  }
  Debase_setup_tracepoints(self);
  debase_prepare_context(self, file, stop);
  rb_load_protect(file, 0, &state);
  if (0 != state) 
  {
      VALUE error_info = rb_errinfo();
      rb_set_errinfo(Qnil);
      return error_info;
  }
  return Qnil;
}

static int
values_i(VALUE key, VALUE value, VALUE ary)
{
    rb_ary_push(ary, value);
    return ST_CONTINUE;
}

static VALUE
Debase_contexts(VALUE self)
{
  VALUE ary;

  ary = rb_ary_new();
  /* check that all contexts point to alive threads */
  rb_hash_foreach(contexts, remove_dead_threads, 0);
 
  rb_hash_foreach(contexts, values_i, ary);

  return ary;
}

static VALUE
Debase_breakpoints(VALUE self)
{
  return breakpoints;
}

static VALUE
Debase_catchpoints(VALUE self)
{
  if (catchpoints == Qnil)
    rb_raise(rb_eRuntimeError, "Debugger.start is not called yet.");
  return catchpoints; 
}

static VALUE
Debase_started(VALUE self)
{
  return started ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    Debase.verbose? -> bool
 *
 *  Returns +true+ if verbose output of TracePoint API events is enabled.
 */
static VALUE
Debase_verbose(VALUE self)
{
  return verbose;
}

/*
 *  call-seq:
 *    Debase.verbose = bool
 *
 *  Enable verbose output of every TracePoint API events, useful for debugging debase.
 */
static VALUE
Debase_set_verbose(VALUE self, VALUE value)
{
  verbose = RTEST(value) ? Qtrue : Qfalse;
  return value;
}

/*
 *  call-seq:
 *    Debase.enable_file_filtering(bool)
 *
 *  Enables/disables file filtering.
 */
static VALUE
Debase_enable_file_filtering(VALUE self, VALUE value)
{
  file_filter_enabled = RTEST(value) ? Qtrue : Qfalse;
  return value;
}

#if RUBY_API_VERSION_CODE >= 20500 && RUBY_API_VERSION_CODE < 20600 && !(RUBY_RELEASE_YEAR == 2017 && RUBY_RELEASE_MONTH == 10 && RUBY_RELEASE_DAY == 10)
    static const rb_iseq_t *
    my_iseqw_check(VALUE iseqw)
    {
        rb_iseq_t *iseq = DATA_PTR(iseqw);

        if (!iseq->body) {
            return NULL;
        }

        return iseq;
    }

    static VALUE
    Debase_set_trace_flag_to_iseq(VALUE self, VALUE rb_iseq) {
        if (!SPECIAL_CONST_P(rb_iseq) && RBASIC_CLASS(rb_iseq) == rb_cISeq) {
            const rb_iseq_t *iseq = my_iseqw_check(rb_iseq);

            if(iseq) {
                rb_iseq_trace_set(iseq, RUBY_EVENT_TRACEPOINT_ALL);
            }
        }
    }

    static VALUE
    Debase_unset_trace_flags(VALUE self, VALUE rb_iseq) {
        if (!SPECIAL_CONST_P(rb_iseq) && RBASIC_CLASS(rb_iseq) == rb_cISeq) {
            const rb_iseq_t *iseq = my_iseqw_check(rb_iseq);

            if(iseq) {
                rb_iseq_trace_set(iseq, RUBY_EVENT_NONE);
            }
        }
    }
#else
      static VALUE
      Debase_set_trace_flag_to_iseq(VALUE self, VALUE rb_iseq) {
        return Qnil;
      }

      static VALUE
      Debase_unset_trace_flags(VALUE self, VALUE rb_iseq) {
        return Qnil;
      }
#endif

static VALUE
Debase_init_variables()
{
  started = 0;
  verbose = Qfalse;
  locker = Qnil;
  file_filter_enabled = Qfalse;
  contexts = Qnil;
  catchpoints = Qnil;
  breakpoints = Qnil;

  context_init_variables();
  breakpoint_init_variables();

  return Qtrue;
}

/*
 *   Document-class: Debase
 *
 *   == Summary
 *
 *   This is a singleton class allows controlling the debugger. Use it to start/stop debugger,
 *   set/remove breakpoints, etc.
 */
void
Init_debase_internals()
{
  mDebase = rb_define_module("Debase");
  rb_define_module_function(mDebase, "setup_tracepoints", Debase_setup_tracepoints, 0);
  rb_define_module_function(mDebase, "remove_tracepoints", Debase_remove_tracepoints, 0);
  rb_define_module_function(mDebase, "current_context", Debase_current_context, 0);
  rb_define_module_function(mDebase, "debug_load", Debase_debug_load, -1);
  rb_define_module_function(mDebase, "contexts", Debase_contexts, 0);
  rb_define_module_function(mDebase, "breakpoints", Debase_breakpoints, 0);
  rb_define_module_function(mDebase, "catchpoints", Debase_catchpoints, 0);
  rb_define_module_function(mDebase, "started?", Debase_started, 0);
  rb_define_module_function(mDebase, "verbose?", Debase_verbose, 0);
  rb_define_module_function(mDebase, "verbose=", Debase_set_verbose, 1);
  rb_define_module_function(mDebase, "enable_file_filtering", Debase_enable_file_filtering, 1);
  rb_define_module_function(mDebase, "enable_trace_points", Debase_enable_trace_points, 0);
  rb_define_module_function(mDebase, "prepare_context", Debase_prepare_context, 0);
  rb_define_module_function(mDebase, "init_variables", Debase_init_variables, 0);
  rb_define_module_function(mDebase, "set_trace_flag_to_iseq", Debase_set_trace_flag_to_iseq, 1);

  //use only for tests
  rb_define_module_function(mDebase, "unset_iseq_flags", Debase_unset_trace_flags, 1);

  idAlive = rb_intern("alive?");
  idAtLine = rb_intern("at_line");
  idAtBreakpoint = rb_intern("at_breakpoint");
  idAtCatchpoint = rb_intern("at_catchpoint");
  idFileFilter = rb_intern("file_filter");
  idAccept = rb_intern("accept?");

  cContext = Init_context(mDebase);
  Init_breakpoint(mDebase);
  cDebugThread  = rb_define_class_under(mDebase, "DebugThread", rb_cThread);
  Debase_init_variables();

  rb_global_variable(&locker);
  rb_global_variable(&breakpoints);
  rb_global_variable(&catchpoints);
  rb_global_variable(&contexts);
}
