#ifndef _STRM_H_
#define _STRM_H_
#include <stdio.h>
#include <stdint.h>

typedef enum {
  STRM_VALUE_BOOL,
  STRM_VALUE_ARRAY,
  STRM_VALUE_MAP,
  STRM_VALUE_STRING,
  STRM_VALUE_DOUBLE,
  STRM_VALUE_FIXNUM,
  STRM_VALUE_NIL,
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

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
} parser_state;

int strm_parse_init(parser_state*);
int strm_parse_file(parser_state*, const char*);
int strm_parse_input(parser_state*, FILE* in, const char*);
int strm_parse_string(parser_state*, const char*);

#endif /* _STRM_H_ */
