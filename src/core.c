#include "strm.h"
#include "queue.h"
#include "atomic.h"
#include <pthread.h>

struct strm_worker {
  pthread_t th;
  struct strm_queue* queue;
} *workers;

static struct strm_queue* queue;
static struct strm_queue* prod_queue;
static int worker_max;
static int stream_count = 0;

struct strm_task {
  strm_stream* strm;
  strm_callback func;
  strm_value data;
};

/* internal variable to tell multi-threaded mode */
int strm_event_loop_started = FALSE;

#include <assert.h>

static void task_init();

struct strm_task*
strm_task_alloc(strm_stream* strm, strm_callback func, strm_value data)
{
  struct strm_task *t;
 
  t = malloc(sizeof(struct strm_task));
  t->strm = strm;
  t->func = func;
  t->data = data;
 
  return t;
}

void
strm_task_add(struct strm_task* task)
{
  strm_queue_add(queue, task);
}

void
strm_task_push(strm_stream* strm, strm_callback func, strm_value data)
{
  struct strm_task *t = strm_task_alloc(strm, func, data);
  strm_task_add(t);
}

void
strm_emit(strm_stream* strm, strm_value data, strm_callback func)
{
  strm_stream **d = strm->dst;
  strm_stream **t = d;
  strm_stream **e = d + strm->dlen;

  if (!strm_nil_p(data)) {
    while (t < e) {
      if ((*d)->mode == strm_killed) {
        strm->dlen--;
        t++;
      }
      else {
        strm_task_push(*d, (*d)->start_func, data);
        *d++ = *t++;
      }
    }
  }
  if (func) {
    strm_queue_add(prod_queue, strm_task_alloc(strm, func, strm_nil_value()));
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
    strm_task_push(src, src->start_func, strm_nil_value());
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
task_exec(struct strm_task* task)
{
  strm_stream* strm = task->strm;
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
  struct strm_worker* w = (struct strm_worker*)data;
  struct strm_queue* q = w->queue;
  struct strm_task* t;

  for (;;) {
    t = strm_queue_get(q);
    if (t) {
      task_exec(t);
    }
    else {
      t = strm_queue_get(queue);
      if (!t) {
        t = strm_queue_get(prod_queue);
      }
      if (t) {
        switch (t->strm->mode) {
        case strm_filter:
        case strm_consumer:
          if (!t->strm->queue) {
            strm_atomic_cas(&t->strm->queue, NULL, q);
          }
          strm_queue_add(t->strm->queue, t);
          continue;
        default:
          break;
        }
        task_exec(t);
      }
    }
    if (stream_count < 4) {
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

  queue = strm_queue_alloc();
  prod_queue = strm_queue_alloc();
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
  if (stream_count == 0) return STRM_OK;
  task_init();
  for (;;) {
    if (stream_count < 4) {
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
  s->dlen = 0;
  s->flags = 0;
  s->exc = NULL;
  s->queue = NULL;
  strm_atomic_add(&stream_count, 1);

  return s;
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
      strm_task_push(*d, (strm_callback)strm_stream_close, strm_nil_value());
    }
    d++;
  }
  free(strm->dst);
  strm->mode = strm_killed;
  strm_atomic_sub(&stream_count, 1);
}
