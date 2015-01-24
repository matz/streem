#ifndef _STRM_H_
#define _STRM_H_
#include <stdlib.h>

typedef enum {
  STRM_VALUE_BOOL,
  STRM_VALUE_CARRAY,
  STRM_VALUE_ARRAY,
  STRM_VALUE_MAP,
  STRM_VALUE_STRING,
  STRM_VALUE_DOUBLE,
  STRM_VALUE_FIXNUM,
  STRM_VALUE_NIL,
} strm_value_type;

typedef enum {
  STRM_NODE_ARRAY,
  STRM_NODE_VALUE,
  STRM_NODE_BLOCK,
  STRM_NODE_IDENT,
  STRM_NODE_LET,
  STRM_NODE_IF,
  STRM_NODE_ELSE,
  STRM_NODE_ELSEIF,
  STRM_NODE_VAR,
  STRM_NODE_CONST,
  STRM_NODE_OP_PLUS,
  STRM_NODE_OP_MINUS,
} strm_node_type;

typedef unsigned int strm_id;
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
  strm_node_type type;
  strm_value value;
} strm_node;

typedef struct {
  int len;
  int max;
  strm_node** data;
} strm_array;

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
} parser_state;

extern strm_node* node_value_new(strm_node*);
extern strm_node* node_array_new();
extern void node_array_add(strm_node*, strm_node*);
extern strm_node* node_let_new(strm_node*, strm_node*);
extern strm_node* node_op_new(const char*, strm_node*, strm_node*);
extern strm_node* node_funcall_new(strm_id, strm_node*, strm_node*);
extern strm_node* node_double_new(strm_double);
extern strm_node* node_string_new(strm_string);
extern strm_node* node_string_len_new(strm_string, size_t);
extern strm_node* node_ident_new(strm_id);
extern strm_id node_ident_of(const char*);
#endif /* _STRM_H_ */
