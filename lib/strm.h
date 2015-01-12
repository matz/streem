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
typedef void (*strm_func)(strm_stream*, void*);
typedef void*(*strm_map_func)(strm_stream*, void*);

#define STRM_IO_NOWAIT 1
#define STRM_IO_BFULL  2

struct strm_stream {
  strm_task_mode mode;
  unsigned int flags;
  strm_func start_func;
  strm_func close_func;
  void *data;
  strm_stream *dst;
  strm_stream *nextd;
};

strm_stream* strm_alloc_stream(strm_task_mode mode, strm_func start, strm_func close, void *data);
void strm_emit(strm_stream *strm, void *data, strm_func cb);
int strm_connect(strm_stream *src, strm_stream *dst);
int strm_loop();
void strm_close(strm_stream *strm);

void strm_task_push(strm_stream *s, strm_func func, void *data);

/* ----- queue */
typedef struct strm_queue strm_queue;
struct strm_queue_entry;

strm_queue* strm_queue_alloc(void);
void strm_queue_free(strm_queue *q);
void strm_queue_push(strm_queue *q, strm_stream *strm, strm_func func, void *data);
int strm_queue_exec(strm_queue *q);
int strm_queue_p(strm_queue *q);

/* ----- I/O */
void strm_io_start_read(strm_stream *strm, int fd, strm_func cb);
void strm_io_start_write(strm_stream *strm, int fd, strm_func cb);
void strm_io_stop(strm_stream *strm, int fd);
strm_stream* strm_readio(int fd);
strm_stream* strm_writeio(int fd);
