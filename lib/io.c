#include "strm.h"

void
strm_fd_read(strm_stream *strm, int fd, strm_func cb)
{
  strm->callback = cb;
  strm_io_enque(strm, fd);
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
  ssize_t n;

  n = read(b->fd, b->end, sizeof(b->buf)-(b->end-b->buf));
  if (n <= 0) {
    if (b->buf < b->end) {
      char *s = read_str(b->beg, b->end-b->beg);
      b->beg = b->end = b->buf;
      strm_emit(strm, s, read_cb);
    }
    return;
  }
  b->end += n;
  readline_cb(strm);
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
      strm_fd_read(strm, b->fd, read_cb);
      return;
    }
  }
  s = read_str(b->beg, len);
  b->beg += len;
  strm_emit(strm, s, readline_cb);
}

static void
stdio_read(strm_stream *strm)
{
  struct fd_read_buffer *buf = strm->data;

  strm_fd_read(strm, buf->fd, read_cb);
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
strm_fd_write(strm_stream *strm, int fd, strm_func cb)
{
  (*cb)(strm);
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
  strm_fd_write(strm, (int)strm->data, write_cb);
}

strm_stream*
strm_writeio(int fd)
{
  return strm_alloc_stream(strm_task_cons, stdio_write, (void*)fd);
}
