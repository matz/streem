#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

#include "y.tab.h"
#include "lex.yy.h"

node*
node_value_new(node* v)
{
  /* TODO */
  return NULL;
}

node*
node_values_new(node_type type) {
  /* TODO: error check */
  node_values* v = malloc(sizeof(node_values));
  v->type = type;
  v->len = 0;
  v->max = 0;
  v->data = NULL;
  return (node*)v;
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
  return node_values_new(NODE_ARRAY);
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
  node_values_add((node_values*)arr, np);
}

void
node_array_free(node* np)
{
  int i;
  node_values* v = (node_values*)np;
  for (i = 0; i < v->len; i++)
    node_free(v->data[i]);
  free(np);
}

node*
node_pair_new(node* key, node* value)
{
  node_pair* npair = malloc(sizeof(node_pair));
  npair->type = NODE_PAIR; 
  npair->key = key;
  npair->value = value;
  return (node*)npair;
}

node*
node_map_new()
{
  return node_values_new(NODE_MAP);
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
  node_values* v = (node_values*)np;
  for (i = 0; i < v->len; i++) {
    node* pair = v->data[i];
    node_pair* npair = (node_pair*)pair;
    node_free(npair->key);
    node_free(npair->value);
    free(npair);
    free(pair);
  }
  free(np);
}

node*
node_let_new(node* lhs, node* rhs)
{
  node_let* nlet = malloc(sizeof(node_let));
  nlet->type = NODE_LET;
  nlet->lhs = lhs;
  nlet->rhs = rhs;
  return (node*)nlet;
}

node*
node_op_new(const char* op, node* lhs, node* rhs)
{
  node_op* nop = malloc(sizeof(node_op));
  nop->type = NODE_OP;
  nop->lhs = lhs;
  nop->op = node_ident_of(op);
  nop->rhs = rhs;
  return (node*)nop;
}

node*
node_block_new(node* args, node* compstmt)
{
  node_block* block = malloc(sizeof(node_block));
  block->type = NODE_BLOCK;
  block->args = args;
  block->compstmt = compstmt;
  return (node*)block;
}

node*
node_call_new(node* recv, node* ident, node* args, node* blk)
{
  node_call* ncall = malloc(sizeof(node_call));
  ncall->type = NODE_CALL;
  ncall->recv = recv;
  ncall->ident = ident;
  ncall->args = args;
  ncall->blk = blk;
  return (node*)ncall;
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
  np->value.v.s = strm_str_new(s, l);
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

node*
node_ident_str(node_id id)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_STRING;
  np->value.v.s = id;
  return np;
}

node_id
node_ident_of(const char* s)
{
  extern int strm_event_loop_started;
  strm_string *str;

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
  nif->type = NODE_IF;
  nif->cond = cond;
  nif->compstmt = compstmt;
  nif->opt_else = opt_else;
  return (node*)nif;
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
  yyrestart(f);
  n = yyparse(p);
  if (n == 0 && p->nerr == 0) {
    return 0;
  }
  return 1;
}

int
node_parse_string(parser_state* p, const char *prog)
{
  int n;

  /* yydebug = 1; */
  yy_scan_string(prog);
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
    node_free(((node_if*)np)->cond);
    node_free(((node_if*)np)->compstmt);
    node_free(((node_if*)np)->opt_else);
    free(np);
    break;
  case NODE_EMIT:
    node_free((node*) np->value.v.p);
    free(np);
    break;
  case NODE_OP:
    node_free(((node_op*) np)->lhs);
    node_free(((node_op*) np)->rhs);
    free(np);
    break;
  case NODE_BLOCK:
    node_free(((node_block*) np)->args);
    node_free(((node_block*) np)->compstmt);
    free(np);
    break;
  case NODE_CALL:
    node_free(((node_call*) np)->recv);
    node_free(((node_call*) np)->ident);
    node_free(((node_call*) np)->args);
    node_free(((node_call*) np)->blk);
    free(np);
    break;
  case NODE_RETURN:
    node_free((node*) np);
    free(np);
    break;
  case NODE_IDENT:
    free(np);
    break;
  case NODE_ARRAY:
    node_array_free(np);
    break;
  case NODE_MAP:
    node_map_free(np);
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
