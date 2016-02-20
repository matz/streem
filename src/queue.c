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

struct strm_queue_node {
  struct strm_queue_node *next;
  void *n;
};

struct strm_queue*
strm_queue_alloc()
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
strm_queue_add(struct strm_queue* root, void* val)
{
  struct strm_queue_node *n;
  struct strm_queue_node *node = (struct strm_queue_node*)malloc(sizeof(struct strm_queue_node));

  node->n = val; 
  node->next = NULL;
  while (1) {
    n = root->tail;
    if (strm_atomic_cas(&(n->next), NULL, node)) {
      break;
    }
    else {
      strm_atomic_cas(&(root->tail), n, n->next);
    }
  }
  strm_atomic_cas(&(root->tail), n, node);
  return 1;
}

void*
strm_queue_get(struct strm_queue* root)
{
  struct strm_queue_node *n;
  void *val;

  while (1) {
    n = root->head;
    if (n->next == NULL) {
      return NULL;
    }
    if (strm_atomic_cas(&(root->head), n, n->next)) {
      break;
    }
  }
  val = (void *) n->next->n;
  free(n);
  return val;
}
