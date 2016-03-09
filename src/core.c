#include "strm.h"
#include "queue.h"
#include "atomic.h"
#include <pthread.h>
#include <sched.h>

struct strm_worker {
  pthread_t th;
} *workers;

static struct strm_queue* queue;
static struct strm_queue* prod_queue;
static int worker_max;
static int stream_count = 0;

/* internal variable to tell multi-threaded mode */
int strm_event_loop_started = FALSE;

#include <assert.h>

static void task_init();

struct strm_task*
strm_task_new(strm_callback func, strm_value data)
{
  struct strm_task *t;
 
  t = malloc(sizeof(struct strm_task));
  t->func = func;
  t->data = data;

  return t;
}

void
strm_task_add(strm_stream* strm, struct strm_task* task)
{
  strm_queue_add(strm->queue, task);
  if (strm->mode == strm_producer) {
    strm_queue_add(prod_queue, strm);
  }
  else {
    strm_queue_add(queue, strm);
  }
}

void
strm_task_push(strm_stream* strm, strm_callback func, strm_value data)
{
  strm_task_add(strm, strm_task_new(func, data));
}

void
strm_emit(strm_stream* strm, strm_value data, strm_callback func)
{
  if (!strm_nil_p(data)) {
    strm_task_push(strm->dst, strm->dst->start_func, data);
  }
  sched_yield();
  if (func) {
    strm_task_push(strm, func, strm_nil_value());
  }
}

int
strm_stream_connect(strm_stream* src, strm_stream* dst)
{
  assert(src->mode != strm_consumer);
  assert(dst->mode != strm_producer);
  assert(src->dst == NULL);
  src->dst = dst;
  strm_atomic_inc(dst->refcnt);

  if (src->mode == strm_producer) {
    task_init();
    strm_task_push(src, src->start_func, strm_nil_value());
  }
  return STRM_OK;
}

int cpu_count();
void strm_init_io_loop();
strm_stream* strm_io_deque();

static int
worker_count()
{
  char *e = getenv("STRM_WORKER_MAX");
  int n;

  if (e) {
    n = atoi(e);
    if (n > 0) return n;
  }
  return cpu_count();
}

static void
task_exec(strm_stream* strm, struct strm_task* task)
{
  strm_callback func = task->func;
  strm_value data = task->data;

  free(task);
  if ((*func)(strm, data) == STRM_NG) {
    if (strm_option_verbose) {
      strm_eprint(strm);
    }
  }
}

static void*
task_loop(void *data)
{
  strm_stream* strm;

  for (;;) {
    strm = strm_queue_get(queue);
    if (!strm) {
      strm = strm_queue_get(prod_queue);
    }
    if (strm) {
      if (strm_atomic_cas(strm->excl, 0, 1)) {
        struct strm_task* t;

        while ((t = strm_queue_get(strm->queue)) != NULL) {
          task_exec(strm, t);
        }
        strm_atomic_cas(strm->excl, 1, 0);
      }
    }
    if (stream_count == 0) {
      break;
    }
  }
  return NULL;
}

static void
task_init()
{
  int i;

  if (workers) return;

  strm_event_loop_started = TRUE;
  strm_init_io_loop();

  queue = strm_queue_new();
  prod_queue = strm_queue_new();
  worker_max = worker_count();
  workers = malloc(sizeof(struct strm_worker)*worker_max);
  for (i=0; i<worker_max; i++) {
    pthread_create(&workers[i].th, NULL, task_loop, &workers[i]);
  }
}

int
strm_loop()
{
  if (stream_count == 0) return STRM_OK;
  task_init();
  for (;;) {
    sched_yield();
    if (stream_count == 0) {
      break;
    }
  }
  return STRM_OK;
}

strm_stream*
strm_stream_new(strm_stream_mode mode, strm_callback start_func, strm_callback close_func, void* data)
{
  strm_stream *s = malloc(sizeof(strm_stream));
  s->type = STRM_PTR_STREAM;
  s->mode = mode;
  s->start_func = start_func;
  s->close_func = close_func;
  s->data = data;
  s->dst = NULL;
  s->flags = 0;
  s->exc = NULL;
  s->refcnt = 0;
  s->excl = 0;
  s->queue = strm_queue_new();
  strm_atomic_inc(stream_count);

  return s;
}

void
strm_stream_close(strm_stream* strm)
{
  strm_stream_mode mode = strm->mode;

  if (mode == strm_killed) return;
  if (!strm_atomic_cas(strm->mode, mode, strm_killed)) {
    return;
  }
  strm_atomic_dec(strm->refcnt);
  if (strm->refcnt > 0) return;
  if (strm->close_func) {
    if ((*strm->close_func)(strm, strm_nil_value()) == STRM_NG)
      return;
    free(strm->data);
  }

  if (strm->dst) {
    strm_task_push(strm->dst, (strm_callback)strm_stream_close, strm_nil_value());
  }
  strm_atomic_dec(stream_count);
}
