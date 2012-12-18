#ifndef DEBASE_INTERNALS
#define DEBASE_INTERNALS

#include <ruby.h>
#include <ruby/debug.h>

/* Debase::Context flags */
#define CTX_FL_SUSPEND      (1<<1)
#define CTX_FL_TRACING      (1<<2)
#define CTX_FL_SKIPPED      (1<<3)
#define CTX_FL_IGNORE       (1<<4)
#define CTX_FL_DEAD         (1<<5)
#define CTX_FL_WAS_RUNNING  (1<<6)
#define CTX_FL_ENABLE_BKPT  (1<<7)
#define CTX_FL_STEPPED      (1<<8)
#define CTX_FL_FORCE_MOVE   (1<<9)
#define CTX_FL_CATCHING     (1<<10)

#define CTX_FL_TEST(c,f)  ((c)->flags & (f))
#define CTX_FL_SET(c,f)   do { (c)->flags |= (f); } while (0)
#define CTX_FL_UNSET(c,f) do { (c)->flags &= ~(f); } while (0)

#define IS_THREAD_ALIVE(t) (rb_funcall((t), idAlive, 0) == Qtrue)

typedef enum {CTX_STOP_NONE, CTX_STOP_STEP, CTX_STOP_BREAKPOINT, CTX_STOP_CATCHPOINT} ctx_stop_reason;

typedef struct debug_frame_t
{
    struct debug_frame_t *prev;
    char *file;
    int line;
    VALUE binding;
    VALUE self;
} debug_frame_t;

typedef struct {
  debug_frame_t *stack;
  VALUE thread;
  int stack_size;
  int thnum;
  int flags;
  ctx_stop_reason stop_reason;
} debug_context_t;

/* Debase::Context functions */
extern VALUE Init_context(VALUE mDebase);
extern VALUE context_create(VALUE thread);
extern VALUE Context_ignored(VALUE self);
extern void push_frame(VALUE context_object, char* file, int line, VALUE binding, VALUE self);
extern void pop_frame(VALUE context_object);
extern void update_frame(VALUE context_object, char* file, int line, VALUE binding, VALUE self);
#endif