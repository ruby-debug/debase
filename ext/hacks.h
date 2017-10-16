#include <vm_core.h>
#include <version.h>

#define ruby_current_thread ((rb_thread_t *)RTYPEDDATA_DATA(rb_thread_current()))

#if RUBY_API_VERSION_CODE >= 20500
  #define NEW_RUBY 1
#endif

#ifdef NEW_RUBY
  #define TH_CFP(th) ((rb_control_frame_t *)(th)->ec.cfp)
#else
  #define TH_CFP(th) ((rb_control_frame_t *)(th)->cfp)
#endif

extern void
update_stack_size(debug_context_t *context) 
{
  rb_thread_t *thread;

  thread = ruby_current_thread;
  /* see backtrace_each in vm_backtrace.c */
  context->stack_size = (int)(RUBY_VM_END_CONTROL_FRAME(thread) - TH_CFP(thread) - 1);
  if (CTX_FL_TEST(context, CTX_FL_UPDATE_STACK)) {
    context->calced_stack_size = context->stack_size;
    CTX_FL_UNSET(context, CTX_FL_UPDATE_STACK);
  }
}
