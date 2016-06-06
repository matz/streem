#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define YYDEBUG 1

#include "y.tab.h"
#include "lex.yy.h"
/* old bison does not have yyparse prototype in y.tab.h */
int yyparse(parser_state*);

#define VALUES_NEW(type1, type2, blk) \
  type1* v = malloc(sizeof(type1));\
  v->type = type2;\
  v->len = 0;\
  v->max = 0;\
  v->data = NULL;\
  blk;\
  return (node*)v;

#define VALUES_ADD(type1, v, type2) \
  type1* _v = (type1*)v;\
  if (_v->len == _v->max) {\
    _v->max = _v->len + 10;\
    _v->data = realloc(_v->data, sizeof(type1) * _v->max);\
  }\
  _v->data[_v->len] = data;\
  _v->len++;

#define VALUES_PREPEND(type1, v, type2) \
  type1* _v = (type1*)v;\
  if (_v->len == _v->max) {\
    _v->max = _v->len + 10;\
    _v->data = realloc(_v->data, sizeof(type2) * _v->max);\
  }\
  memmove(_v->data+1, _v->data, _v->len*sizeof(type2));\
  _v->data[0] = data;\
  _v->len++;\

node*
node_array_new() {
  VALUES_NEW(node_array, NODE_ARRAY, {
      v->headers = NULL;
      v->ns = NULL;
    });
}

node*
node_pair_new(node_string key, node* value)
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
  node_array* v;
  node_string* headers = NULL;

  if (!np)
    np = node_array_new();
  v = (node_array*)np;
  for (i = 0; i < v->len; i++) {
    node_pair* npair = (node_pair*)v->data[i];
    if (npair && npair->type == NODE_PAIR) {
      if (!headers) {
        headers = malloc(sizeof(node_string)*v->len);
      }
      headers[i] = npair->key;
      v->data[i] = npair->value;
      free(npair);
    }
  }
  v->headers = headers;

  return np;
}

void
node_array_add(node* a, node* data) {
  VALUES_ADD(node_array,a,node*);
}

void
node_array_free(node_array* v)
{
  strm_int i;

  for (i = 0; i < v->len; i++)
    node_free(v->data[i]);
  free(v->data);
  if (v->headers) {
    for (i=0; i<v->len; i++) {
      free(v->headers[i]);
    }
    free(v->headers);
  }
  if (v->ns) {
    free(v->ns);
  }
  free(v);
}

node*
node_nodes_new()
{
  VALUES_NEW(node_nodes, NODE_NODES, {});
}

void
node_nodes_add(node* a, node* data)
{
  VALUES_ADD(node_nodes,a,node*);
}

void
node_nodes_prepend(node* a, node* data) {
  VALUES_PREPEND(node_nodes,a,node*);
}

node*
node_nodes_concat(node* s, node* s2)
{
  if (!s) return s2;
  if (s2) {
    node_nodes* v = (node_nodes*)s;
    node_nodes* v2 = (node_nodes*)s2;

    if (v->len + v2->len > v->max) {
      v->max = v->len + v2->len + 10;
      v->data = realloc(v->data, sizeof(void*) * v->max);
    }
    memcpy(v->data+v->len, v2->data, v2->len*sizeof(node*));
    v->len += v2->len;
  }
  return s;
}

void
node_nodes_free(node_nodes* v)
{
  strm_int i;

  for (i = 0; i < v->len; i++)
    node_free(v->data[i]);
  free(v->data);
  free(v);
}

node*
node_obj_new(node* np, node_string ns)
{
  node_array* v;

  if (!np) v = (node_array*)node_array_new();
  else v = (node_array*)np;
  v->ns = ns;
  return (node*)v;
}

node*
node_args_new()
{
  VALUES_NEW(node_args, NODE_ARGS, {});
}

void
node_args_add(node* v, node_string data)
{
  VALUES_ADD(node_args,v,node_string);
}

void
node_args_prepend(node* a, node_string data) {
  VALUES_PREPEND(node_args,a,node_string);
}

void
node_args_free(node* a)
{
  strm_int i;
  node_args* v = (node_args*)a;

  assert(a->type == NODE_ARGS);
  for (i = 0; i < v->len; i++)
    free(v->data[i]);
  free(v->data);
  free(v);
}

node*
node_ns_new(node_string name, node* body)
{
  node_ns* newns = malloc(sizeof(node_ns));
  newns->type = NODE_NS;
  newns->name = name;
  newns->body = body;
  return (node*)newns;
}

node*
node_import_new(node_string name)
{
  node_import* nimp = malloc(sizeof(node_import));
  nimp->type = NODE_IMPORT;
  nimp->name = name;
  return (node*)nimp;
}

node*
node_let_new(node_string lhs, node* rhs)
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
  nop->op = node_str_new(op, strlen(op));
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
  lambda->fname = compstmt ? compstmt->fname : NULL;
  lambda->lineno = compstmt ? compstmt->lineno : 0;
  return (node*)lambda;
}

node*
node_method_new(node* args, node* compstmt)
{
  node_lambda* lambda = malloc(sizeof(node_lambda));
  lambda->type = NODE_LAMBDA;
  if (args) {
    node_args_prepend(args, node_str_new("self", 4));
  }
  else {
    args = node_args_new();
    node_args_add(args, node_str_new("self", 4));
  }
  lambda->args = args;
  lambda->compstmt = compstmt;
  return (node*)lambda;
}

node*
node_call_new(node_string ident, node* recv, node* args, node* blk)
{
  node_call* ncall = malloc(sizeof(node_call));
  ncall->type = NODE_CALL;
  ncall->ident = ident;
  if (!args) args = node_array_new();
  if (recv) {
    node_nodes_prepend(args, recv);
  }
  if (blk) {
    node_nodes_add(args, blk);
  }
  ncall->args = args;
  return (node*)ncall;
}

node*
node_fcall_new(node* func, node* args, node* blk)
{
  node_fcall* ncall = malloc(sizeof(node_fcall));
  ncall->type = NODE_FCALL;
  ncall->func = func;
  if (!args) args = node_array_new();
  if (blk) {
    node_nodes_add(args, blk);
  }
  ncall->args = args;
  return (node*)ncall;
}

node*
node_int_new(long i)
{
  node_int* ni = malloc(sizeof(node_int));

  ni->type = NODE_INT;
  ni->value = i;
  return (node*)ni;
}

node*
node_float_new(double d)
{
  node_float* nf = malloc(sizeof(node_float));

  nf->type = NODE_FLOAT;
  nf->value = d;
  return (node*)nf;
}

node*
node_time_new(const char* s, strm_int len)
{
  long sec, usec;
  int utc_offset;
  node_time* ns;

  if (strm_time_parse_time(s, len, &sec, &usec, &utc_offset) < 0) {
    return NULL;
  }
  ns = malloc(sizeof(node_time));
  ns->type = NODE_TIME;
  ns->sec = sec;
  ns->usec = usec;
  ns->utc_offset = utc_offset;
  return (node*)ns;
}

static strm_int
string_escape(char* s, strm_int len)
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
  return (strm_int)(p - s);
}

node*
node_string_new(const char* s, strm_int len)
{
  node_str* ns = malloc(sizeof(node_str));

  ns->type = NODE_STR;
  len = string_escape((char*)s, len);
  ns->value = node_str_new(s, len);
  return (node*)ns;
}

node*
node_ident_new(node_string name)
{
  node_ident* ni = malloc(sizeof(node_ident));

  ni->type = NODE_IDENT;
  ni->name = name;
  return (node*)ni;
}

node_string
node_str_new(const char* s, strm_int len)
{
  node_string str;

  str = malloc(sizeof(struct node_string)+len+1);
  str->len = len;
  memcpy(str->buf, s, len);
  str->buf[len] = '\0';
  return str;
}

node_string
node_str_escaped(const char* s, strm_int len)
{
  len = string_escape((char*)s, len);
  return node_str_new(s, len);
}

node*
node_nil()
{
  static node nd = { NODE_NIL };
  return &nd;
}

node*
node_true()
{
  static node_bool nd = { NODE_BOOL, 0, 0, 1 };
  return (node*)&nd;
}

node*
node_false()
{
  static node_bool nd = { NODE_BOOL, 0, 0, 0 };
  return (node*)&nd;
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
  node_emit* ne = malloc(sizeof(node_emit));
  ne->type = NODE_EMIT;
  ne->emit = value;
  return (node*)ne;
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
    node_args_free(np);
    break;
  case NODE_IF:
    node_free(((node_if*)np)->cond);
    node_free(((node_if*)np)->then);
    node_free(((node_if*)np)->opt_else);
    free(np);
    break;
  case NODE_EMIT:
    node_free(((node_emit*)np)->emit);
    free(np);
    break;
  case NODE_OP:
    node_free(((node_op*)np)->lhs);
    node_free(((node_op*)np)->rhs);
    free(np);
    break;
  case NODE_LAMBDA:
    node_args_free(((node_lambda*)np)->args);
    node_free(((node_lambda*)np)->compstmt);
    free(np);
    break;
  case NODE_CALL:
    node_free(((node_call*)np)->args);
    free(np);
    break;
  case NODE_RETURN:
    node_free((node*)np);
    free(np);
    break;
  case NODE_IDENT:
    free(((node_ident*)np)->name);
    free(np);
    break;
  case NODE_ARRAY:
    node_array_free((node_array*)np);
    break;
  case NODE_INT:
  case NODE_FLOAT:
    free(np);
    break;
  case NODE_BOOL:
    return;
  case NODE_STR:
    free(((node_str*)np)->value);
    free(np);
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
