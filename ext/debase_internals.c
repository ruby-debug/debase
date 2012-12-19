#include <debase_internals.h>

static VALUE mDebase;                 /* Ruby Debase Module object */
static VALUE cContext;
static VALUE cBreakpoint;
static VALUE cDebugThread;

static VALUE locker = Qnil;

static VALUE tpLine;
static VALUE tpCall;
static VALUE tpReturn;

static VALUE idList;
static VALUE idCurrent;
static VALUE idContexts;
static VALUE idAlive;
static VALUE idPath;
static VALUE idLineno;
static VALUE idBinding;
static VALUE idAtLine;
static VALUE idAtReturn;
static VALUE idAtBreakpoint;
static VALUE idFind;
static VALUE idBreakpoints;
static VALUE idSelf;

#define CURRENT_THREAD() (rb_funcall(rb_cThread, idCurrent, 0))

static VALUE
Debase_thread_context(VALUE self, VALUE thread)
{
  VALUE contexts;
  VALUE context;

  contexts = rb_ivar_get(self, idContexts);
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
  VALUE current;  

  current = CURRENT_THREAD();
  return Debase_thread_context(self, current);	
}

static int
remove_dead_threads(VALUE thread, VALUE context, VALUE ignored)
{
  return IS_THREAD_ALIVE(thread) ? ST_CONTINUE : ST_DELETE;
}

static void
cleanup_contexts()
{
  VALUE contexts;

  contexts = rb_ivar_get(mDebase, idContexts);
  rb_hash_foreach(contexts, remove_dead_threads, 0);
}

static void 
cleanup(debug_context_t *context)
{
  VALUE thread;

  context->stop_reason = CTX_STOP_NONE;

  /* check that all contexts point to alive threads */
  cleanup_contexts();

  /* release a lock */
  locker = Qnil;
  
  /* let the next thread to run */
  thread = remove_from_locked();
  if(thread != Qnil)
    rb_thread_run(thread);
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

inline void
load_frame_info(VALUE trace_point, VALUE *path, VALUE *lineno, VALUE *binding, VALUE *self)
{
  *path = rb_funcall(trace_point, idPath, 0);
  *lineno = rb_funcall(trace_point, idLineno, 0);
  *binding = rb_funcall(trace_point, idBinding, 0);
  *self = rb_funcall(trace_point, idSelf, 0);
}

static void
process_line_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE binding;
  VALUE self;
  VALUE context_object;
  VALUE breakpoint;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, CURRENT_THREAD())) return;

  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  update_frame(context_object, RSTRING_PTR(path), FIX2INT(lineno), binding, self);
  breakpoint = rb_funcall(cBreakpoint, idFind, 4, rb_ivar_get(mDebase, idBreakpoints), path, lineno, binding);
  if (breakpoint != Qnil) {
    context->stop_reason = CTX_STOP_STEP;
    rb_funcall(context_object, idAtBreakpoint, 1, breakpoint);
    rb_funcall(context_object, idAtLine, 2, path, lineno);
  }
  cleanup(context);
}

static void
process_return_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE binding;
  VALUE self;
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, CURRENT_THREAD())) return;

  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  // rb_funcall(context_object, idAtReturn, 2, path, lineno);
  pop_frame(context_object);
  cleanup(context);
}

static void
process_call_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE binding;
  VALUE self;
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);
  if (!check_start_processing(context, CURRENT_THREAD())) return;
  
  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  push_frame(context_object, RSTRING_PTR(path), FIX2INT(lineno), binding, self);
  cleanup(context);
}

static VALUE
Debase_setup_tracepoints(VALUE self)
{	
  tpLine = rb_tracepoint_new(Qnil, RUBY_EVENT_LINE, process_line_event, NULL);
  rb_tracepoint_enable(tpLine);
  tpReturn = rb_tracepoint_new(Qnil, RUBY_EVENT_RETURN | RUBY_EVENT_C_RETURN | RUBY_EVENT_B_RETURN, process_return_event, NULL);
  rb_tracepoint_enable(tpReturn);
  tpCall = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_C_CALL | RUBY_EVENT_B_CALL, process_call_event, NULL);
  rb_tracepoint_enable(tpCall);
  return Qnil;
}

static VALUE
Debase_remove_tracepoints(VALUE self)
{ 
  if (tpLine != Qnil) rb_tracepoint_disable(tpLine);
  tpLine = Qnil;
  if (tpReturn != Qnil) rb_tracepoint_disable(tpReturn);
  tpReturn = Qnil;
  if (tpCall != Qnil) rb_tracepoint_disable(tpCall);
  tpCall = Qnil;
  return Qnil;
}

static VALUE
Debase_prepare_context(VALUE self, VALUE file, VALUE stop)
{
  VALUE context_object;
  debug_context_t *context;

  context_object = Debase_current_context(mDebase);
  Data_Get_Struct(context_object, debug_context_t, context);

  if(RTEST(stop)) context->stop_next = 1;
  ruby_script(RSTRING_PTR(file));
  return self;
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
  rb_define_module_function(mDebase, "prepare_context", Debase_prepare_context, 2);

  idList = rb_intern("list");
  idCurrent = rb_intern("current");
  idContexts = rb_intern("@contexts");
  idAlive = rb_intern("alive?");
  idPath = rb_intern("path");
  idLineno = rb_intern("lineno");
  idBinding = rb_intern("binding");
  idAtLine = rb_intern("at_line");
  idAtReturn = rb_intern("at_return");
  idAtBreakpoint = rb_intern("at_breakpoint");  
  idFind = rb_intern("find");
  idBreakpoints = rb_intern("@breakpoints");
  idSelf = rb_intern("self");

  cContext = Init_context(mDebase);

  cBreakpoint = rb_define_class_under(mDebase, "Breakpoint", rb_cObject);
  cDebugThread  = rb_define_class_under(mDebase, "DebugThread", rb_cThread);
}