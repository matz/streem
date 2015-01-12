#include "strm.h"
#include <pthread.h>

struct strm_queue {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct strm_queue_entry *fi, *fo;
};

struct strm_queue_entry {
  strm_stream *strm;
  strm_func func;
  void *data;
  struct strm_queue_entry *next;
};

strm_queue*
strm_queue_alloc()
{
  strm_queue *q = malloc(sizeof(strm_queue));

  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);
  q->fi = q->fo = NULL;
  return q;
}

void
strm_queue_free(strm_queue *q)
{
  if (!q) return;
  if (q->fo) {
    struct strm_queue_entry *e = q->fo;
    struct strm_queue_entry *tmp;

    while (e) {
      tmp = e->next;
      free(e);
      e = tmp;
    }
  }
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond);
  free(q);
}

void
strm_queue_push(strm_queue *q, strm_stream *strm, strm_func func, void *data)
{
  struct strm_queue_entry *e;

  if (!q) return;

  e = malloc(sizeof(struct strm_queue_entry));
  pthread_mutex_lock(&q->mutex);
  e->strm = strm;
  e->func = func;
  e->data = data;
  e->next = NULL;
  if (q->fi) {
    q->fi->next = e;
  }
  q->fi = e;
  if (!q->fo) q->fo = e;
  pthread_cond_signal(&q->cond);
  pthread_mutex_unlock(&q->mutex);
}

int
strm_queue_exec(strm_queue *q)
{
  struct strm_queue_entry *e;
  strm_stream *strm;
  strm_func func;
  void *data;

  pthread_mutex_lock(&q->mutex);
  if (!q->fo) {
    pthread_cond_wait(&q->cond, &q->mutex);
  }
  e = q->fo;
  q->fo = e->next;
  if (!q->fo) q->fi = NULL;
  pthread_mutex_unlock(&q->mutex);

  strm = e->strm;
  func = e->func;
  data = e->data;
  free(e);

  (*func)(strm, data);
  return 1;
}

int
strm_queue_p(strm_queue *q)
{
  return q->fi != NULL;
}
