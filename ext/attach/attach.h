#ifndef __ATTACH_H__
#define __ATTACH_H__

#include <ruby.h>
#include <ruby/debug.h>

void __func_to_set_breakpoint_at();
int start_attach();

#endif //__ATTACH_H__