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
task_tid(strm_task* s, int tid)
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
task_push(int tid, struct strm_queue_task* t)
{
  strm_task *s = t->strm;

  assert(threads != NULL);
  task_tid(s, tid);
  strm_queue_push(threads[s->tid].queue, t);
}

void
strm_task_push(struct strm_queue_task* t)
{
  task_push(-1, t);
}

void
strm_emit(strm_task* task, strm_value data, strm_callback func)
{
  int tid = task->tid;
  strm_task **d = task->dst;
  strm_task **t = d;
  strm_task **e = d + task->dlen;

  if (!strm_nil_p(data)) {
    while (t < e) {
      if ((*d)->mode == strm_task_killed) {
        task->dlen--;
        t++;
      }
      else {
        task_push(tid, strm_queue_task(*d, (*d)->start_func, data));
        *d++ = *t++;
        tid++;
      }
    }
  }
  if (func) {
    strm_task_push(strm_queue_task(task, func, strm_nil_value()));
  }
}

int
strm_task_connect(strm_task* src, strm_task* dst)
{
  strm_task** d;

  assert(dst->mode != strm_task_prod);
  d = src->dst;
  if (!d) {
    d = malloc(sizeof(strm_task*));
  }
  else {
    d = realloc(d, sizeof(strm_task*)*(src->dlen+1));
  }
  d[src->dlen++] = dst;
  src->dst = d;

  if (src->mode == strm_task_prod) {
    task_init();
    pipeline_count++;
    strm_task_push(strm_queue_task(src, src->start_func, strm_nil_value()));
  }
  return STRM_OK;
}

int cpu_count();
void strm_init_io_loop();
strm_task* strm_io_deque();
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
  if (pipeline_count == 0) return STRM_OK;
  task_init();
  for (;;) {
    pthread_mutex_lock(&pipeline_mutex);
    pthread_cond_wait(&pipeline_cond, &pipeline_mutex);
    pthread_mutex_unlock(&pipeline_mutex);
    if (pipeline_count == 0) {
      int i;

      for (i=0; i<thread_max; i++) {
        if (strm_queue_p(threads[i].queue))
          break;
      }
      if (i == thread_max) break;
    }
  }
  return STRM_OK;
}

strm_task*
strm_task_new(strm_task_mode mode, strm_callback start_func, strm_callback close_func, void* data)
{
  strm_task *s = malloc(sizeof(strm_task));
  s->type = STRM_PTR_TASK;
  s->tid = -1;                  /* -1 means uninitialized */
  s->mode = mode;
  s->start_func = start_func;
  s->close_func = close_func;
  s->data = data;
  s->dst = NULL;
  s->dlen = 0;
  s->flags = 0;

  return s;
}

static int
pipeline_finish(strm_task* task, strm_value data)
{
  pthread_mutex_lock(&pipeline_mutex);
  pipeline_count--;
  if (pipeline_count == 0) {
    pthread_cond_signal(&pipeline_cond);
  }
  pthread_mutex_unlock(&pipeline_mutex);
  return STRM_OK;
}

void
strm_task_close(strm_task* task)
{
  strm_task** d;
  size_t dlen = task->dlen;

  if (task->close_func) {
    (*task->close_func)(task, strm_nil_value());
    free(task->data);
  }

  d = task->dst;
  while (dlen--) {
    if ((*d)->mode != strm_task_killed) {
      strm_task_push(strm_queue_task(*d, (strm_callback)strm_task_close, strm_nil_value()));
    }
    d++;
  }
  free(task->dst);
  if (task->mode == strm_task_prod) {
    strm_task_push(strm_queue_task(task, pipeline_finish, strm_nil_value()));
  }
  task->mode = strm_task_killed;
}
