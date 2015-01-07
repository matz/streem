#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* ----- core */
typedef enum {
  strm_task_prod,               /* Producer */
  strm_task_filt,               /* Filter */
  strm_task_cons,               /* Consumer */
} strm_task_mode;

typedef struct strm_stream strm_stream;
typedef void (*strm_func)(strm_stream*);
typedef void*(*strm_map_func)(strm_stream*, void*);

#define STRM_IO_NOWAIT 1

struct strm_stream {
  strm_task_mode mode;
  uint flags;
  strm_func start_func;
  strm_func callback;
  void *cb_data;
  void *data;
  strm_stream *dst;
  strm_stream *nextd;
  strm_stream *nextq;
};

strm_stream* strm_alloc_stream(strm_task_mode mode, strm_func start, void *data);
void strm_emit(strm_stream *strm, void *data, strm_func cb);
int strm_connect(strm_stream *src, strm_stream *dst);
int strm_loop();

void strm_task_enque(strm_stream *s);

/* ----- I/O */
void strm_io_start_read(strm_stream *strm, int fd, strm_func cb);
void strm_io_start_write(strm_stream *strm, int fd, strm_func cb);
void strm_io_stop(strm_stream *strm, int fd);
strm_stream* strm_readio(int fd);
strm_stream* strm_writeio(int fd);
