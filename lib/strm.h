#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#ifndef FALSE
# define FALSE 0
#elif FALSE
# error FALSE must be false
#endif
#ifndef TRUE
# define TRUE 1
#elif !TRUE
# error TRUE must be true
#endif

/* ----- Values */
enum strm_value_type {
  STRM_VALUE_BOOL,
  STRM_VALUE_INT,
  STRM_VALUE_FLT,
  STRM_VALUE_PTR,
};

typedef struct strm_value {
  enum strm_value_type type;
  union {
    long i;
    void *p;
    double f;
  } val;
} strm_value;

strm_value strm_ptr_value(void*);
strm_value strm_bool_value(int);
strm_value strm_int_value(long);
strm_value strm_flt_value(double);

#define strm_null_value() strm_ptr_value(NULL)

void *strm_value_ptr(strm_value);
long strm_value_int(strm_value);
int strm_value_bool(strm_value);
double strm_value_flt(strm_value);

enum strm_obj_type {
  STRM_OBJ_ARRAY,
  STRM_OBJ_MAP,
  STRM_OBJ_STRING,
  STRM_OBJ_CFUNC,
  STRM_OBJ_USER,
};

#define STRM_OBJ_HEADER \
  unsigned int flags; \
  enum strm_obj_type type

struct strm_object {
  STRM_OBJ_HEADER;
};

void *strm_value_obj(strm_value, enum strm_obj_type);

/* ----- Strings */
struct strm_string {
  STRM_OBJ_HEADER;
  const char *ptr;
  size_t len;
};

struct strm_string *strm_str_new(const char*,size_t len);
#define strm_str_value(p,len) strm_ptr_value(strm_str_new(p,len))
#define strm_value_str(v) (struct strm_string*)strm_value_obj(v, STRM_OBJ_STRING)

/* ----- Tasks */
typedef enum {
  strm_task_prod,               /* Producer */
  strm_task_filt,               /* Filter */
  strm_task_cons,               /* Consumer */
} strm_task_mode;

typedef struct strm_stream strm_stream;
typedef void (*strm_func)(strm_stream*, strm_value);
typedef strm_value (*strm_map_func)(strm_stream*, strm_value);

#define STRM_IO_NOWAIT 1
#define STRM_IO_BFULL  2

struct strm_stream {
  int tid;
  strm_task_mode mode;
  unsigned int flags;
  strm_func start_func;
  strm_func close_func;
  void *data;
  strm_stream *dst;
  strm_stream *nextd;
};

strm_stream* strm_alloc_stream(strm_task_mode mode, strm_func start, strm_func close, void *data);
void strm_emit(strm_stream *strm, strm_value data, strm_func cb);
int strm_connect(strm_stream *src, strm_stream *dst);
int strm_loop();
void strm_close(strm_stream *strm);

/* ----- queue */
typedef struct strm_queue strm_queue;
struct strm_queue_task {
  strm_stream *strm;
  strm_func func;
  strm_value data;
  struct strm_queue_task *next;
};

strm_queue* strm_queue_alloc(void);
struct strm_queue_task* strm_queue_task(strm_stream *strm, strm_func func, strm_value data);
void strm_queue_free(strm_queue *q);
void strm_queue_push(strm_queue *q, struct strm_queue_task *t);
int strm_queue_exec(strm_queue *q);
int strm_queue_size(strm_queue *q);
int strm_queue_p(strm_queue *q);

void strm_task_push(struct strm_queue_task *t);

/* ----- I/O */
void strm_io_start_read(strm_stream *strm, int fd, strm_func cb);
void strm_io_start_write(strm_stream *strm, int fd, strm_func cb);
void strm_io_stop(strm_stream *strm, int fd);
strm_stream* strm_readio(int fd);
strm_stream* strm_writeio(int fd);
