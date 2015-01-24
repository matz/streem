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
strm_value_new(strm_node* v) {
  return NULL;
}

strm_node*
strm_array_new() {
  return NULL;
}

strm_node*
strm_array_add(strm_node* arr, strm_node* node)
{
  return NULL;
}

strm_node*
strm_stmt_new()
{
  return NULL;
}

strm_node*
strm_let_new(strm_node* var, strm_node* node)
{
  return NULL;
}

strm_node*
strm_op_new(const char* op, strm_node* lhs, strm_node* rhs)
{
  return NULL;
}

strm_node*
strm_funcall_new(strm_id id, strm_node* args, strm_node* blk)
{
  return NULL;
}

strm_node*
strm_nil_value()
{
  return NULL;
}

strm_node*
strm_double_new(strm_double d)
{
  strm_node* node = malloc(sizeof(strm_node));
  
  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_DOUBLE;
  node->value.v.d = d;
  return node;
}

strm_node*
strm_string_new(strm_string s)
{
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_STRING;
  node->value.v.s = strdup0(s);
  return node;
}

strm_node*
strm_string_len_new(strm_string s, size_t l) {
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_VALUE;
  node->value.t = STRM_VALUE_STRING;
  node->value.v.s = strndup0(s, l);
  return node;
}

strm_node*
strm_ident_new(strm_id id) {
  strm_node* node = malloc(sizeof(strm_node));

  node->type = STRM_NODE_IDENT;
  node->value.t = STRM_VALUE_FIXNUM;
  node->value.v.id = id;
  return node;
}
