/* copied and modified https://github.com/supermartian/lockfree-queue
 *
 * Copyright (C) 2014 Yuzhong Wen<supermartian@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef STRM_QUEUE_H
#define STRM_QUEUE_H

#include <pthread.h>

/*
 * Here we implement a lock-free queue for buffering the
 * packets that come out from multiple filters.
 *
 * */

struct strm_queue {
  struct strm_queue_node* head;
  struct strm_queue_node* tail;
};

struct strm_queue* strm_queue_new();
int strm_queue_add(struct strm_queue* queue, void* val);
void* strm_queue_get(struct strm_queue* queue);

#endif /* !STRM_QUEUE_H */
