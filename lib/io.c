#include "strm.h"
#include <pthread.h>
#include "pollfd.h"
#include <errno.h>

static pthread_t io_worker;
static int io_wait_num = 0;
static int epoll_fd;

#define MAX_EVENTS 10

int
strm_io_waiting()
{
  return (io_wait_num > 0);
}

static int
io_push(int fd, strm_stream *strm, strm_func cb)
{
  struct epoll_event ev = { 0 };

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(strm, cb, strm_null_value());
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int
io_kick(int fd, strm_stream *strm, strm_func cb)
{
  struct epoll_event ev;

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(strm, cb, strm_null_value());
  return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

static int
io_pop(int fd)
{
  return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void strm_task_push_task(struct strm_queue_task *t);

static void*
io_loop(void *d)
{
  struct epoll_event events[MAX_EVENTS];
  int i, n;

  for (;;) {
    n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0) {
      return NULL;
    }
    for (i=0; i<n; i++) {
      struct strm_queue_task *t = events[i].data.ptr;
      strm_task_push_task(t);
    }
  }
  return NULL;
}

void
strm_init_io_loop()
{
  epoll_fd = epoll_create(10);
  pthread_create(&io_worker, NULL, io_loop, NULL);
}

static void
strm_io_start(strm_stream *strm, int fd, strm_func cb, uint32_t events)
{
  int n;

  n = io_push(fd, strm, cb);
  if (n == 0) {
    io_wait_num++;
  }
  else {
    if (errno == EPERM) {
      /* fd must be a regular file */
      /* enqueue task without waiting */
      strm->flags |= STRM_IO_NOWAIT;
      strm_task_push(strm, cb, strm_null_value());
    }
  }
}

void
strm_io_stop(strm_stream *strm, int fd)
{
  if ((strm->flags & STRM_IO_NOWAIT) == 0) {
    io_wait_num--;
    io_pop(fd);
  }
  strm_close(strm);
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

static void readline_cb(strm_stream *strm, strm_value data);

static strm_value
read_str(const char *beg, size_t len)
{
  char *p = malloc(len);

  memcpy(p, beg, len);
  return strm_str_value(p, len);
}

static void
read_cb(strm_stream *strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;
  size_t count;
  ssize_t n;

  count = sizeof(b->buf)-(b->end-b->buf);
  n = read(b->fd, b->end, count);
  if (n <= 0) {
    if (b->buf < b->end) {
      strm_value s = read_str(b->beg, b->end-b->beg);
      b->beg = b->end = b->buf;
      strm_emit(strm, s, NULL);
      io_kick(b->fd, strm, read_cb);
    }
    else {
      strm_io_stop(strm, b->fd);
    }
    return;
  }
  b->end += n;
  (*readline_cb)(strm, strm_null_value());
}

static void
readline_cb(strm_stream *strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;
  strm_value s;
  char *p;
  ssize_t len = b->end-b->beg;

  p = memchr(b->beg, '\n', len);
  if (p) {
    len = p - b->beg + 1;
  }
  else {                        /* no newline */
    if (len < sizeof(b->buf)) {
      memmove(b->buf, b->beg, len);
      b->beg = b->buf;
      b->end = b->beg + len;
    }
    if (strm->flags & STRM_IO_NOWAIT) {
      strm_task_push(strm, read_cb, strm_null_value());
    }
    else {
      io_kick(b->fd, strm, read_cb);
    }
    return;
  }
  s = read_str(b->beg, len);
  b->beg += len;
  strm_emit(strm, s, readline_cb);
}

static void
stdio_read(strm_stream *strm, strm_value data)
{
  struct fd_read_buffer *buf = strm->data;

  strm_io_start_read(strm, buf->fd, read_cb);
}

static void
read_close(strm_stream *strm, strm_value d)
{
  struct fd_read_buffer *b = strm->data;

  close(b->fd);
}

strm_stream*
strm_readio(int fd)
{
  struct fd_read_buffer *buf = malloc(sizeof(struct fd_read_buffer));

  buf->fd = fd;
  buf->beg = buf->end = buf->buf;
  return strm_alloc_stream(strm_task_prod, stdio_read, read_close, (void*)buf);
}

struct write_data {
  int fd;
};

static void
write_cb(strm_stream *strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;
  struct strm_string *p = strm_value_str(data);

  write(d->fd, p->ptr, p->len);
}

static void
write_close(strm_stream *strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;

  close(d->fd);
  free(d);
}

strm_stream*
strm_writeio(int fd)
{
  struct write_data *d = malloc(sizeof(struct write_data));

  d->fd = fd;
  return strm_alloc_stream(strm_task_cons, write_cb, write_close, (void*)d);
}
