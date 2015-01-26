#include "strm.h"
#include "node.h"
#include <string.h>

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

strm_node*
node_value_new(strm_node* v)
{
  /* TODO */
  return NULL;
}

strm_node*
node_array_new()
{
  strm_array* arr = malloc(sizeof(strm_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_ARRAY;
  node->value.v.p = arr;
  return node;
}

strm_node*
node_array_of(strm_node* node)
{
  if (node == NULL)
    node = node_array_new();
  return node;
}

void
node_array_add(strm_node* arr, strm_node* node)
{
  /* TODO: error check */
  strm_array* arr0 = arr->value.v.p;
  if (arr0->len == arr0->max) {
    arr0->max = arr0->len + 10;
    arr0->data = realloc(arr0->data, sizeof(strm_node*) * arr0->max);
  }
  arr0->data[arr0->len] = node;
  arr0->len++;
}

strm_node*
node_pair_new(strm_node* key, strm_node* value)
{
  strm_pair* pair = malloc(sizeof(strm_pair));
  pair->key = key;
  pair->value = value;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_PAIR;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = pair;
  return node;
}

strm_node*
node_map_new()
{
  strm_array* arr = malloc(sizeof(strm_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_MAP;
  node->value.v.p = arr;
  return node;
}

strm_node*
node_map_of(strm_node* node)
{
  if (node == NULL)
    node = node_map_new();
  return node;
}

strm_node*
node_let_new(strm_node* lhs, strm_node* rhs)
{
  strm_node_let* node_let = malloc(sizeof(strm_node_let));
  node_let->lhs = lhs;
  node_let->rhs = rhs;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_LET;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = node_let;
  return node;
}

strm_node*
node_op_new(char* op, strm_node* lhs, strm_node* rhs)
{
  strm_node_op* node_op = malloc(sizeof(strm_node_op));
  node_op->lhs = lhs;
  node_op->op = op;
  node_op->rhs = rhs;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_OP;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = node_op;
  return node;
}

strm_node*
node_block_new(strm_node* args, strm_node* compstmt)
{
  strm_node_block* node_block = malloc(sizeof(strm_node_block));
  node_block->args = args;
  node_block->compstmt = compstmt;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_BLOCK;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = node_block;
  return node;
}

strm_node*
node_call_new(strm_node* cond, strm_node* ident, strm_node* args, strm_node* blk)
{
  strm_node_call* node_call = malloc(sizeof(strm_node_call));
  node_call->cond = cond;
  node_call->ident = ident;
  node_call->args = args;
  node_call->blk = blk;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_CALL;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = node_call;
  return node;
}

strm_node*
node_double_new(strm_double d)
{
  strm_node* node = malloc(sizeof(strm_node));
  
  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_DOUBLE;
  node->value.v.d = d;
  return node;
}

strm_node*
node_string_new(strm_string s)
{
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_STRING;
  node->value.v.s = strdup0(s);
  return node;
}

strm_node*
node_string_len_new(strm_string s, size_t l)
{
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_STRING;
  node->value.v.s = strndup0(s, l);
  return node;
}

strm_node*
node_ident_new(strm_id id)
{
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_IDENT;
  node->value.t = STRM_VALUE_FIXNUM;
  node->value.v.id = id;
  return node;
}

strm_id
node_ident_of(char* s)
{
  /* TODO: get id of the identifier which named as s */
  return (strm_id) s;
}

strm_node*
node_nil()
{
  static strm_node node = { STRM_NODE_VALUE, { STRM_VALUE_NIL, {0} } };
  return &node;
}

strm_node*
node_true()
{
  static strm_node node = { STRM_NODE_VALUE, { STRM_VALUE_BOOL, {1} } };
  return &node;
}

strm_node*
node_false()
{
  static strm_node node = { STRM_NODE_VALUE, { STRM_VALUE_BOOL, {0} } };
  return &node;
}

strm_node*
node_if_new(strm_node* cond, strm_node* compstmt, strm_node* opt_else)
{
  strm_node_if* node_if = malloc(sizeof(strm_node_if));
  node_if->cond = cond;
  node_if->compstmt = compstmt;
  node_if->opt_else = opt_else;

  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_IF;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = node_if;
  return node;
}


strm_node*
node_emit_new(strm_node* value)
{
  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_EMIT;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = value;
  return node;
}

strm_node*
node_return_new(strm_node* value)
{
  strm_node* node = malloc(sizeof(strm_node));
  node->type = STRM_NODE_RETURN;
  node->value.t = STRM_VALUE_USER;
  node->value.v.p = value;
  return node;
}

strm_node*
node_break_new(strm_node* value)
{
  static strm_node node = { STRM_NODE_BREAK };
  return &node;
}
