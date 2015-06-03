#include "strm.h"
#include <pthread.h>

struct strm_thread {
  pthread_t th;
  strm_queue *queue;
} *threads;

static int thread_max;
static int pipeline_count = 0;
static pthread_mutex_t pipeline_mutex;
static pthread_cond_t pipeline_cond;

/* internal variable to tell multi-threaded mode */
int strm_event_loop_started = FALSE;

#include <assert.h>

static void task_init();

static int
task_tid(strm_task *s, int tid)
{
  int i;

  if (s->tid < 0) {
    if (tid >= 0) {
      s->tid = tid % thread_max;
    }
    else {
      int n = 0;
      int max = 0;

      for (i=0; i<thread_max; i++) {
        int size = strm_queue_size(threads[i].queue);
        if (size == 0) break;
        if (size > max) {
          max = size;
          n = i;
        }
      }
      if (i == thread_max) {
        s->tid = n;
      }
      else {
        s->tid = i;
      }
    }
  }
  return s->tid;
}

static void
task_push(int tid, struct strm_queue_task *t)
{
  strm_task *s = t->strm;

  assert(threads != NULL);
  task_tid(s, tid);
  strm_queue_push(threads[s->tid].queue, t);
}

void
strm_task_push(struct strm_queue_task *t)
{
  task_push(-1, t);
}

void
strm_emit(strm_task *strm, strm_value data, strm_callback func)
{
  strm_task *d = strm->dst;
  int tid = strm->tid;

  if (!strm_nil_p(data)) {
    while (d) {
      task_push(tid, strm_queue_task(d, d->start_func, data));
      d = d->nextd;
      tid++;
    }
  }
  if (func) {
    strm_task_push(strm_queue_task(strm, func, strm_nil_value()));
  }
}

int
strm_connect(strm_task *src, strm_task *dst)
{
  strm_task *s;

  assert(dst->mode != strm_task_prod);
  s = src->dst;
  if (s) {
    while (s->nextd) {
      s = s->nextd;
    }
    s->nextd = dst;
  }
  else {
    src->dst = dst;
  }

  if (src->mode == strm_task_prod) {
    task_init();
    pipeline_count++;
    strm_task_push(strm_queue_task(src, src->start_func, strm_nil_value()));
  }
  return 1;
}

int cpu_count();
void strm_init_io_loop();
strm_task *strm_io_deque();
int strm_io_waiting();

static int
thread_count()
{
  char *e = getenv("STRM_THREAD_MAX");
  int n;

  if (e) {
    n = atoi(e);
    if (n > 0) return n;
  }
  return cpu_count();
}

static void
task_ping()
{
  pthread_mutex_lock(&pipeline_mutex);
  pthread_cond_signal(&pipeline_cond);
  pthread_mutex_unlock(&pipeline_mutex);
}

static void*
task_loop(void *data)
{
  struct strm_thread *th = (struct strm_thread*)data;

  for (;;) {
    strm_queue_exec(th->queue);
    if (pipeline_count == 0 && !strm_queue_p(th->queue)) {
      task_ping();
    }
  }
  return NULL;
}

static void
task_init()
{
  int i;

  if (threads) return;

  strm_event_loop_started = TRUE;
  strm_init_io_loop();

  pthread_mutex_init(&pipeline_mutex, NULL);
  pthread_cond_init(&pipeline_cond, NULL);

  thread_max = thread_count();
  threads = malloc(sizeof(struct strm_thread)*thread_max);
  for (i=0; i<thread_max; i++) {
    threads[i].queue = strm_queue_alloc();
    pthread_create(&threads[i].th, NULL, task_loop, &threads[i]);
  }
}

int
strm_loop()
{
  if (pipeline_count == 0) return 0;
  task_init();
  for (;;) {
    pthread_mutex_lock(&pipeline_mutex);
    pthread_cond_wait(&pipeline_cond, &pipeline_mutex);
    pthread_mutex_unlock(&pipeline_mutex);
    if (pipeline_count == 0) {
      int i;

      for (i=0; i<thread_max; i++) {
        if (strm_queue_size(threads[i].queue) > 0)
          break;
      }
      if (i == thread_max) break;
    }
  }
  return 1;
}

strm_task*
strm_alloc_stream(strm_task_mode mode, strm_callback start_func, strm_callback close_func, void *data)
{
  strm_task *s = malloc(sizeof(strm_task));
  s->tid = -1;                  /* -1 means uninitialized */
  s->mode = mode;
  s->start_func = start_func;
  s->close_func = close_func;
  s->data = data;
  s->dst = NULL;
  s->nextd = NULL;
  s->flags = 0;

  return s;
}

void
pipeline_finish(strm_task *strm, strm_value data)
{
  pthread_mutex_lock(&pipeline_mutex);
  pipeline_count--;
  if (pipeline_count == 0) {
    pthread_cond_signal(&pipeline_cond);
  }
  pthread_mutex_unlock(&pipeline_mutex);
}

void
strm_close(strm_task *strm)
{
  if (strm->close_func) {
    (*strm->close_func)(strm, strm_nil_value());
  }
  strm_task *d = strm->dst;

  while (d) {
    strm_task_push(strm_queue_task(d, (strm_callback)strm_close, strm_nil_value()));
    d = d->nextd;
  }
  if (strm->mode == strm_task_prod) {
    strm_task_push(strm_queue_task(strm, pipeline_finish, strm_nil_value()));
  }
}
