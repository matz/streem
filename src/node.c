#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

extern FILE *yyin, *yyout;
extern int yyparse(parser_state*);

static char*
strdup0(const char *s)
{
  size_t len = strlen(s);
  char *p;

  p = (char*)malloc(len+1);
  if (p) {
    strcpy(p, s);
  }
  return p;
}

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

node*
node_array_new()
{
  node_array* arr = malloc(sizeof(node_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  node* node = malloc(sizeof(node));
  node->type = NODE_VALUE;
  node->value.t = STRM_VALUE_ARRAY;
  node->value.v.p = arr;
  return node;
}

node*
node_array_of(node* node)
{
  if (node == NULL)
    node = node_array_new();
  return node;
}

void
node_array_add(node* arr, node* np)
{
  /* TODO: error check */
  node_array* arr0 = arr->value.v.p;
  if (arr0->len == arr0->max) {
    arr0->max = arr0->len + 10;
    arr0->data = realloc(arr0->data, sizeof(node*) * arr0->max);
  }
  arr0->data[arr0->len] = np;
  arr0->len++;
}

void
node_array_free(node* np)
{
  int i;
  node_array* arr0 = np->value.v.p;
  for (i = 0; i < arr0->len; i++)
    node_free(arr0->data[i]);
  free(arr0);
  free(np);
}

node*
node_pair_new(node* key, node* value)
{
  node_pair* pair = malloc(sizeof(node_pair));
  pair->key = key;
  pair->value = value;

  node* np = malloc(sizeof(node));
  np->type = NODE_PAIR;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = pair;
  return np;
}

node*
node_map_new()
{
  node_array* arr = malloc(sizeof(node_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  node* np = malloc(sizeof(node));
  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_MAP;
  np->value.v.p = arr;
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
  node_array* arr0 = np->value.v.p;
  for (i = 0; i < arr0->len; i++) {
    node* pair = arr0->data[i];
    node_pair* pair0 = pair->value.v.p;
    node_free(pair0->key);
    node_free(pair0->value);
    free(pair0);
  }
  free(arr0);
  free(np);
}

node*
node_let_new(node* lhs, node* rhs)
{
  node_let* node_let = malloc(sizeof(node_let));
  node_let->lhs = lhs;
  node_let->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_LET;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = node_let;
  return np;
}

node*
node_op_new(char* op, node* lhs, node* rhs)
{
  node_op* node_op = malloc(sizeof(node_op));
  node_op->lhs = lhs;
  node_op->op = op;
  node_op->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_OP;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = node_op;
  return np;
}

node*
node_block_new(node* args, node* compstmt)
{
  node_block* node_block = malloc(sizeof(node_block));
  node_block->args = args;
  node_block->compstmt = compstmt;

  node* np = malloc(sizeof(node));
  np->type = NODE_BLOCK;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = node_block;
  return np;
}

node*
node_call_new(node* cond, node* ident, node* args, node* blk)
{
  node_call* node_call = malloc(sizeof(node_call));
  node_call->cond = cond;
  node_call->ident = ident;
  node_call->args = args;
  node_call->blk = blk;

  node* np = malloc(sizeof(node));
  np->type = NODE_CALL;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = node_call;
  return np;
}

node*
node_double_new(strm_double d)
{
  node* np = malloc(sizeof(node));
  
  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_DOUBLE;
  np->value.v.d = d;
  return np;
}

node*
node_string_new(strm_string s)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_STRING;
  np->value.v.s = strdup0(s);
  return np;
}

node*
node_string_len_new(strm_string s, size_t l)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_STRING;
  np->value.v.s = strndup0(s, l);
  return np;
}

node*
node_ident_new(strm_id id)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_IDENT;
  np->value.t = STRM_VALUE_FIXNUM;
  np->value.v.id = id;
  return np;
}

strm_id
node_ident_of(char* s)
{
  /* TODO: get id of the identifier which named as s */
  return (strm_id) s;
}

node*
node_nil()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_NIL, {0} } };
  return &nd;
}

node*
node_true()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_BOOL, {1} } };
  return &nd;
}

node*
node_false()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_BOOL, {0} } };
  return &nd;
}

node*
node_if_new(node* cond, node* compstmt, node* opt_else)
{
  node_if* node_if = malloc(sizeof(node_if));
  node_if->cond = cond;
  node_if->compstmt = compstmt;
  node_if->opt_else = opt_else;

  node* np = malloc(sizeof(node));
  np->type = NODE_IF;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = node_if;
  return np;
}


node*
node_emit_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_EMIT;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = value;
  return np;
}

node*
node_return_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_RETURN;
  np->value.t = STRM_VALUE_USER;
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
strm_parse_init(parser_state *p)
{
  p->nerr = 0;
  p->lval = NULL;
  p->fname = NULL;
  p->lineno = 1;
  p->tline = 1;
  return 0;
}

int
strm_parse_file(parser_state* p, const char* fname)
{
  int r;
  FILE* fp = fopen(fname, "rb");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }
  r = strm_parse_input(p, fp, fname);
  fclose(fp);
  return r;
}

int
strm_parse_input(parser_state* p, FILE *f, const char *fname)
{
  int n;

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
    {
      node_free(((node_if*)np->value.v.p)->cond);
      node_free(((node_if*)np->value.v.p)->compstmt);
      node_free(((node_if*)np->value.v.p)->opt_else);
    }
    break;
  case NODE_EMIT:
    node_free((node*) np->value.v.p);
    break;
  case NODE_OP:
    node_free(((node_op*) np->value.v.p)->lhs);
    node_free(((node_op*) np->value.v.p)->rhs);
    break;
  case NODE_BLOCK:
    node_free(((node_block*) np->value.v.p)->args);
    node_free(((node_block*) np->value.v.p)->compstmt);
    break;
  case NODE_CALL:
    node_free(((node_call*) np->value.v.p)->cond);
    node_free(((node_call*) np->value.v.p)->ident);
    node_free(((node_call*) np->value.v.p)->args);
    node_free(((node_call*) np->value.v.p)->blk);
    break;
  case NODE_IDENT:
    free(np);
    break;
  case NODE_VALUE:
    switch (np->value.t) {
    case STRM_VALUE_DOUBLE:
      free(np);
      break;
    case STRM_VALUE_STRING:
      free(np->value.v.s);
      free(np);
      break;
    case STRM_VALUE_BOOL:
      free(np);
      break;
    case STRM_VALUE_NIL:
      break;
    case STRM_VALUE_ARRAY:
      node_array_free(np);
      break;
    case STRM_VALUE_MAP:
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
strm_parse_free(parser_state* p)
{
  node_free(p->lval);
}
