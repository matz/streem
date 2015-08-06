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
#define STRM_OK 0
#define STRM_NG 1

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
int strm_nil_p(strm_value);
int strm_num_p(strm_value);
int strm_io_p(strm_value);
int strm_task_p(strm_value);
int strm_cfunc_p(strm_value);
int strm_lambda_p(strm_value);
int strm_array_p(strm_value);

enum strm_obj_type {
  STRM_OBJ_ARRAY,
  STRM_OBJ_STRING,
  STRM_OBJ_LAMBDA,
  STRM_OBJ_IO,
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
strm_string *strm_str_intern_str(strm_string *s);
int strm_str_eq(strm_string *a, strm_string *b);
int strm_str_p(strm_value v);

strm_string *strm_to_str(strm_value v);
/* ----- Arrays */
typedef struct strm_array {
  STRM_OBJ_HEADER;
  size_t len;
  const strm_value *ptr;
  struct strm_array *headers;
} strm_array;

strm_array *strm_ary_new(const strm_value*,size_t len);
#define strm_ary_value(p,len) strm_ptr_value(strm_ary_new(p,len))
#define strm_value_ary(v) (strm_array*)strm_value_obj(v, STRM_OBJ_ARRAY)

int strm_ary_eq(strm_array *a, strm_array *b);

/* ----- Tasks */
typedef enum {
  strm_task_prod,               /* Producer */
  strm_task_filt,               /* Filter */
  strm_task_cons,               /* Consumer */
} strm_task_mode;

typedef struct strm_task strm_task;
typedef int (*strm_callback)(strm_task*, strm_value);
typedef strm_value (*strm_map_func)(strm_task*, strm_value);

struct strm_task {
  int tid;
  strm_task_mode mode;
  unsigned int flags;
  strm_callback start_func;
  strm_callback close_func;
  void *data;
  strm_task *dst;
  strm_task *nextd;
};

strm_task* strm_task_new(strm_task_mode mode, strm_callback start, strm_callback close, void *data);
void strm_emit(strm_task* task, strm_value data, strm_callback cb);
void strm_io_emit(strm_task* task, strm_value data, int fd, strm_callback cb);
int strm_task_connect(strm_task* src, strm_task* dst);
int strm_loop();
void strm_task_close(strm_task* strm);

extern int strm_event_loop_started;
strm_task* strm_value_task(strm_value);

/* ----- queue */
typedef struct strm_queue strm_queue;
struct strm_queue_task {
  strm_task *strm;
  strm_callback func;
  strm_value data;
  struct strm_queue_task *next;
};

strm_queue* strm_queue_alloc(void);
struct strm_queue_task* strm_queue_task(strm_task* strm, strm_callback func, strm_value data);
void strm_queue_free(strm_queue* q);
void strm_queue_push(strm_queue* q, struct strm_queue_task* t);
int strm_queue_exec(strm_queue* q);
int strm_queue_size(strm_queue* q);
int strm_queue_p(strm_queue* q);

void strm_task_push(struct strm_queue_task* t);

/* ----- Variables */
struct node_error;
typedef struct strm_state {
  struct node_error* exc;
  void *env;
  struct strm_state *prev;
  strm_task *task;
} strm_state;
int strm_var_set(strm_state*, strm_string*, strm_value);
int strm_var_def(const char*, strm_value);
int strm_var_get(strm_state*, strm_string*, strm_value*);

/* ----- I/O */
#define STRM_IO_READ  1
#define STRM_IO_WRITE 2
#define STRM_IO_FLUSH 4
#define STRM_IO_READING 8

typedef struct strm_io {
  STRM_OBJ_HEADER;
  int fd;
  int mode;
  strm_task *read_task, *write_task;
} strm_io;

strm_io* strm_io_new(int fd, int mode);
strm_task* strm_io_open(strm_io* io, int mode);
void strm_io_start_read(strm_task* strm, int fd, strm_callback cb);
#define strm_value_io(v) (strm_io*)strm_value_obj(v, STRM_OBJ_IO)

/* ----- lambda */
typedef struct strm_lambda {
  STRM_OBJ_HEADER;
  struct node_lambda* body;
  struct strm_state* state;
} strm_lambda;

#define strm_value_lambda(v) (strm_lambda*)strm_value_obj(v, STRM_OBJ_LAMBDA);
#define strm_value_array(v) (strm_array*)strm_value_obj(v, STRM_OBJ_ARRAY);
#endif
