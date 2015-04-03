#include "strm.h"
#include <pthread.h>

struct strm_queue {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct strm_queue_task *fi, *hi, *fo;
};

strm_queue*
strm_queue_alloc()
{
  strm_queue *q = malloc(sizeof(strm_queue));

  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);
  q->fi = q->hi = q->fo = NULL;
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
push_high_task(strm_queue *q, struct strm_queue_task *t)
{
  pthread_mutex_lock(&q->mutex);
  if (q->hi) {
    t->next = q->hi->next;
    q->hi->next = t;
    q->hi = t;
  }
  else {
    t->next = q->fo;
    q->fo = t;
    q->hi = t;
  }
  if (!q->fi)
    q->fi = t;
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}

static void
push_low_task(strm_queue *q, struct strm_queue_task *t)
{
  pthread_mutex_lock(&q->mutex);
  if (q->fi) {
    q->fi->next = t;
  }
  if (!q->hi)
    q->hi = t;
  q->fi = t;
  if (!q->fo) {
    q->fo = t;
  }
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}

void
strm_queue_push(strm_queue *q, struct strm_queue_task *t)
{
  if (!q) return;

  if (t->strm->mode == strm_task_prod)
    push_low_task(q, t);
  else
    push_high_task(q, t);
}

struct strm_queue_task*
strm_queue_task(strm_task *strm, strm_func func, strm_value data)
{
  struct strm_queue_task *t;

  t = malloc(sizeof(struct strm_queue_task));
  t->strm = strm;
  t->func = func;
  t->data = data;
  t->next = NULL;

  return t;
}

int
strm_queue_exec(strm_queue *q)
{
  struct strm_queue_task *t;
  strm_task *strm;
  strm_func func;
  strm_value data;

  pthread_mutex_lock(&q->mutex);
  while (!q->fo) {
    pthread_cond_wait(&q->cond, &q->mutex);
  }
  t = q->fo;
  q->fo = t->next;
  if (t == q->hi) {
    q->hi = NULL;
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
