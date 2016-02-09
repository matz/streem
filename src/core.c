#include "strm.h"
#include <pthread.h>

struct strm_worker {
  pthread_t th;
  strm_queue *queue;
} *workers;

static int worker_max;
static int pipeline_count = 0;
static pthread_mutex_t pipeline_mutex;
static pthread_cond_t pipeline_cond;

/* internal variable to tell multi-threaded mode */
int strm_event_loop_started = FALSE;

#include <assert.h>

static void task_init();

static int
task_tid(strm_stream* s, int tid)
{
  int i;

  if (s->tid < 0) {
    if (tid >= 0) {
      s->tid = tid % worker_max;
    }
    else {
      int n = 0;
      int max = 0;

      for (i=0; i<worker_max; i++) {
        int size = strm_queue_size(workers[i].queue);
        if (size == 0) break;
        if (size > max) {
          max = size;
          n = i;
        }
      }
      if (i == worker_max) {
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
  strm_stream *s = t->strm;

  assert(workers != NULL);
  task_tid(s, tid);
  strm_queue_push(workers[s->tid].queue, t);
}

void
strm_stream_push(struct strm_queue_task* t)
{
  task_push(-1, t);
}

void
strm_emit(strm_stream* task, strm_value data, strm_callback func)
{
  int tid = task->tid;
  strm_stream **d = task->dst;
  strm_stream **t = d;
  strm_stream **e = d + task->dlen;

  if (!strm_nil_p(data)) {
    while (t < e) {
      if ((*d)->mode == strm_killed) {
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
    strm_stream_push(strm_queue_task(task, func, strm_nil_value()));
  }
}

int
strm_stream_connect(strm_stream* src, strm_stream* dst)
{
  strm_stream** d;

  assert(dst->mode != strm_producer);
  d = src->dst;
  if (!d) {
    d = malloc(sizeof(strm_stream*));
  }
  else {
    d = realloc(d, sizeof(strm_stream*)*(src->dlen+1));
  }
  d[src->dlen++] = dst;
  src->dst = d;

  if (src->mode == strm_producer) {
    task_init();
    pipeline_count++;
    strm_stream_push(strm_queue_task(src, src->start_func, strm_nil_value()));
  }
  return STRM_OK;
}

int cpu_count();
void strm_init_io_loop();
strm_stream* strm_io_deque();
int strm_io_waiting();

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
task_ping()
{
  pthread_mutex_lock(&pipeline_mutex);
  pthread_cond_signal(&pipeline_cond);
  pthread_mutex_unlock(&pipeline_mutex);
}

static void*
task_loop(void *data)
{
  struct strm_worker *w = (struct strm_worker*)data;

  for (;;) {
    strm_queue_exec(w->queue);
    if (pipeline_count == 0 && !strm_queue_p(w->queue)) {
      task_ping();
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

  pthread_mutex_init(&pipeline_mutex, NULL);
  pthread_cond_init(&pipeline_cond, NULL);

  worker_max = worker_count();
  workers = malloc(sizeof(struct strm_worker)*worker_max);
  for (i=0; i<worker_max; i++) {
    workers[i].queue = strm_queue_alloc();
    pthread_create(&workers[i].th, NULL, task_loop, &workers[i]);
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

      for (i=0; i<worker_max; i++) {
        if (strm_queue_p(workers[i].queue))
          break;
      }
      if (i == worker_max) break;
    }
  }
  return STRM_OK;
}

strm_stream*
strm_stream_new(strm_stream_mode mode, strm_callback start_func, strm_callback close_func, void* data)
{
  strm_stream *s = malloc(sizeof(strm_stream));
  s->type = STRM_PTR_STREAM;
  s->tid = -1;                  /* -1 means uninitialized */
  s->mode = mode;
  s->start_func = start_func;
  s->close_func = close_func;
  s->data = data;
  s->dst = NULL;
  s->dlen = 0;
  s->flags = 0;
  s->exc = NULL;

  return s;
}

static int
pipeline_finish(strm_stream* strm, strm_value data)
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
strm_stream_close(strm_stream* strm)
{
  strm_stream** d;
  size_t dlen = strm->dlen;

  if (strm->close_func) {
    (*strm->close_func)(strm, strm_nil_value());
    free(strm->data);
  }

  d = strm->dst;
  while (dlen--) {
    if ((*d)->mode != strm_killed) {
      strm_stream_push(strm_queue_task(*d, (strm_callback)strm_stream_close, strm_nil_value()));
    }
    d++;
  }
  free(strm->dst);
  if (strm->mode == strm_producer) {
    strm_stream_push(strm_queue_task(strm, pipeline_finish, strm_nil_value()));
  }
  strm->mode = strm_killed;
}
