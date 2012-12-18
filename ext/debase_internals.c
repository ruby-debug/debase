#include <debase_internals.h>

static VALUE mDebase;                 /* Ruby Debase Module object */
static VALUE cContext;
static VALUE cBreakpoint;

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
    context = context_create(thread);
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
  // int i;
  VALUE contexts;
  // VALUE list;
  // VALUE thread;

  contexts = rb_ivar_get(mDebase, idContexts);
  // list = rb_funcall(rb_cThread, idList, 0);
  // for(i = 0; i < RARRAY_LEN(list); i++)
  // {
  //   thread = rb_ary_entry(list, i);

  // }

  rb_hash_foreach(contexts, remove_dead_threads, 0);
}

static void cleanup()
{
  cleanup_contexts();
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

  context_object = Debase_current_context(mDebase);
  if (Context_ignored(context_object) == Qtrue) return;

  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  update_frame(context_object, RSTRING_PTR(path), FIX2INT(lineno), binding, self);
  breakpoint = rb_funcall(cBreakpoint, idFind, 4, rb_ivar_get(mDebase, idBreakpoints), path, lineno, binding);
  if (breakpoint != Qnil) {
    rb_funcall(context_object, idAtBreakpoint, 1, breakpoint);
    rb_funcall(context_object, idAtLine, 2, path, lineno);
  }
  cleanup();
}

static void
process_return_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE binding;
  VALUE self;
  VALUE context_object;

  context_object = Debase_current_context(mDebase);
  if (Context_ignored(context_object) == Qtrue) return;

  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  // rb_funcall(context_object, idAtReturn, 2, path, lineno);
  pop_frame(context_object);
  cleanup();
}

static void
process_call_event(VALUE trace_point, void *data)
{
  VALUE path;
  VALUE lineno;
  VALUE binding;
  VALUE self;
  VALUE context_object;

  context_object = Debase_current_context(mDebase);
  if (Context_ignored(context_object) == Qtrue) return;
  
  load_frame_info(trace_point, &path, &lineno, &binding, &self);
  push_frame(context_object, RSTRING_PTR(path), FIX2INT(lineno), binding, self);
  cleanup();
}

static VALUE
Debase_setup_tracepoints(VALUE self)
{
  VALUE trace_point;	
  trace_point = rb_tracepoint_new(Qnil, RUBY_EVENT_LINE, process_line_event, NULL);
  rb_tracepoint_enable(trace_point);
  trace_point = rb_tracepoint_new(Qnil, RUBY_EVENT_RETURN | RUBY_EVENT_C_RETURN | RUBY_EVENT_B_RETURN, process_return_event, NULL);
  rb_tracepoint_enable(trace_point);
  trace_point = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_C_CALL | RUBY_EVENT_B_CALL, process_call_event, NULL);
  rb_tracepoint_enable(trace_point);
  return Qnil;
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
  rb_define_module_function(mDebase, "current_context", Debase_current_context, 0);

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
}