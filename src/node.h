#ifndef _NODE_H_
#define _NODE_H_
#include "khash.h"

typedef enum {
  NODE_VALUE_BOOL,
  NODE_VALUE_ARRAY,
  NODE_VALUE_MAP,
  NODE_VALUE_STRING,
  NODE_VALUE_DOUBLE,
  NODE_VALUE_INT,
  NODE_VALUE_IDENT,
  NODE_VALUE_NIL,
  NODE_VALUE_CFUNC,
  NODE_VALUE_USER,
  NODE_VALUE_ERROR,
} node_value_type;

typedef intptr_t node_id;

typedef struct {
  node_value_type t;
  union {
    int b;
    int i;
    double d;
    char* s;
    void* p;
    node_id id;
  } v;
} node_value;

KHASH_MAP_INIT_STR(value, node_value*)

typedef khash_t(value) node_env;

typedef struct {
  int type;
  node_value* arg;
} node_error;

typedef struct {
  khash_t(value)* env;
  node_error* exc;
} node_ctx;

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
  /* TODO: should be separated as another context structure */
  node_ctx ctx;
} parser_state;

int node_parse_init(parser_state*);
void node_parse_free(parser_state*);
int node_parse_file(parser_state*, const char*);
int node_parse_input(parser_state*, FILE* in, const char*);
int node_parse_string(parser_state*, const char*);
int node_run(parser_state*);
void node_raise(node_ctx*, const char*);

typedef enum {
  NODE_ARGS,
  NODE_PAIR,
  NODE_VALUE,
  NODE_CFUNC,
  NODE_BLOCK,
  NODE_IDENT,
  NODE_LET,
  NODE_IF,
  NODE_EMIT,
  NODE_RETURN,
  NODE_BREAK,
  NODE_VAR,
  NODE_CONST,
  NODE_OP,
  NODE_CALL,
  NODE_ARRAY,
} node_type;

typedef struct {
  node_type type;
  node_value value;
} node;

typedef struct {
  node* key;
  node* value;
} node_pair;

typedef struct {
  int len;
  int max;
  void** data;
} node_values;

typedef struct {
  node* cond;
  node* compstmt;
  node* opt_else;
} node_if;

typedef struct {
  node* lhs;
  node* rhs;
} node_let;

typedef struct {
  node* lhs;
  char* op;
  node* rhs;
} node_op;

typedef struct {
  node* args;
  node* compstmt;
} node_block;

typedef struct {
  node* cond;
  node* ident;
  node* args;
  node* blk;
} node_call;

typedef struct {
  node* rv;
} node_return;

typedef node_value* (*node_cfunc)(node_ctx*, node_values*);

node_values* node_values_new();
void node_values_add(node_values*, void*);

extern node* node_value_new(node*);
extern node* node_array_new();
extern node* node_array_of(node*);
extern void node_array_add(node*, node*);
extern void node_array_free(node*);
extern node* node_pair_new(node*, node*);
extern node* node_map_new();
extern node* node_map_of(node*);
extern node* node_let_new(node*, node*);
extern node* node_op_new(char*, node*, node*);
extern node* node_block_new(node*, node*);
extern node* node_call_new(node*, node*, node*, node*);
extern node* node_double_new(double);
extern node* node_string_new(const char*, size_t);
extern node* node_if_new(node*, node*, node*);
extern node* node_emit_new(node*);
extern node* node_return_new(node*);
extern node* node_break_new();
extern node* node_ident_new(node_id);
extern node_id node_ident_of(const char*);
extern node* node_nil();
extern node* node_true();
extern node* node_false();
extern void node_free(node*);

#endif /* _NODE_H_ */

