#include <debase_internals.h>

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