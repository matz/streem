#include "strm.h"

/* ----- core private */
struct strm_queue {
  strm_stream *fi, *fo;
};

static struct strm_queue task_q = {NULL, NULL};
static struct strm_queue io_q   = {NULL, NULL};

static void
strm_enque(struct strm_queue *q, strm_stream *s)
{
  if (q->fi) {
    q->fi->nextq = s;
  }
  q->fi = s;
  if (!q->fo) q->fo = s;
}

static strm_stream*
strm_deque(struct strm_queue *q)
{
  strm_stream *s = q->fo;
  q->fo = s->nextq;
  if (!q->fo) q->fi = NULL;
  s->nextq = NULL;
  return s;
}

void
strm_task_enque(strm_stream *s)
{
  strm_enque(&task_q, s);
}

strm_stream*
strm_task_deque()
{
  return strm_deque(&task_q);
}

int
strm_task_que_p()
{
  return task_q.fi != NULL;
}

void
strm_io_enque(strm_stream *s, int fd)
{
  strm_enque(&io_q, s);
}

strm_stream*
strm_io_deque()
{
  return strm_deque(&io_q);
}

int
strm_io_que_p()
{
  return io_q.fi != NULL;
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

int
strm_loop()
{
  strm_stream *s;

  for (;;) {
    if (strm_io_que_p()) {
      /* enqueue I/O ready stream */
      s = strm_io_deque();
      strm_task_enque(s);
    }
    if (strm_task_que_p()) {
      s = strm_task_deque();
      (*s->callback)(s);
    }
    if (!strm_io_que_p() && !strm_task_que_p()) {
      break;
    }
  }
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
  
  return s;
}
