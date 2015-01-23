#ifndef _STRM_H_
#define _STRM_H_
#include <stdlib.h>

typedef enum {
  STRM_VALUE_BOOL,
  STRM_VALUE_ARRAY,
  STRM_VALUE_MAP,
  STRM_VALUE_STRING,
  STRM_VALUE_DOUBLE,
  STRM_VALUE_FIXNUM,
} strm_value_type;

typedef enum {
  STRM_NODE_STMTS,
  STRM_NODE_STMT,
  STRM_NODE_EXPR,
  STRM_NODE_BLOCK,
  STRM_NODE_IDENT,
  STRM_NODE_LET,
  STRM_NODE_IF,
  STRM_NODE_ELSE,
  STRM_NODE_ELSEIF,
  STRM_NODE_VAR,
  STRM_NODE_CONST,
  STRM_NODE_OP_PLUS,
  STRM_NODE_VALUE,
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

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
} parser_state;

extern strm_node* strm_value_new(strm_node* v);
extern strm_node* strm_array_new();
extern strm_node* strm_array_add(strm_node*, strm_node*);
extern strm_node* strm_stmt_new();
extern strm_node* strm_let_new(strm_node*, strm_node*);
extern strm_node* strm_op_new(const char*, strm_node*, strm_node*);
extern strm_node* strm_funcall_new(strm_id, strm_node*, strm_node*);
extern strm_node* strm_nil_value();
extern strm_node* strm_double_new(strm_double);
extern strm_node* strm_string_new(strm_string);
extern strm_node* strm_string_len_new(strm_string, size_t);
extern strm_node* strm_ident_new(strm_id);
#endif /* _STRM_H_ */
