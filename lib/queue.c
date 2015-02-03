#include "strm.h"
#include <pthread.h>

struct strm_queue {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct strm_queue_task *fi, *fm, *fo;
};

strm_queue*
strm_queue_alloc()
{
  strm_queue *q = malloc(sizeof(strm_queue));

  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);
  q->fi = q->fm = q->fo = NULL;
  return q;
}

void
strm_queue_free(strm_queue *q)
{
  if (!q) return;
  if (q->fo) {
    struct strm_queue_task *t = q->fo;
    struct strm_queue_task *tmp;

    while (t) {
      tmp = t->next;
      free(t);
      t = tmp;
    }
  }
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond);
  free(q);
}

static void
queue_push_task(strm_queue *q, struct strm_queue_task *t)
{
  pthread_mutex_lock(&q->mutex);
  if (q->fi) {
    q->fi->next = t;
  }
  q->fi = t;
  if (!q->fm) {
    q->fm = t;
  }
  if (!q->fo) {
    q->fo = t;
  }
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}

void
strm_queue_push_io(strm_queue *q, struct strm_queue_task *t)
{
  if (!q) return;
  pthread_mutex_lock(&q->mutex);
  if (q->fm) {
    t->next = q->fm->next;
    q->fm->next = t;
  }
  if (!q->fi || q->fm == q->fi)
    q->fi = t;
  q->fm = t;
  if (!q->fo) q->fo = t;
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}

struct strm_queue_task*
strm_queue_task(strm_stream *strm, strm_func func, void *data)
{
  struct strm_queue_task *t;

  t = malloc(sizeof(struct strm_queue_task));
  t->strm = strm;
  t->func = func;
  t->data = data;
  t->next = NULL;

  return t;
}

void
strm_queue_push(strm_queue *q, strm_stream *strm, strm_func func, void *data)
{
  if (!q) return;
  queue_push_task(q, strm_queue_task(strm, func, data));
}

int
strm_queue_exec(strm_queue *q)
{
  struct strm_queue_task *t;
  strm_stream *strm;
  strm_func func;
  void *data;

  pthread_mutex_lock(&q->mutex);
  while (!q->fo) {
    pthread_cond_wait(&q->cond, &q->mutex);
  }
  t = q->fo;
  q->fo = t->next;
  if (t == q->fm) {
    q->fm = NULL;
  }
  if (!q->fo) {
    q->fi = NULL;
  }
  pthread_mutex_unlock(&q->mutex);

  strm = t->strm;
  func = t->func;
  data = t->data;
  free(t);

  (*func)(strm, data);
  return 1;
}

int
strm_queue_size(strm_queue *q)
{
  int n = 0;
  struct strm_queue_task *e = q->fo;

  while (e) {
    n++;
    e = e->next;
  }
  return n;
}

int
strm_queue_p(strm_queue *q)
{
  return q->fi != NULL;
}
