#include "strm.h"
#include <pthread.h>
#include "pollfd.h"
#include <errno.h>
#ifndef _WIN32
# include <sys/socket.h>
#else
# include <ws2tcpip.h>
#endif
#include <sys/stat.h>

#ifdef STRM_USE_WRITEV
#include <sys/uio.h>
#endif

static pthread_t io_worker;
static int io_wait_num = 0;
static int epoll_fd;

#define STRM_IO_NOWAIT 1
#define STRM_IO_NOFILL 2

#define STRM_IO_MMAP   4
/* should undef STRM_IO_MMAP on platform without mmap(2) */
#ifdef _WIN32
# undef STRM_IO_MMAP
#endif
#ifdef STRM_IO_MMAP
#include <sys/mman.h>
#endif

#define MAX_EVENTS 10

int
strm_io_waiting()
{
  return (io_wait_num > 0);
}

static int
io_push(int fd, strm_task* task, strm_callback cb)
{
  struct epoll_event ev = { 0 };

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(task, cb, strm_nil_value());
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int
io_kick(int fd, strm_task* task, strm_callback cb)
{
  struct epoll_event ev;

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = strm_queue_task(task, cb, strm_nil_value());
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

void
strm_io_start_read(strm_task* task, int fd, strm_callback cb)
{
  if (io_push(fd, task, cb) == 0) {
    io_wait_num++;
  }
}

static void
strm_io_stop(strm_task* task, int fd)
{
  if ((task->flags & STRM_IO_NOWAIT) == 0) {
    io_wait_num--;
    io_pop(fd);
  }
  strm_task_close(task);
}

struct fd_read_buffer {
  int fd;
  char *beg, *end;
  strm_io* io;
#ifdef STRM_IO_MMAP
  char *buf;
  char fixed[BUFSIZ];
#else
  char buf[BUFSIZ];
#endif
};

static void readline_cb(strm_task* task, strm_value data);

static strm_value
read_str(const char *beg, size_t len)
{
  char *p = malloc(len);

  memcpy(p, beg, len);
  return strm_str_value(p, len);
}

static void
read_cb(strm_task* task, strm_value data)
{
  struct fd_read_buffer *b = task->data;
  size_t count;
  ssize_t n;

  count = BUFSIZ-(b->end-b->buf);
  n = read(b->fd, b->end, count);
  if (n <= 0) {
    if (b->buf < b->end) {
      strm_value s = read_str(b->beg, b->end-b->beg);
      b->beg = b->end = b->buf;
      strm_emit(task, s, NULL);
      io_kick(b->fd, task, read_cb);
    }
    else {
      strm_io_stop(task, b->fd);
    }
    return;
  }
  b->end += n;
  (*readline_cb)(task, strm_nil_value());
}

static void
readline_cb(strm_task* task, strm_value data)
{
  struct fd_read_buffer *b = task->data;
  strm_value s;
  char *p;
  ssize_t len = b->end-b->beg;

  p = memchr(b->beg, '\n', len);
  if (p) {
    len = p - b->beg;
  }
  /* no newline */
  else if (task->flags & STRM_IO_NOFILL) {
    if (len <= 0) {
#ifdef STRM_IO_MMAP
      if (task->flags & STRM_IO_MMAP) {
        munmap(b->buf, b->end - b->beg);
      }
#endif
      strm_io_stop(task, b->fd);
      return;
    }
  }
  else {
    if (len < sizeof(b->buf)) {
      memmove(b->buf, b->beg, len);
      b->beg = b->buf;
      b->end = b->beg + len;
    }
    if (task->flags & STRM_IO_NOWAIT) {
      strm_task_push(strm_queue_task(task, read_cb, strm_nil_value()));
    }
    else {
      io_kick(b->fd, task, read_cb);
    }
    return;
  }
  s = read_str(b->beg, len);
  b->beg += len + 1;
  strm_emit(task, s, readline_cb);
}

static void
stdio_read(strm_task* task, strm_value data)
{
  struct fd_read_buffer *b = task->data;

  strm_io_start_read(task, b->fd, read_cb);
}

static void
read_close(strm_task* task, strm_value d)
{
  struct fd_read_buffer *b = task->data;

  close(b->fd);
}

static strm_task*
strm_readio(strm_io* io)
{
  strm_callback cb = stdio_read;
  unsigned int flags = 0;

  if (io->read_task == NULL) {
    struct fd_read_buffer *buf = malloc(sizeof(struct fd_read_buffer));
    struct stat st;

    io->mode |= STRM_IO_READING;
    buf->fd = io->fd;
    buf->io = io;
#ifdef STRM_IO_MMAP    
    buf->buf = buf->fixed;
#endif

    if (fstat(io->fd, &st) == 0 && (st.st_mode & S_IFMT) == S_IFREG) {
      /* fd must be a regular file */
      /* try mmap if STRM_IO_MMAP is defined */
      flags |= STRM_IO_NOWAIT;
#ifdef STRM_IO_MMAP
      buf->buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, buf->fd, 0);
      if (buf->buf != MAP_FAILED) {
        buf->beg = buf->buf;
        buf->end = buf->buf + st.st_size;
        flags |= STRM_IO_NOFILL;
        /* enqueue task without waiting */
        cb = readline_cb;
      }
#endif
    }
    else {
      buf->beg = buf->end = buf->buf;
    }
    io->read_task = strm_task_new(strm_task_prod, cb, read_close, (void*)buf);
    io->read_task->flags |= flags;
  }
  return io->read_task;
}

struct write_data {
  FILE *f;
  strm_io* io;
};

static void
write_cb(strm_task* task, strm_value data)
{
  struct write_data *d = (struct write_data*)task->data;
  strm_string *p = strm_to_str(data);

  fwrite(p->ptr, p->len, 1, d->f);
  fputs("\n", d->f);
  if (d->io->mode & STRM_IO_FLUSH) {
    fflush(d->f);
  }
}

static void
write_close(strm_task* task, strm_value data)
{
  struct write_data *d = (struct write_data*)task->data;

  /* tell peer we close the socket for writing (if it is) */
  shutdown(fileno(d->f), 1);
  /* if we have a reading task, let it close the fd */
  if ((d->io->mode & STRM_IO_READING) == 0) {
    fclose(d->f);
  }
  free(d);
}

static strm_task*
strm_writeio(strm_io* io)
{
  if (io->write_task == NULL) {
    struct write_data *d = malloc(sizeof(struct write_data));

    d->f = fdopen(io->fd, "w");
    d->io = io;
    io->write_task = strm_task_new(strm_task_cons, write_cb, write_close, (void*)d);
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
