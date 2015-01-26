#ifndef _STRM_H_
#define _STRM_H_
#include <stdlib.h>
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

typedef enum {
  STRM_NODE_ARGS,
  STRM_NODE_PAIR,
  STRM_NODE_VALUE,
  STRM_NODE_BLOCK,
  STRM_NODE_IDENT,
  STRM_NODE_LET,
  STRM_NODE_IF,
  STRM_NODE_EMIT,
  STRM_NODE_RETURN,
  STRM_NODE_BREAK,
  STRM_NODE_VAR,
  STRM_NODE_CONST,
  STRM_NODE_OP,
  STRM_NODE_FUNC,
  STRM_NODE_CALL,
} strm_node_type;

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
  strm_node_type type;
  strm_value value;
} strm_node;

typedef struct {
  strm_node* key;
  strm_node* value;
} strm_pair;

typedef struct {
  int len;
  int max;
  strm_node** data;
} strm_array;

typedef struct {
  strm_node* cond;
  strm_node* compstmt;
  strm_node* opt_else;
} strm_node_if;

typedef struct {
  strm_node* lhs;
  strm_node* rhs;
} strm_node_let;

typedef struct {
  strm_node* lhs;
  char* op;
  strm_node* rhs;
} strm_node_op;

typedef struct {
  strm_node* ident;
  strm_node* args;
  strm_node* blk;
} strm_node_func;

typedef struct {
  strm_node* cond;
  strm_node* ident;
  strm_node* args;
} strm_node_call;

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
} parser_state;

extern strm_node* node_value_new(strm_node*);
extern strm_node* node_array_new();
extern strm_node* node_array_of(strm_node*);
extern void node_array_add(strm_node*, strm_node*);
extern strm_node* node_pair_new(strm_node*, strm_node*);
extern strm_node* node_map_new();
extern strm_node* node_map_of(strm_node*);
extern strm_node* node_let_new(strm_node*, strm_node*);
extern strm_node* node_op_new(char*, strm_node*, strm_node*);
extern strm_node* node_func_new(strm_node*, strm_node*, strm_node*);
extern strm_node* node_call_new(strm_node*, strm_node*, strm_node*);
extern strm_node* node_double_new(strm_double);
extern strm_node* node_string_new(strm_string);
extern strm_node* node_string_len_new(strm_string, size_t);
extern strm_node* node_if_new(strm_node*, strm_node*, strm_node*);
extern strm_node* node_emit_new(strm_node*);
extern strm_node* node_return_new(strm_node*);
extern strm_node* node_break_new();
extern strm_node* node_ident_new(strm_id);
extern strm_id node_ident_of(char*);
extern strm_node* node_nil();
extern strm_node* node_true();
extern strm_node* node_false();
#endif /* _STRM_H_ */
