#ifndef STRM_NODE_H
#define STRM_NODE_H

typedef enum {
  NODE_VALUE_BOOL,
  NODE_VALUE_STRING,
  NODE_VALUE_DOUBLE,
  NODE_VALUE_INT,
  NODE_VALUE_IDENT,
  NODE_VALUE_NIL,
  NODE_VALUE_USER,
  NODE_VALUE_ERROR,
} node_value_type;

typedef struct {
  node_value_type t;
  union {
    int b;
    long i;
    double d;
    void* p;
    strm_string s;
  } v;
} node_value;

typedef struct node_error {
  int type;
  strm_value arg;
} node_error;

typedef struct parser_state {
  int nerr;
  void *lval;
  const char *fname;
  int lineno;
  int tline;
  /* TODO: should be separated as another context structure */
} parser_state;

void node_parse_init(parser_state*);
void node_parse_free(parser_state*);
int node_parse_file(parser_state*, const char*);
int node_parse_input(parser_state*, FILE* in, const char*);
int node_parse_string(parser_state*, const char*);
int node_run(parser_state*);
void node_raise(strm_state*, const char*);

typedef enum {
  NODE_ARGS,
  NODE_PAIR,
  NODE_VALUE,
  NODE_CFUNC,
  NODE_LAMBDA,
  NODE_IDENT,
  NODE_LET,
  NODE_IF,
  NODE_EMIT,
  NODE_SKIP,
  NODE_RETURN,
  NODE_BREAK,
  NODE_VAR,
  NODE_CONST,
  NODE_OP,
  NODE_CALL,
  NODE_ARRAY,
  NODE_STMTS,
  NODE_NS,
  NODE_IMPORT,
} node_type;

#define NODE_HEADER node_type type

typedef struct {
  NODE_HEADER;
  node_value value;
} node;

typedef struct {
  NODE_HEADER;
  strm_string key;
  node* value;
} node_pair;

typedef struct {
  NODE_HEADER;
  int len;
  int max;
  void** data;
  strm_array* headers;
  strm_string ns;
} node_values;

typedef struct {
  NODE_HEADER;
  node* cond;
  node* then;
  node* opt_else;
} node_if;

typedef struct {
  NODE_HEADER;
  strm_string lhs;
  node* rhs;
} node_let;

typedef struct {
  NODE_HEADER;
  strm_string op;
  node* lhs;
  node* rhs;
} node_op;

typedef struct node_lambda {
  NODE_HEADER;
  node* args;
  node* compstmt;
} node_lambda;

typedef struct {
  NODE_HEADER;
  strm_string ident;
  node* args;
} node_call;

typedef struct {
  NODE_HEADER;
  node* rv;
} node_return;

typedef struct {
  NODE_HEADER;
  strm_string name;
  node* body;
} node_ns;

typedef struct {
  NODE_HEADER;
  strm_string name;
} node_import;

extern node* node_array_new();
extern node* node_array_headers(node*);
extern void node_array_add(node*, node*);
extern void node_array_free(node*);
extern node* node_stmts_new();
extern void node_stmts_add(node*, node*);
extern node* node_stmts_concat(node*, node*);
extern node* node_pair_new(strm_string, node*);
extern node* node_args_new();
extern void node_args_add(node*, strm_string);
extern node* node_ns_new(strm_string, node*);
extern node* node_import_new(strm_string);
extern node* node_let_new(strm_string, node*);
extern node* node_op_new(const char*, node*, node*);
extern node* node_obj_new(node*, strm_string);
extern node* node_lambda_new(node*, node*);
extern node* node_method_new(node*, node*);
extern node* node_call_new(strm_string, node*, node*, node*);
extern node* node_int_new(long);
extern node* node_double_new(double);
extern node* node_string_new(const char*, size_t);
extern node* node_if_new(node*, node*, node*);
extern node* node_emit_new(node*);
extern node* node_skip_new();
extern node* node_return_new(node*);
extern node* node_break_new();
extern node* node_ident_new(strm_string);
extern strm_string node_id(const char*, size_t len);
extern strm_string node_id_str(strm_string);
extern strm_string node_id_escaped(const char* s, size_t len);
extern node* node_nil();
extern node* node_true();
extern node* node_false();
extern void node_free(node*);

#endif /* STRM_NODE_H */
