#include "attach.h"

#ifndef __GNUC__
#define __asm__ asm
#endif

/*
We need to prevent compiler from optimizing this function calls. For more details
see "noinline" section here: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
*/
static void
#if defined(_MSC_VER)
__declspec(noinline)
__func_to_set_breakpoint_at()
{
}
#else
__attribute__((noinline))
__func_to_set_breakpoint_at()
{
    __asm__("");
}
#endif

static void
__catch_line_event(rb_event_flag_t evflag, VALUE data, VALUE self, ID mid, VALUE klass)
{
    (void)sizeof(evflag);
    (void)sizeof(self);
    (void)sizeof(mid);
    (void)sizeof(klass);

    rb_remove_event_hook(__catch_line_event);
    if (rb_during_gc())
        return;
    __func_to_set_breakpoint_at();
}

int
debase_start_attach()
{
    if (rb_during_gc())
        return 1;
    rb_add_event_hook(__catch_line_event, RUBY_EVENT_LINE, (VALUE) NULL);
    return 0;
}

void
debase_rb_eval(const char *string_to_eval)
{
    rb_eval_string_protect(string_to_eval, NULL);
}

void
Init_attach()
{
  /*
  The only purpose of this library is to be dlopen'ed inside
  gdb/lldb. So no initialization here, you should directly
  call functions above.
  */
}
