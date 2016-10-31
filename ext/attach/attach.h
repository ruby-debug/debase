#ifndef __ATTACH_H__
#define __ATTACH_H__

#include <ruby.h>
#include <ruby/debug.h>

int debase_start_attach();
void debase_rb_eval(const char *);

#endif //__ATTACH_H__
