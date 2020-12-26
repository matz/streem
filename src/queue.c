#include "queue.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

struct strm_queue_node {
  struct strm_queue_node *next;
  void *n;
};

#ifdef NO_LOCKFREE_QUEUE

#include <pthread.h>

struct strm_queue {
  struct strm_queue_node* head;
  struct strm_queue_node* tail;
  pthread_mutex_t mutex;
};

struct strm_queue*
strm_queue_new()
{
  struct strm_queue* q;

  q = (struct strm_queue*)malloc(sizeof(struct strm_queue));
  if (q == NULL) {
    return NULL;
  }
  q->head = NULL;
  q->tail = NULL;
  pthread_mutex_init(&q->mutex, NULL);
  return q;
}

int
strm_queue_add(struct strm_queue* q, void* val)
{
  struct strm_queue_node *node = (struct strm_queue_node*)malloc(sizeof(struct strm_queue_node));

  if (node == NULL) return 0;
  node->n = val;
  node->next = NULL;
  pthread_mutex_lock(&q->mutex);
  if (q->tail) {
    q->tail->next = node;
  }
  q->tail = node;
  if (!q->head) {
    q->head = node;
  }
  pthread_mutex_unlock(&q->mutex);
  return 1;
}

void*
strm_queue_get(struct strm_queue* q)
{
  struct strm_queue_node* t;
  void* n;

  pthread_mutex_lock(&q->mutex);
  while (!q->head) {
    pthread_mutex_unlock(&q->mutex);
    return NULL;
  }
  t = q->head;
  q->head = t->next;
  if (q->head == NULL)
    q->tail = NULL;
  pthread_mutex_unlock(&q->mutex);
  n = t->n;
  free(t);

  return n;
}

void
strm_queue_free(struct strm_queue* q)
{
  if (!q) return;
  if (q->head) {
    struct strm_queue_node* t = q->head;
    struct strm_queue_node* tmp;

    while (t) {
      tmp = t->next;
      free(t);
      t = tmp;
    }
  }
  pthread_mutex_destroy(&q->mutex);
  free(q);
}

int
strm_queue_empty_p(struct strm_queue* q)
{
  if (q->head == NULL) return 1;
  return 0;
}

#else  /* NO_LOCKFREE_QUEUE */
/* copied and modified https://github.com/supermartian/lockfree-queue
 *
 * Copyright (C) 2014 Yuzhong Wen<supermartian@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */


#include "atomic.h"

struct strm_queue {
  struct strm_queue_node* head;
  struct strm_queue_node* tail;
};

struct strm_queue*
strm_queue_new()
{
  struct strm_queue* q;

  q = (struct strm_queue*)malloc(sizeof(struct strm_queue));
  if (q == NULL) {
    return NULL;
  }
  q->head = (struct strm_queue_node*)malloc(sizeof(struct strm_queue_node)); /* Sentinel node */
  q->tail = q->head;
  q->head->next = NULL;
  return q;
}

int
strm_queue_add(struct strm_queue* q, void* val)
{
  struct strm_queue_node *node = (struct strm_queue_node*)malloc(sizeof(struct strm_queue_node));

  node->n = val;
  node->next = NULL;
  while (1) {
    struct strm_queue_node *n = q->tail;
    struct strm_queue_node *next = n->next;
    if (n != q->tail) continue;
    if (next == NULL) {
      if (strm_atomic_cas(n->next, next, node)) {
        strm_atomic_cas(q->tail, n, node);
        return 1;
      }
    }
    else {
      strm_atomic_cas(q->tail, n, next);
    }
  }
}

void*
strm_queue_get(struct strm_queue* q)
{
  while (1) {
    struct strm_queue_node *n = q->head;
    struct strm_queue_node *next = n->next;
    if (n != q->head) continue;
    if (next == NULL) {
      return NULL;
    }
    if (strm_atomic_cas(q->head, n, next)) {
      void *val = (void*)next->n;
      free(n);                  /* may cause ABA problem; use GC */
      return val;
    }
  }
}

void
strm_queue_free(struct strm_queue* q)
{
  struct strm_queue_node* n = q->head;

  free(q);
  while (n) {
    struct strm_queue_node* tmp = n->next;
    free(n);
    n = tmp;
  }
}

int
strm_queue_empty_p(struct strm_queue* q)
{
  if (q->head == q->tail) return 1;
  return 0;
}
#endif /* LOCKFREE_QUEUE */
