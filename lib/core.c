#include "strm.h"
#include <pthread.h>

static strm_queue *task_q;

#include <assert.h>

void
strm_task_push(strm_stream *s, strm_func func, void *data)
{
  strm_queue_push(task_q, s, func, data);
}

int
strm_task_exec()
{
  return strm_queue_exec(task_q);
}

void
strm_emit(strm_stream *strm, void *data, strm_func func)
{
  strm_stream *d = strm->dst;

  while (d) {
    strm_task_push(d, d->start_func, data);
    d = d->nextd;
  }
  if (func) {
    strm_task_push(strm, func, NULL);
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
    strm_task_push(src, src->start_func, NULL);
  }
  return 1;
}

void strm_init_io_loop();
strm_stream *strm_io_deque();
int strm_io_waiting();

int
strm_loop()
{
  strm_init_io_loop();
  for (;;) {
    strm_task_exec();
    if (strm_io_waiting() == 0 && !strm_queue_p(task_q)) {
      break;
    }
  }
  strm_queue_free(task_q);
  return 1;
}

strm_stream*
strm_alloc_stream(strm_task_mode mode, strm_func start_func, strm_func close_func, void *data)
{
  strm_stream *s = malloc(sizeof(strm_stream));
  s->mode = mode;
  s->start_func = start_func;
  s->close_func = close_func;
  s->data = data;
  s->dst = NULL;
  s->nextd = NULL;
  s->flags = 0;

  if (!task_q) {
    task_q = strm_queue_alloc();
  }
  return s;
}

void
strm_close(strm_stream *strm)
{
  if (strm->close_func) {
    (*strm->close_func)(strm, NULL);
  }
  strm_stream *d = strm->dst;

  while (d) {
    strm_task_push(d, strm_close, NULL);
    d = d->nextd;
  }
}
