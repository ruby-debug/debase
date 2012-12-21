#include <debase_internals.h>

#if defined DOSISH
#define isdirsep(x) ((x) == '/' || (x) == '\\')
#else
#define isdirsep(x) ((x) == '/')
#endif

static VALUE cBreakpoint;
static int breakpoint_max;

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
  for(i = 0; i < RARRAY_LEN(ancestors); i++)
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

  id = FIX2INT(id_value);

  for(i = 0; i < RARRAY_LEN(breakpoints); i++)
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

  if (realpath(file, path) != NULL)
    return filename_cmp_impl(source, path);
  else
    return filename_cmp_impl(source, file);
#else
  char *path = realpath(file, NULL);
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

extern VALUE
breakpoint_find(VALUE breakpoints, VALUE source, VALUE pos, VALUE binding)
{
  VALUE breakpoint_object;
  int line;
  int i;

  line = FIX2INT(pos);
  for(i = 0; i < RARRAY_LEN(breakpoints); i++)
  {
    breakpoint_object = rb_ary_entry(breakpoints, i);
    if (check_breakpoint_by_pos(breakpoint_object, RSTRING_PTR(source), FIX2INT(pos)))
    {
      return breakpoint_object;
    }
  }
  return Qnil;
}

extern VALUE
Init_breakpoint(VALUE mDebase)
{
  breakpoint_max = 0;
  cBreakpoint = rb_define_class_under(mDebase, "Breakpoint", rb_cObject);
  rb_define_singleton_method(cBreakpoint, "remove", Breakpoint_remove, 2);
  rb_define_method(cBreakpoint, "initialize", Breakpoint_initialize, 3);
  rb_define_method(cBreakpoint, "id", Breakpoint_id, 0);
  rb_define_method(cBreakpoint, "source", Breakpoint_source, 0);
  rb_define_method(cBreakpoint, "pos", Breakpoint_pos, 0);
  rb_define_alloc_func(cBreakpoint, Breakpoint_create);
  return cBreakpoint;
}