#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

#define YYDEBUG 1

#include "y.tab.h"
#include "lex.yy.h"
/* old bison does not have yyparse prototype in y.tab.h */
int yyparse(parser_state*);

node*
node_values_new(node_type type) {
  /* TODO: error check */
  node_values* v = malloc(sizeof(node_values));
  v->type = type;
  v->len = 0;
  v->max = 0;
  v->data = NULL;
  v->headers = NULL;
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

void
node_values_concat(node_values* v, node_values* v2) {
  if (v->len + v2->len > v->max) {
    v->max = v->len + v2->len + 10;
    v->data = realloc(v->data, sizeof(void*) * v->max);
  }
  memcpy(v->data+v->len, v2->data, v2->len*sizeof(void*));
  v->len += v2->len;
}

void
node_values_prepend(node_values* v, void* data) {
  if (v->len == v->max) {
    v->max = v->len + 10;
    v->data = realloc(v->data, sizeof(void*) * v->max);
  }
  memmove(v->data+1, v->data, v->len*sizeof(void*));
  v->data[0] = data;
  v->len++;
}

void
node_values_free(node* np)
{
  node_values* v = (node_values*)np;
  v->headers = NULL;            /* leave free() upto GC */
  free(v->data);
  free(np);
}

node*
node_array_new()
{
  return node_values_new(NODE_ARRAY);
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
  node_values_free(np);
}

node*
node_stmts_new()
{
  return node_values_new(NODE_STMTS);
}

void
node_stmts_add(node* s, node* np)
{
  node_array_add(s, np);
}

node*
node_stmts_concat(node* s, node* s2)
{
  if (!s) return s2;
  if (s2) {
    node_values_concat((node_values*)s, (node_values*)s2);
  }
  return s;
}

node*
node_pair_new(strm_string* key, node* value)
{
  node_pair* npair = malloc(sizeof(node_pair));
  npair->type = NODE_PAIR;
  npair->key = key;
  npair->value = value;
  return (node*)npair;
}

node*
node_array_headers(node* np)
{
  int i;
  node_values* v;
  strm_array *headers = NULL;
  strm_value *p = NULL;

  if (!np)
    np = node_array_new();
  v = (node_values*)np;
  for (i = 0; i < v->len; i++) {
    node_pair* npair = (node_pair*)v->data[i];
    if (npair->type == NODE_PAIR) {
      if (!headers) {
        headers = strm_ary_new(NULL, v->len);
        p = (strm_value*)headers->ptr;
      }
      p[i] = strm_ptr_value(npair->key);
      v->data[i] = npair->value;
    }
  }
  v->headers = headers;

  return np;
}

node*
node_args_new()
{
  return node_values_new(NODE_ARGS);
}

void
node_args_add(node* arr, strm_string* s)
{
  node_values_add((node_values*)arr, s);
}

node*
node_ns_new(strm_string* name, node* body)
{
  node_ns* nns = malloc(sizeof(node_ns));
  nns->type = NODE_NS;
  nns->name = name;
  nns->body = body;
  return (node*)nns;
}

node*
node_import_new(strm_string* name)
{
  node_import* nimp = malloc(sizeof(node_import));
  nimp->type = NODE_IMPORT;
  nimp->name = name;
  return (node*)nimp;
}

node*
node_let_new(strm_string* lhs, node* rhs)
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
  nop->op = node_id(op, strlen(op));
  nop->rhs = rhs;
  return (node*)nop;
}

node*
node_lambda_new(node* args, node* compstmt)
{
  node_lambda* lambda = malloc(sizeof(node_lambda));
  lambda->type = NODE_LAMBDA;
  lambda->args = args;
  lambda->compstmt = compstmt;
  return (node*)lambda;
}

node*
node_method_new(node* args, node* compstmt)
{
  node_lambda* lambda = malloc(sizeof(node_lambda));
  lambda->type = NODE_LAMBDA;
  if (args) {
    node_values_prepend((node_values*)args, strm_str_intern("self", 4));
  }
  else {
    args = node_array_new();
    node_args_add(args, strm_str_intern("self", 4));
  }
  lambda->args = args;
  lambda->compstmt = compstmt;
  return (node*)lambda;
}

node*
node_call_new(strm_string* ident, node* recv, node* args, node* blk)
{
  node_call* ncall = malloc(sizeof(node_call));
  ncall->type = NODE_CALL;
  ncall->ident = ident;
  if (recv) {
    node_values_prepend((node_values*)args, recv);
  }
  if (blk) {
    node_array_add(args, blk);
  }
  ncall->args = args;
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

static size_t
string_escape(char* s, size_t len)
{
  char* t = s;
  char* tend = t + len;
  char* p = s;

  while (t < tend) {
    switch (*t) {
    case '\\':
      t++;
      if (t == tend) break;
      switch (*t) {
      case 'n':
        *p++ = '\n'; break;
      case 'r':
        *p++ = '\r'; break;
      case 't':
        *p++ = '\t'; break;
      case 'e':
        *p++ = 033; break;
      case '0':
        *p++ = '\0'; break;
      case 'x':
        {
          unsigned char c = 0;
          char* xend = t+3;

          t++;
          while (t < tend && t < xend) {
            switch (*t) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              c *= 16;
              c += *t - '0';
              break;
            case 'a': case 'b': case 'c':
            case 'd': case 'e': case 'f':
              c *= 16;
              c += *t - 'a' + 10;
              break;
            default:
              xend = t;
              break;
            }
            t++;
          }
          *p++ = (char)c;
          t--;
        }
        break;
      default:
        *p++ = *t; break;
      }
      t++;
      break;
    default:
      *p++ = *t++;
      break;
    }
  }
  return (size_t)(p - s);
}

node*
node_string_new(const char* s, size_t len)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = NODE_VALUE_STRING;
  len = string_escape((char*)s, len);
  np->value.v.s = strm_str_new(s, len);
  return np;
}

node*
node_ident_new(strm_string* name)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_IDENT;
  np->value.t = NODE_VALUE_IDENT;
  np->value.v.s = name;
  return np;
}

strm_string*
node_id(const char* s, size_t len)
{
  extern int strm_event_loop_started;
  strm_string *str;

  assert(!strm_event_loop_started);
  str = strm_str_intern(s, len);
  return str;
}

strm_string*
node_id_str(strm_string* s)
{
  if (s->flags & STRM_STR_INTERNED) {
    return s;
  }
  return strm_str_intern(s->ptr, s->len);
}

strm_string*
node_id_escaped(const char* s, size_t len)
{
  len = string_escape((char*)s, len);
  return strm_str_intern(s, len);
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
node_if_new(node* cond, node* then, node* opt_else)
{
  node_if* nif = malloc(sizeof(node_if));
  nif->type = NODE_IF;
  nif->cond = cond;
  nif->then = then;
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
node_skip_new(node* value)
{
  static node nd = { NODE_SKIP };
  return &nd;
}

node*
node_return_new(node* value)
{
  node_return* nreturn = malloc(sizeof(node_return));
  nreturn->type = NODE_RETURN;
  nreturn->rv = value;
  return (node*)nreturn;
}

node*
node_break_new(node* value)
{
  static node nd = { NODE_BREAK };
  return &nd;
}

void
node_parse_init(parser_state* p)
{
  p->nerr = 0;
  p->lval = NULL;
  p->fname = NULL;
  p->lineno = 1;
  p->tline = 1;
}

int
node_parse_file(parser_state* p, const char* fname)
{
  int r;
  FILE* fp = fopen(fname, "rb");
  if (fp == NULL) {
    perror("fopen");
    return 0;
  }
  p->fname = fname;
  r = node_parse_input(p, fp, fname);
  fclose(fp);
  return r;
}

int
node_parse_input(parser_state* p, FILE* f, const char* fname)
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
node_parse_string(parser_state* p, const char* prog)
{
  int n;

  /* yydebug = 1; */
  p->fname = "-e";
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
    node_values_free(np);
    break;
  case NODE_IF:
    node_free(((node_if*)np)->cond);
    node_free(((node_if*)np)->then);
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
  case NODE_LAMBDA:
    node_free(((node_lambda*) np)->args);
    node_free(((node_lambda*) np)->compstmt);
    free(np);
    break;
  case NODE_CALL:
    node_free(((node_call*) np)->args);
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
