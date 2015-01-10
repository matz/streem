#include "strm.h"
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>

static strm_queue *io_queue;
static pthread_t io_worker;
static int io_wait_num = 0;
static int epoll_fd;

#define MAX_EVENTS 10

static strm_queue *io_queue;

strm_stream*
strm_io_deque()
{
  return strm_queue_get(io_queue);
}

int
strm_io_queue()
{
  return (io_wait_num > 0);
}

static void*
io_loop(void *d)
{
  struct epoll_event events[MAX_EVENTS];
  int i, n;

  for (;;) {
    n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0) {
      strm_queue_free(io_queue);
      return NULL;
    }
    for (i=0; i<n; i++) {
      strm_queue_put(io_queue, events[i].data.ptr);
    }
  }
  strm_queue_free(io_queue);
  return NULL;
}

void
strm_init_io_loop()
{
  io_queue = strm_queue_alloc();
  pthread_create(&io_worker, NULL, io_loop, NULL);
  epoll_fd = epoll_create(10);
}

static void
io_kick(strm_stream *strm, int fd)
{
  struct epoll_event ev;

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm;
  epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

static void
strm_io_start(strm_stream *strm, int fd, strm_func cb, uint32_t events)
{
  struct epoll_event ev;
  int n;

  strm->callback = cb;
  ev.events = events | EPOLLONESHOT;
  ev.data.ptr = strm;
  n = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  if (n == 0) {
    io_wait_num++;
  }
  else {
    if (errno == EPERM) {
      /* fd must be a regular file */
      /* enqueue task without waiting */
      strm->flags |= STRM_IO_NOWAIT;
      strm_queue_put(io_queue, strm);
    }
  }
}

void
strm_io_stop(strm_stream *strm, int fd)
{
  if (strm->flags & STRM_IO_NOWAIT) return;
  io_wait_num--;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void
strm_io_start_read(strm_stream *strm, int fd, strm_func cb)
{
  strm_io_start(strm, fd, cb, EPOLLIN);
}

struct fd_read_buffer {
  int fd;
  char *beg, *end;
  char buf[1024];
};

static void readline_cb(strm_stream *strm);

static char*
read_str(const char *beg, size_t len)
{
  char *p = malloc(len+1);

  memcpy(p, beg, len);
  p[len] = '\0';
  return p;
}

static void
read_cb(strm_stream *strm)
{
  struct fd_read_buffer *b = strm->data;
  size_t count;
  ssize_t n;

  count = sizeof(b->buf)-(b->end-b->buf);
  n = read(b->fd, b->end, count);
  if (n <= 0) {
    if (b->buf < b->end) {
      char *s = read_str(b->beg, b->end-b->beg);
      b->beg = b->end = b->buf;
      strm_emit(strm, s, NULL);
      io_kick(strm, b->fd);
    }
    else {
      strm_io_stop(strm, b->fd);
    }
    return;
  }
  b->end += n;
  strm->callback = readline_cb;
  (*readline_cb)(strm);
}

static void
readline_cb(strm_stream *strm)
{
  struct fd_read_buffer *b = strm->data;
  char *s;
  ssize_t len = b->end-b->beg;

  s = memchr(b->beg, '\n', len);
  if (s) {
    len = s - b->beg + 1;
  }
  else {                        /* no newline */
    if (len < sizeof(b->buf)) {
      memmove(b->buf, b->beg, len);
      b->beg = b->buf;
      b->end = b->beg + len;
    }
    strm->callback = read_cb;
    if (strm->flags & STRM_IO_NOWAIT) {
      strm_queue_put(io_queue, strm);
    }
    else {
      io_kick(strm, b->fd);
    }
    return;
  }
  s = read_str(b->beg, len);
  b->beg += len;
  strm_emit(strm, s, readline_cb);
}

static void
stdio_read(strm_stream *strm)
{
  struct fd_read_buffer *buf = strm->data;

  strm_io_start_read(strm, buf->fd, read_cb);
}

strm_stream*
strm_readio(int fd)
{
  struct fd_read_buffer *buf = malloc(sizeof(struct fd_read_buffer));

  buf->fd = fd;
  buf->beg = buf->end = buf->buf;
  return strm_alloc_stream(strm_task_prod, stdio_read, (void*)buf);
}

void
strm_io_start_write(strm_stream *strm, int fd, strm_func cb)
{
#if 0
  strm_io_start(strm, fd, cb, EPOLLOUT);
#else
  (*cb)(strm);
#endif
}

static void
write_cb(strm_stream *strm)
{
  const char *p = (char*)strm->cb_data;

  write((int)strm->data, p, strlen(p));
}

static void
stdio_write(strm_stream *strm)
{
  strm_io_start_write(strm, (int)strm->data, write_cb);
}

strm_stream*
strm_writeio(int fd)
{
  return strm_alloc_stream(strm_task_cons, stdio_write, (void*)fd);
}
