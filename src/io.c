#include "strm.h"
#include <pthread.h>
#include "pollfd.h"
#include <errno.h>
#ifndef _WIN32
# include <sys/socket.h>
#else
# include <ws2tcpip.h>
# include <fcntl.h>
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

#ifdef _WIN32
ssize_t
strm_read(int fd, void *buf, size_t count) {
  WSANETWORKEVENTS wev;
  if (WSAEnumNetworkEvents((SOCKET) fd, NULL, &wev) == 0)
    return recv(fd, buf, count, 0);
  return read(fd, buf, count);
}
#else
#define strm_read(fd,buf,n) read(fd,buf,n)
#endif

static struct strm_task*
io_task(strm_stream* strm, strm_callback func)
{
  return strm_task_new(func, strm_foreign_value(strm));
}

static void
io_task_add(struct strm_task* task)
{
  strm_stream* strm = strm_value_foreign(task->data);
  task->data = strm_nil_value();
  strm_task_add(strm, task);
}

static int
io_push(int fd, strm_stream* strm, strm_callback cb)
{
  struct epoll_event ev = { 0 };

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = io_task(strm, cb);
  return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int
io_kick(int fd, strm_stream* strm, strm_callback cb)
{
  struct epoll_event ev;

  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.ptr = io_task(strm, cb);
  return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

static int
io_pop(int fd)
{
  return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

static void*
io_loop(void* d)
{
  struct epoll_event events[MAX_EVENTS];
  int i, n;

  for (;;) {
    n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n < 0) {
      return NULL;
    }
    for (i=0; i<n; i++) {
      io_task_add(events[i].data.ptr);
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
strm_io_start_read(strm_stream* strm, int fd, strm_callback cb)
{
  if (io_push(fd, strm, cb) == 0) {
    io_wait_num++;
  }
}

static void
strm_io_stop(strm_stream* strm, int fd)
{
  if ((strm->flags & STRM_IO_NOWAIT) == 0) {
    io_wait_num--;
    io_pop(fd);
  }
  strm_stream_close(strm);
}

void
strm_io_emit(strm_stream* strm, strm_value data, int fd, strm_callback cb)
{
  strm_emit(strm, data, NULL);
  io_kick(fd, strm, cb);
}

struct fd_read_buffer {
  int fd;
  char *beg, *end;
  strm_io io;
#ifdef STRM_IO_MMAP
  char *buf;
  char fixed[BUFSIZ];
#else
  char buf[BUFSIZ];
#endif
};

static int readline_cb(strm_stream* strm, strm_value data);

static strm_value
read_str(const char* beg, strm_int len)
{
  char *p = malloc(len);

  memcpy(p, beg, len);
  return strm_str_value(strm_str_new(p, len));
}

static int
read_cb(strm_stream* strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;
  strm_int count;
  strm_int n;

  count = BUFSIZ-(b->end-b->buf);
  n = strm_read(b->fd, b->end, count);
  if (n <= 0) {
    if (b->buf < b->end) {
      strm_value s = read_str(b->beg, b->end-b->beg);
      b->beg = b->end = b->buf;
      strm_io_emit(strm, s, b->fd, read_cb);
    }
    else {
      strm_io_stop(strm, b->fd);
    }
    return STRM_OK;
  }
  b->end += n;
  (*readline_cb)(strm, strm_nil_value());
  return STRM_OK;
}

static int
readline_cb(strm_stream* strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;
  strm_value s;
  char *p;
  strm_int len = b->end-b->beg;

  p = memchr(b->beg, '\n', len);
  if (p) {
    len = p - b->beg;
  }
  /* no newline */
  else if (strm->flags & STRM_IO_NOFILL) {
    if (len <= 0) {
#ifdef STRM_IO_MMAP
      if (strm->flags & STRM_IO_MMAP) {
        munmap(b->buf, b->end - b->beg);
      }
#endif
      strm_io_stop(strm, b->fd);
      return STRM_OK;
    }
  }
  else {
    if (len < sizeof(b->buf)) {
      memmove(b->buf, b->beg, len);
      b->beg = b->buf;
      b->end = b->beg + len;
    }
    if (strm->flags & STRM_IO_NOWAIT) {
      strm_task_push(strm, read_cb, strm_nil_value());
    }
    else {
      io_kick(b->fd, strm, read_cb);
    }
    return STRM_OK;
  }
  s = read_str(b->beg, len);
  b->beg += len + 1;
  strm_emit(strm, s, readline_cb);
  return STRM_OK;
}

static int
stdio_read(strm_stream* strm, strm_value data)
{
  struct fd_read_buffer *b = strm->data;

  strm_io_start_read(strm, b->fd, read_cb);
  return STRM_OK;
}

static int
read_close(strm_stream* strm, strm_value d)
{
  struct fd_read_buffer *b = strm->data;

  close(b->fd);
  free(b);
  return STRM_OK;
}

static strm_stream*
strm_readio(strm_io io)
{
  strm_callback cb = stdio_read;
  unsigned int flags = 0;

  if (io->read_stream == NULL) {
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
    io->read_stream = strm_stream_new(strm_producer, cb, read_close, (void*)buf);
    io->read_stream->flags |= flags;
  }
  return io->read_stream;
}

struct write_data {
  FILE *f;
  strm_io io;
};

static int
write_cb(strm_stream* strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;
  strm_string p = strm_to_str(data);

  fwrite(strm_str_ptr(p), strm_str_len(p), 1, d->f);
  fputs("\n", d->f);
  if (d->io->mode & STRM_IO_FLUSH) {
    fflush(d->f);
  }
  return STRM_OK;
}

static int
write_close(strm_stream* strm, strm_value data)
{
  struct write_data *d = (struct write_data*)strm->data;

  /* tell peer we close the socket for writing (if it is) */
  shutdown(fileno(d->f), 1);
  /* if we have a reading strm, let it close the fd */
  if ((d->io->mode & STRM_IO_READING) == 0) {
    fclose(d->f);
  }
  free(d);
  return STRM_OK;
}

static strm_stream*
strm_writeio(strm_io io)
{
  struct write_data *d;

  if (io->write_stream) {
    d = (struct write_data*)io->write_stream->data;
  }
  else {
    d = malloc(sizeof(struct write_data));

#ifdef _WIN32
    WSANETWORKEVENTS wev;
    if (WSAEnumNetworkEvents((SOCKET) io->fd, NULL, &wev) == 0) {
      d->f = fdopen(_open_osfhandle(io->fd, _O_RDONLY), "w");
    } else
      d->f = fdopen(io->fd, "w");
#else
    d->f = fdopen(io->fd, "w");
#endif
    d->io = io;
    io->write_stream = strm_stream_new(strm_consumer, write_cb, write_close, (void*)d);
  }
  return io->write_stream;
}

strm_value
strm_io_new(int fd, int mode)
{
  strm_io io = malloc(sizeof(struct strm_io));

  io->fd = fd;
  io->mode = mode;
  io->type = STRM_PTR_IO;
  io->read_stream = io->write_stream = NULL;
  return strm_ptr_value(io);
}

strm_stream*
strm_io_stream(strm_value iov, int mode)
{
  strm_io io;

  assert(strm_io_p(iov));
  io = strm_value_io(iov);
  switch (mode) {
  case STRM_IO_READ:
    return strm_readio(io);
  case STRM_IO_WRITE:
    return strm_writeio(io);
 default:
   return NULL;
  }
}
