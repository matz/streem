#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

extern FILE *yyin, *yyout;
extern int yyparse(parser_state*);
extern int yydebug;

static char*
strndup0(const char *s, size_t n)
{
  size_t i;
  const char *p = s;
  char *new;

  for (i=0; i<n && *p; i++,p++)
    ;
  new = (char*)malloc(i+1);
  if (new) {
    memcpy(new, s, i);
    new[i] = '\0';
  }
  return new;
}

node*
node_value_new(node* v)
{
  /* TODO */
  return NULL;
}

node_values*
node_values_new() {
  /* TODO: error check */
  node_values* v = malloc(sizeof(node_values));
  v->len = 0;
  v->max = 0;
  v->data = NULL;
  return v;
}

void
node_values_add(node_values* v, void* data) {
  if (v->len == v->max) {
    v->max = v->len + 10;
    v->data = realloc(v->data, sizeof(void*) * v->max);
  }
  /* TODO: error check */
  v->data[v->len] = data;
  v->len++;
}

node*
node_array_new()
{
  /* TODO: error check */
  node* np = malloc(sizeof(node));
  np->type = NODE_ARRAY;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = node_values_new();
  return np;
}

node*
node_array_of(node* np)
{
  if (np == NULL)
    np = node_array_new();
  return np;
}

void
node_array_add(node* arr, node* np)
{
  node_values* v = arr->value.v.p;
  node_values_add(v, np);
}

void
node_array_free(node* np)
{
  int i;
  node_values* v = np->value.v.p;
  for (i = 0; i < v->len; i++)
    node_free(v->data[i]);
  free(v);
  free(np);
}

node*
node_pair_new(node* key, node* value)
{
  node_pair* npair = malloc(sizeof(node_pair));
  npair->key = key;
  npair->value = value;

  node* np = malloc(sizeof(node));
  np->type = NODE_PAIR;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = npair;
  return np;
}

node*
node_map_new()
{
  node_values* v = malloc(sizeof(node_values));
  /* TODO: error check */
  v->len = 0;
  v->max = 0;
  v->data = NULL;

  node* np = malloc(sizeof(node));
  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_MAP;
  np->value.v.p = v;
  return np;
}

node*
node_map_of(node* np)
{
  if (np == NULL)
    np = node_map_new();
  return np;
}

void
node_map_free(node* np)
{
  int i;
  node_values* v = np->value.v.p;
  for (i = 0; i < v->len; i++) {
    node* pair = v->data[i];
    node_pair* npair = pair->value.v.p;
    node_free(npair->key);
    node_free(npair->value);
    free(npair);
    free(pair);
  }
  free(v);
  free(np);
}

node*
node_let_new(node* lhs, node* rhs)
{
  node_let* nlet = malloc(sizeof(node_let));
  nlet->lhs = lhs;
  nlet->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_LET;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = nlet;
  return np;
}

node*
node_op_new(char* op, node* lhs, node* rhs)
{
  node_op* nop = malloc(sizeof(node_op));
  nop->lhs = lhs;
  nop->op = op;
  nop->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_OP;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = nop;
  return np;
}

node*
node_block_new(node* args, node* compstmt)
{
  node_block* block = malloc(sizeof(node_block));
  block->args = args;
  block->compstmt = compstmt;

  node* np = malloc(sizeof(node));
  np->type = NODE_BLOCK;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = block;
  return np;
}

node*
node_call_new(node* cond, node* ident, node* args, node* blk)
{
  node_call* ncall = malloc(sizeof(node_call));
  ncall->cond = cond;
  ncall->ident = ident;
  ncall->args = args;
  ncall->blk = blk;

  node* np = malloc(sizeof(node));
  np->type = NODE_CALL;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = ncall;
  return np;
}

node*
node_int_new(long i)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_INT;
  np->value.v.i = i;
  return np;
}

node*
node_double_new(double d)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_DOUBLE;
  np->value.v.d = d;
  return np;
}

node*
node_string_new(const char* s, size_t l)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_STRING;
  np->value.v.s = strndup0(s, l);
  return np;
}

node*
node_ident_new(node_id id)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_IDENT;
  np->value.t = NODE_VALUE_IDENT;
  np->value.v.id = id;
  return np;
}

node_id
node_ident_of(const char* s)
{
  extern int strm_event_loop_started;
  struct strm_string *str;

  assert(!strm_event_loop_started);
  str = strm_str_intern(s, strlen(s));
  return (node_id)str;
}

node*
node_nil()
{
  static node nd = { NODE_VALUE, { NODE_VALUE_NIL, {0} } };
  return &nd;
}

node*
node_true()
{
  static node nd = { NODE_VALUE, { NODE_VALUE_BOOL, {1} } };
  return &nd;
}

node*
node_false()
{
  static node nd = { NODE_VALUE, { NODE_VALUE_BOOL, {0} } };
  return &nd;
}

node*
node_if_new(node* cond, node* compstmt, node* opt_else)
{
  node_if* nif = malloc(sizeof(node_if));
  nif->cond = cond;
  nif->compstmt = compstmt;
  nif->opt_else = opt_else;

  node* np = malloc(sizeof(node));
  np->type = NODE_IF;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = nif;
  return np;
}


node*
node_emit_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_EMIT;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = value;
  return np;
}

node*
node_return_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_RETURN;
  np->value.t = NODE_VALUE_USER;
  np->value.v.p = value;
  return np;
}

node*
node_break_new(node* value)
{
  static node nd = { NODE_BREAK };
  return &nd;
}

int
node_parse_init(parser_state *p)
{
  p->nerr = 0;
  p->lval = NULL;
  p->fname = NULL;
  p->lineno = 1;
  p->tline = 1;
  p->ctx.exc = NULL;
  return 0;
}

int
node_parse_file(parser_state* p, const char* fname)
{
  int r;
  FILE* fp = fopen(fname, "rb");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }
  r = node_parse_input(p, fp, fname);
  fclose(fp);
  return r;
}

int
node_parse_input(parser_state* p, FILE *f, const char *fname)
{
  int n;

  /* yydebug = 1; */
  yyin = f;
  n = yyparse(p);
  if (n == 0 && p->nerr == 0) {
    return 0;
  }
  return 1;
}

void
node_free(node* np) {
  if (!np) {
    return;
  }

  switch (np->type) {
  case NODE_ARGS:
    node_array_free(np);
    break;
  case NODE_IF:
    node_free(((node_if*)np->value.v.p)->cond);
    node_free(((node_if*)np->value.v.p)->compstmt);
    node_free(((node_if*)np->value.v.p)->opt_else);
    free(np);
    break;
  case NODE_EMIT:
    node_free((node*) np->value.v.p);
    free(np);
    break;
  case NODE_OP:
    node_free(((node_op*) np->value.v.p)->lhs);
    node_free(((node_op*) np->value.v.p)->rhs);
    free(np);
    break;
  case NODE_BLOCK:
    node_free(((node_block*) np->value.v.p)->args);
    node_free(((node_block*) np->value.v.p)->compstmt);
    free(np);
    break;
  case NODE_CALL:
    node_free(((node_call*) np->value.v.p)->cond);
    node_free(((node_call*) np->value.v.p)->ident);
    node_free(((node_call*) np->value.v.p)->args);
    node_free(((node_call*) np->value.v.p)->blk);
    free(np);
    break;
  case NODE_RETURN:
    node_free((node*) np->value.v.p);
    free(np);
    break;
  case NODE_IDENT:
    free(np);
    break;
  case NODE_VALUE:
    switch (np->value.t) {
    case NODE_VALUE_DOUBLE:
      free(np);
      break;
    case NODE_VALUE_STRING:
      free(np->value.v.s);
      free(np);
      break;
    case NODE_VALUE_BOOL:
      break;
    case NODE_VALUE_NIL:
      break;
    case NODE_VALUE_ARRAY:
      node_array_free(np);
      break;
    case NODE_VALUE_MAP:
      node_map_free(np);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

void
node_parse_free(parser_state* p)
{
  node_free(p->lval);
}
