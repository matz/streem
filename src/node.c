#include "strm.h"
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
  node->type = STRM_NODE_ARRAY;
  node->value.t = STRM_VALUE_CARRAY;
  node->value.v.p = arr;
  return node;
}

strm_node*
node_array_of(strm_node* node)
{
  if (node == NULL)
    node = node_array_new();
  node->type = STRM_NODE_VALUE;
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
node_let_new(strm_node* var, strm_node* node)
{
  /* TODO */
  return NULL;
}

strm_node*
node_op_new(const char* op, strm_node* lhs, strm_node* rhs)
{
  /* TODO */
  return NULL;
}

strm_node*
node_funcall_new(strm_id id, strm_node* args, strm_node* blk)
{
  /* TODO */
  return NULL;
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
node_ident_of(const char* s)
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
