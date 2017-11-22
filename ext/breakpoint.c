#include <debase_internals.h>

#ifdef _WIN32
#include <ctype.h>
#endif

#if defined DOSISH
#define isdirsep(x) ((x) == '/' || (x) == '\\')
#else
#define isdirsep(x) ((x) == '/')
#endif

static VALUE cBreakpoint;
static int breakpoint_max;

static ID idEval;

static VALUE
eval_expression(VALUE args)
{
  return rb_funcall2(rb_mKernel, idEval, 2, RARRAY_PTR(args));
}

extern VALUE
catchpoint_hit_count(VALUE catchpoints, VALUE exception, VALUE *exception_name) {
  VALUE ancestors;
  VALUE expn_class;
  VALUE aclass;
  VALUE mod_name;
  VALUE hit_count;
  int i;

  if (catchpoints == Qnil /*|| st_get_num_entries(RHASH_TBL(rdebug_catchpoints)) == 0)*/)
    return Qnil;
  expn_class = rb_obj_class(exception);
  ancestors = rb_mod_ancestors(expn_class);
  for(i = 0; i < RARRAY_LENINT(ancestors); i++)
  {
    aclass    = rb_ary_entry(ancestors, i);
    mod_name  = rb_mod_name(aclass);
    hit_count = rb_hash_aref(catchpoints, mod_name);
    if(hit_count != Qnil)
    {
      *exception_name = mod_name;
      return hit_count;
    }
  }
  return Qnil;
}

static void
Breakpoint_mark(breakpoint_t *breakpoint)
{
  rb_gc_mark(breakpoint->source);
  rb_gc_mark(breakpoint->expr);
}

static VALUE
Breakpoint_create(VALUE klass)
{
    breakpoint_t *breakpoint;

    breakpoint = ALLOC(breakpoint_t);
    return Data_Wrap_Struct(klass, Breakpoint_mark, xfree, breakpoint);
}

static VALUE
Breakpoint_initialize(VALUE self, VALUE source, VALUE pos, VALUE expr)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  breakpoint->id = ++breakpoint_max;
  breakpoint->source = StringValue(source);
  breakpoint->line = FIX2INT(pos);
  breakpoint->enabled = Qtrue;
  breakpoint->expr = NIL_P(expr) ? expr : StringValue(expr);

  return Qnil;
}

static VALUE
Breakpoint_remove(VALUE self, VALUE breakpoints, VALUE id_value)
{
  int i;
  int id;
  VALUE breakpoint_object;
  breakpoint_t *breakpoint;

  if (breakpoints == Qnil) return Qnil;

  id = FIX2INT(id_value);

  for(i = 0; i < RARRAY_LENINT(breakpoints); i++)
  {
    breakpoint_object = rb_ary_entry(breakpoints, i);
    Data_Get_Struct(breakpoint_object, breakpoint_t, breakpoint);
    if(breakpoint->id == id)
    {
      rb_ary_delete_at(breakpoints, i);
      return breakpoint_object;
    }
  }
  return Qnil;
}

static VALUE
Breakpoint_id(VALUE self)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  return INT2FIX(breakpoint->id);
}

static VALUE
Breakpoint_source(VALUE self)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  return breakpoint->source;
}

static VALUE
Breakpoint_expr_get(VALUE self)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  return breakpoint->expr;
}

static VALUE
Breakpoint_expr_set(VALUE self, VALUE new_val)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  breakpoint->expr = new_val;
  return breakpoint->expr;
}

static VALUE
Breakpoint_enabled_set(VALUE self, VALUE new_val)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  breakpoint->enabled = new_val;
  return breakpoint->enabled;
}

static VALUE
Breakpoint_enabled_get(VALUE self)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  return breakpoint->enabled;
}

static VALUE
Breakpoint_pos(VALUE self)
{
  breakpoint_t *breakpoint;

  Data_Get_Struct(self, breakpoint_t, breakpoint);
  return INT2FIX(breakpoint->line);
}

int
filename_cmp_impl(VALUE source, char *file)
{
  char *source_ptr, *file_ptr;
  long s_len, f_len, min_len;
  long s,f;
  int dirsep_flag = 0;

  s_len = RSTRING_LEN(source);
  f_len = strlen(file);
  min_len = s_len < f_len ? s_len : f_len;

  source_ptr = RSTRING_PTR(source);
  file_ptr   = file;

  for( s = s_len - 1, f = f_len - 1; s >= s_len - min_len && f >= f_len - min_len; s--, f-- )
  {
    if((source_ptr[s] == '.' || file_ptr[f] == '.') && dirsep_flag)
      return 1;
    if(isdirsep(source_ptr[s]) && isdirsep(file_ptr[f]))
      dirsep_flag = 1;
#ifdef DOSISH_DRIVE_LETTER
    else if (s == 0)
      return(toupper(source_ptr[s]) == toupper(file_ptr[f]));
#endif
    else if(source_ptr[s] != file_ptr[f])
      return 0;
  }
  return 1;
}

int
filename_cmp(VALUE source, char *file)
{
#ifdef _WIN32
  return filename_cmp_impl(source, file);
#else
#ifdef PATH_MAX
  char path[PATH_MAX + 1];    
  path[PATH_MAX] = 0;
  return filename_cmp_impl(source, realpath(file, path) != NULL ? path : file);
#else
  char *path;
  int result;
  path = realpath(file, NULL);
  result = filename_cmp_impl(source, path == NULL ? file : path);
  free(path);
  return result;
#endif  
#endif  
}

static int
check_breakpoint_by_pos(VALUE breakpoint_object, char *file, int line)
{
    breakpoint_t *breakpoint;

    if(breakpoint_object == Qnil)
        return 0;
    Data_Get_Struct(breakpoint_object, breakpoint_t, breakpoint);
    if (Qtrue != breakpoint->enabled) return 0;
    if(breakpoint->line != line)
        return 0;
    if(filename_cmp(breakpoint->source, file))
        return 1;
    return 0;
}

static int
check_breakpoint_expr(VALUE breakpoint_object, VALUE trace_point)
{
  breakpoint_t *breakpoint;
  VALUE binding, args, result;
  int error;

  if(breakpoint_object == Qnil) return 0;
  Data_Get_Struct(breakpoint_object, breakpoint_t, breakpoint);
  if (Qtrue != breakpoint->enabled) return 0;
  if (NIL_P(breakpoint->expr)) return 1;

  if (NIL_P(trace_point)) {
    binding = rb_const_get(rb_cObject, rb_intern("TOPLEVEL_BINDING"));
  } else {
    binding = rb_tracearg_binding(rb_tracearg_from_tracepoint(trace_point));
  }

  args = rb_ary_new3(2, breakpoint->expr, binding);
  result = rb_protect(eval_expression, args, &error);
  return !error && RTEST(result);
}

static VALUE
Breakpoint_find(VALUE self, VALUE breakpoints, VALUE source, VALUE pos, VALUE trace_point)
{
  return breakpoint_find(breakpoints, source, pos, trace_point);
}

extern VALUE
breakpoint_find(VALUE breakpoints, VALUE source, VALUE pos, VALUE trace_point)
{
  VALUE breakpoint_object;
  char *file;
  int line;
  int i;

  file = RSTRING_PTR(source);
  line = FIX2INT(pos);
  for(i = 0; i < RARRAY_LENINT(breakpoints); i++)
  {
    breakpoint_object = rb_ary_entry(breakpoints, i);
    if (check_breakpoint_by_pos(breakpoint_object, file, line) &&
      check_breakpoint_expr(breakpoint_object, trace_point))
    {
      return breakpoint_object;
    }
  }
  return Qnil;
}

extern void
breakpoint_init_variables()
{
  breakpoint_max = 0;
}

extern void
Init_breakpoint(VALUE mDebase)
{
  breakpoint_init_variables();
  cBreakpoint = rb_define_class_under(mDebase, "Breakpoint", rb_cObject);
  rb_define_singleton_method(cBreakpoint, "find", Breakpoint_find, 4);
  rb_define_singleton_method(cBreakpoint, "remove", Breakpoint_remove, 2);
  rb_define_method(cBreakpoint, "initialize", Breakpoint_initialize, 3);
  rb_define_method(cBreakpoint, "id", Breakpoint_id, 0);
  rb_define_method(cBreakpoint, "source", Breakpoint_source, 0);
  rb_define_method(cBreakpoint, "pos", Breakpoint_pos, 0);

  /* <For tests> */
  rb_define_method(cBreakpoint, "expr", Breakpoint_expr_get, 0);
  rb_define_method(cBreakpoint, "expr=", Breakpoint_expr_set, 1);
  rb_define_method(cBreakpoint, "enabled", Breakpoint_enabled_get, 0);
  rb_define_method(cBreakpoint, "enabled=", Breakpoint_enabled_set, 1);
  /* </For tests> */

  rb_define_alloc_func(cBreakpoint, Breakpoint_create);

  idEval = rb_intern("eval");
}
