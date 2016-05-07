/* copied and modified https://github.com/supermartian/lockfree-queue
 *
 * Copyright (C) 2014 Yuzhong Wen<supermartian@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "atomic.h"

struct strm_queue {
  struct strm_queue_node* head;
  struct strm_queue_node* tail;
};

struct strm_queue_node {
  struct strm_queue_node *next;
  void *n;
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
