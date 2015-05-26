#include "strm.h"
#include <pthread.h>
#include "pollfd.h"
#include <errno.h>
#include <sys/socket.h>

#ifdef STRM_USE_WRITEV
#include <sys/uio.h>
#endif

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
io_push(int fd, strm_task *strm, strm_func cb)
{
  struct epoll_event ev = { 0 };

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(strm, cb, strm_nil_value());
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int
io_kick(int fd, strm_task *strm, strm_func cb)
{
  struct epoll_event ev;

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(strm, cb, strm_nil_value());
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
      strm_task_push(t);
    }
  }
  return NULL;
}

void
strm_init_io_loop()
{
  epoll_fd = epoll_create(10);
  assert(epoll_fd >= 0);
  pthread_create(&io_worker, NULL, io_loop, NULL);
}

static void
strm_io_start(strm_task *strm, int fd, strm_func cb, uint32_t events)
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
      strm_task_push(strm_queue_task(strm, cb, strm_nil_value()));
    }
  }
}

static void
strm_io_stop(strm_task *strm, int fd)
{
  if ((strm->flags & STRM_IO_NOWAIT) == 0) {
    io_wait_num--;
    io_pop(fd);
  }
  strm_close(strm);
}

void
strm_io_start_read(strm_task *strm, int fd, strm_func cb)
{
  strm_io_start(strm, fd, cb, EPOLLIN);
}

struct fd_read_buffer {
  int fd;
  char *beg, *end;
  strm_io* io;
  char buf[1024];
};

static void readline_cb(strm_task *strm, strm_value data);

static strm_value
read_str(const char *beg, size_t len)
{
  char *p = malloc(len);

  memcpy(p, beg, len);
  return strm_str_value(p, len);
}

static void
read_cb(strm_task *strm, strm_value data)
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
  (*readline_cb)(strm, strm_nil_value());
}

static void
readline_cb(strm_task *strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;
  strm_value s;
  char *p;
  ssize_t len = b->end-b->beg;

  p = memchr(b->beg, '\n', len);
  if (p) {
    len = p - b->beg;
  }
  else {                        /* no newline */
    if (len < sizeof(b->buf)) {
      memmove(b->buf, b->beg, len);
      b->beg = b->buf;
      b->end = b->beg + len;
    }
    if (strm->flags & STRM_IO_NOWAIT) {
      strm_task_push(strm_queue_task(strm, read_cb, strm_nil_value()));
    }
    else {
      io_kick(b->fd, strm, read_cb);
    }
    return;
  }
  s = read_str(b->beg, len);
  b->beg += len + 1;
  strm_emit(strm, s, readline_cb);
}

static void
stdio_read(strm_task *strm, strm_value data)
{
  struct fd_read_buffer *buf = strm->data;

  strm_io_start_read(strm, buf->fd, read_cb);
}

static void
read_close(strm_task *strm, strm_value d)
{
  struct fd_read_buffer *b = strm->data;

  close(b->fd);
}

static strm_task*
strm_readio(strm_io* io)
{
  if (io->read_task == NULL) {
    struct fd_read_buffer *buf = malloc(sizeof(struct fd_read_buffer));

    io->mode |= STRM_IO_READING;
    buf->fd = io->fd;
    buf->io = io;
    buf->beg = buf->end = buf->buf;
    io->read_task = strm_alloc_stream(strm_task_prod, stdio_read, read_close, (void*)buf);
  }
  return io->read_task;
}

struct write_data {
  int fd;
  strm_io* io;
};

static void
write_cb(strm_task *strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;
  strm_string *p = strm_to_str(data);
#ifdef STRM_USE_WRITEV
  struct iovec v[2];

  v[0].iov_base = (char*)p->ptr;
  v[0].iov_len  = p->len;
  v[1].iov_base = "\n";
  v[1].iov_len  = 1;

  writev(d->fd, v, 2);
#else
  write(d->fd, p->ptr, p->len);
  write(d->fd, "\n", 1);
#endif
}

static void
write_close(strm_task *strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;

  /* tell peer we close the socket for writing (if it is) */
  shutdown(d->fd, 1);
  /* if we have a reading task, let it close the fd */
  if ((d->io->mode & STRM_IO_READING) == 0) {
    close(d->fd);
  }
  free(d);
}

static strm_task*
strm_writeio(strm_io* io)
{
  if (io->write_task == NULL) {
    struct write_data *d = malloc(sizeof(struct write_data));

    d->fd = io->fd;
    d->io = io;
    io->write_task = strm_alloc_stream(strm_task_cons, write_cb, write_close, (void*)d);
  }
  return io->write_task;
}

strm_io*
strm_io_new(int fd, int mode)
{
  strm_io *io = malloc(sizeof(strm_io));

  io->fd = fd;
  io->mode = mode;
  io->type = STRM_OBJ_IO;
  io->read_task = io->write_task = NULL;
  return io;
}

strm_task*
strm_io_open(strm_io *io, int mode)
{
  switch (mode) {
  case STRM_IO_READ:
    if (io->read_task) return io->read_task;
    if (io->mode & STRM_IO_READ) {
      return strm_readio(io);
    }
    break;
  case STRM_IO_WRITE:
    if (io->write_task) return io->read_task;
    if (io->mode & STRM_IO_WRITE) {
      return strm_writeio(io);
    }
    break;
 default:
   break;
  }
  return NULL;
}
