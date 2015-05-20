#ifndef _STRM_H_
#define _STRM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

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
  STRM_VALUE_CFUNC,
  STRM_VALUE_BLK,
  STRM_VALUE_TASK,
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
strm_value strm_cfunc_value(void*);
strm_value strm_blk_value(void*);
strm_value strm_task_value(void*);
strm_value strm_bool_value(int);
strm_value strm_int_value(long);
strm_value strm_flt_value(double);

#define strm_nil_value() strm_ptr_value(NULL)

void *strm_value_ptr(strm_value);
long strm_value_int(strm_value);
int strm_value_bool(strm_value);
double strm_value_flt(strm_value);

int strm_value_eq(strm_value, strm_value);
int strm_num_p(strm_value);
int strm_int_p(strm_value);
int strm_flt_p(strm_value);
int strm_task_p(strm_value);
int strm_cfunc_p(strm_value);
int strm_blk_p(strm_value);

enum strm_obj_type {
  STRM_OBJ_ARRAY,
  STRM_OBJ_LIST,
  STRM_OBJ_MAP,
  STRM_OBJ_STRING,
  STRM_OBJ_USER,
};

#define STRM_OBJ_HEADER \
  unsigned int flags; \
  enum strm_obj_type type

struct strm_object {
  STRM_OBJ_HEADER;
};

int strm_obj_eq(struct strm_object*, struct strm_object *);

void *strm_value_obj(strm_value, enum strm_obj_type);

/* ----- Strings */
typedef struct strm_string {
  STRM_OBJ_HEADER;
  const char *ptr;
  size_t len;
} strm_string;

#define STRM_STR_INTERNED 1

strm_string *strm_str_new(const char*,size_t len);
#define strm_str_value(p,len) strm_ptr_value(strm_str_new(p,len))
#define strm_value_str(v) (strm_string*)strm_value_obj(v, STRM_OBJ_STRING)

strm_string *strm_str_intern(const char *p, size_t len);
int strm_str_eq(strm_string *a, strm_string *b);
int strm_str_p(strm_value v);

strm_string *strm_to_str(strm_value v);
/* ----- Variables */
void strm_var_set(strm_string*, strm_value);
void strm_var_def(const char*, strm_value);
strm_value strm_var_get(strm_string*);
/* ----- Arrays */
typedef struct strm_array {
  STRM_OBJ_HEADER;
  size_t len;
  const strm_value *ptr;
} strm_array;

strm_array *strm_ary_new(const strm_value*,size_t len);
#define strm_ary_value(p,len) strm_ptr_value(strm_ary_new(p,len))
#define strm_value_ary(v) (strm_array*)strm_value_obj(v, STRM_OBJ_ARRAY)

int strm_ary_eq(strm_array *a, strm_array *b);

/* ----- Lists */
typedef struct strm_list {
  STRM_OBJ_HEADER;
  size_t len;
  strm_value car;
  struct strm_list *cdr;
} strm_list;

strm_list *strm_list_new(strm_value car, strm_list *cdr);
#define strm_value_list(v) (strm_list*)strm_value_obj(v, STRM_OBJ_LIST)

int strm_list_eq(strm_list *a, strm_list *b);
strm_value strm_list_nth(strm_list *a, size_t n);

/* ----- Tasks */
typedef enum {
  strm_task_prod,               /* Producer */
  strm_task_filt,               /* Filter */
  strm_task_cons,               /* Consumer */
} strm_task_mode;

typedef struct strm_task strm_task;
typedef void (*strm_func)(strm_task*, strm_value);
typedef strm_value (*strm_map_func)(strm_task*, strm_value);

#define STRM_IO_NOWAIT 1
#define STRM_IO_BFULL  2

struct strm_task {
  int tid;
  strm_task_mode mode;
  unsigned int flags;
  strm_func start_func;
  strm_func close_func;
  void *data;
  strm_task *dst;
  strm_task *nextd;
};

strm_task* strm_alloc_stream(strm_task_mode mode, strm_func start, strm_func close, void *data);
void strm_emit(strm_task *strm, strm_value data, strm_func cb);
int strm_connect(strm_task *src, strm_task *dst);
int strm_loop();
void strm_close(strm_task *strm);

extern int strm_event_loop_started;
strm_task* strm_value_task(strm_value);

/* ----- queue */
typedef struct strm_queue strm_queue;
struct strm_queue_task {
  strm_task *strm;
  strm_func func;
  strm_value data;
  struct strm_queue_task *next;
};

strm_queue* strm_queue_alloc(void);
struct strm_queue_task* strm_queue_task(strm_task *strm, strm_func func, strm_value data);
void strm_queue_free(strm_queue *q);
void strm_queue_push(strm_queue *q, struct strm_queue_task *t);
int strm_queue_exec(strm_queue *q);
int strm_queue_size(strm_queue *q);
int strm_queue_p(strm_queue *q);

void strm_task_push(struct strm_queue_task *t);

/* ----- I/O */
void strm_io_start_read(strm_task *strm, int fd, strm_func cb);
void strm_io_start_write(strm_task *strm, int fd, strm_func cb);
void strm_io_stop(strm_task *strm, int fd);
strm_task* strm_readio(int fd);
strm_task* strm_writeio(int fd);

#endif
