#include <vm_core.h>
#include <version.h>

#define ruby_current_thread ((rb_thread_t *)RTYPEDDATA_DATA(rb_thread_current()))

#if RUBY_API_VERSION_CODE >= 20500
  #if (RUBY_RELEASE_YEAR == 2017 && RUBY_RELEASE_MONTH == 10 && RUBY_RELEASE_DAY == 10) //workaround for 2.5.0-preview1
    #define TH_CFP(thread) ((rb_control_frame_t *)(thread)->ec.cfp)
    #define TH_INFO(thread) ((rb_thread_t *)thread)
  #else
    #define TH_CFP(thread) ((rb_control_frame_t *)(thread)->ec->cfp)
    #define TH_INFO(thread) ((rb_execution_context_t *)(thread)->ec)
  #endif
#else
  #define TH_CFP(thread) ((rb_control_frame_t *)(thread)->cfp)
  #define TH_INFO(thread) ((rb_thread_t *)thread)
#endif

extern void
update_stack_size(debug_context_t *context) 
{
  rb_thread_t *thread;

  thread = ruby_current_thread;
  /* see backtrace_each in vm_backtrace.c */
  context->stack_size = (int)(RUBY_VM_END_CONTROL_FRAME(TH_INFO(thread)) - TH_CFP(thread) - 1);
  if (CTX_FL_TEST(context, CTX_FL_UPDATE_STACK)) {
    context->calced_stack_size = context->stack_size;
    CTX_FL_UNSET(context, CTX_FL_UPDATE_STACK);
  }
}
