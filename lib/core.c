#include "strm.h"
#include <pthread.h>

static strm_queue *task_q;

#include <assert.h>

void
strm_task_enque(strm_stream *s)
{
  strm_queue_put(task_q, s);
}

strm_stream*
strm_task_deque()
{
  return strm_queue_get(task_q);
}

void
strm_emit(strm_stream *strm, void *data, strm_func func)
{
  strm_stream *d = strm->dst;

  while (d) {
    if (!d->callback) {
      d->callback = d->start_func;
    }
    d->cb_data = data;
    strm_task_enque(d);
    d = d->nextd;
  }
  if (func) {
    strm->callback = func;
    strm->cb_data = NULL;
    strm_task_enque(strm);
  }
}

int
strm_connect(strm_stream *src, strm_stream *dst)
{
  strm_stream *s;

  if (dst->mode == strm_task_prod) {
    /* destination task should not be a producer */
    return 0;
  }
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
    src->callback = src->start_func;
    strm_task_enque(src);
  }
  return 1;
}

void strm_init_io_loop();
strm_stream *strm_io_deque();
int strm_io_queue();

int
strm_loop()
{
  strm_stream *s;

  strm_init_io_loop();
  for (;;) {
    while ((s = strm_task_deque())) {
      (*s->callback)(s);
    }
    if ((s = strm_io_deque())) {
      (*s->callback)(s);
    }
    if (strm_io_queue() == 0 && !strm_queue_p(task_q)) {
      break;
    }
  }
  strm_queue_free(task_q);
  return 1;
}

strm_stream*
strm_alloc_stream(strm_task_mode mode, strm_func start_func, void *data)
{
  strm_stream *s = malloc(sizeof(strm_stream));
  s->mode = mode;
  s->start_func = start_func;
  s->data = data;
  s->dst = NULL;
  s->nextd = NULL;
  s->nextq = NULL;
  s->callback = NULL;
  s->flags = 0;

  if (!task_q) {
    task_q = strm_queue_alloc();
  }
  return s;
}
