#ifndef _STRM_H_
#define _STRM_H_
#include <stdio.h>
#include <stdint.h>
#include "khash.h"

typedef enum {
  STRM_VALUE_BOOL,
  STRM_VALUE_ARRAY,
  STRM_VALUE_MAP,
  STRM_VALUE_STRING,
  STRM_VALUE_DOUBLE,
  STRM_VALUE_FIXNUM,
  STRM_VALUE_NIL,
  STRM_VALUE_CFUNC,
  STRM_VALUE_USER,
} strm_value_type;

typedef intptr_t strm_id;
typedef double strm_double;
typedef char* strm_string;

typedef struct {
  strm_value_type t;
  union {
    int b;
    int i;
    double d;
    char* s;
    void* p;
    strm_id id;
  } v;
} strm_value;

typedef struct {
  int len;
  int max;
  void** data;
} strm_array;

KHASH_MAP_INIT_STR(value, strm_value*)

typedef khash_t(value) strm_env;

typedef struct {
  khash_t(value)* env;
  strm_value* exc;
} strm_ctx;

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
  /* TODO: should be separated as another context structure */
  strm_ctx ctx;
} parser_state;

typedef strm_value* (*strm_cfunc)(strm_ctx*, strm_array*);

int strm_parse_init(parser_state*);
void strm_parse_free(parser_state*);
int strm_parse_file(parser_state*, const char*);
int strm_parse_input(parser_state*, FILE* in, const char*);
int strm_parse_string(parser_state*, const char*);
int strm_run(parser_state*);
void strm_raise(strm_ctx*, const char*);

#endif /* _STRM_H_ */
