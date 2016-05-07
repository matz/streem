/* copied and modified https://github.com/supermartian/lockfree-queue
 *
 * Copyright (C) 2014 Yuzhong Wen<supermartian@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef STRM_QUEUE_H
#define STRM_QUEUE_H

/*
 * Here we implement a lock-free queue for buffering the
 * packets that come out from multiple filters.
 *
 * */

struct strm_queue;

struct strm_queue* strm_queue_new();
int strm_queue_add(struct strm_queue* queue, void* val);
void* strm_queue_get(struct strm_queue* queue);
void strm_queue_free(struct strm_queue* queue);
int strm_queue_empty_p(struct strm_queue* queue);

#endif /* !STRM_QUEUE_H */
