#include "strm.h"
#include "node.h"

int strm_int_p(strm_value);
int strm_flt_p(strm_value);

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1
#define NODE_ERROR_SKIP 2

static void
strm_clear_exc(strm_stream* strm)
{
  if (strm->exc) {
    free(strm->exc);
  }
  strm->exc = NULL;
}

static node_error*
strm_set_exc(strm_stream* strm, int type, strm_value arg)
{
  node_error* exc = malloc(sizeof(node_error));

  if (!exc) return NULL;
  exc->type = type;
  exc->arg = arg;
  exc->fname = 0;
  exc->lineno = 0;
  strm_clear_exc(strm);
  strm->exc = exc;
  return exc;
}

static strm_string
node_to_sym(node_string s)
{
  return strm_str_intern(s->buf, s->len);
}

static strm_string
node_to_str(node_string s)
{
  return strm_str_new(s->buf, s->len);
}

static int
exec_plus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_string_p(*args)) {
    strm_string str1 = strm_value_str(args[0]);
    strm_string str2 = strm_value_str(args[1]);
    strm_string str3 = strm_str_new(NULL, strm_str_len(str1) + strm_str_len(str2));
    char *p;

    p = (char*)strm_str_ptr(str3);
    memcpy(p, strm_str_ptr(str1), strm_str_len(str1));
    memcpy(p+strm_str_len(str1), strm_str_ptr(str2), strm_str_len(str2));
    p[strm_str_len(str3)] = '\0';
    *ret = strm_str_value(str3);
    return STRM_OK;
  }
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])+strm_value_int(args[1]));
    return STRM_OK;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])+strm_value_flt(args[1]));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
exec_minus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  if (argc == 1) {
    if (strm_int_p(args[0])) {
      *ret = strm_int_value(-strm_value_int(args[0]));
      return STRM_OK;
    }
    if (strm_flt_p(args[0])) {
      *ret = strm_flt_value(-strm_value_flt(args[0]));
      return STRM_OK;
    }
    return STRM_NG;
  }
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])-strm_value_int(args[1]));
    return STRM_OK;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])-strm_value_flt(args[1]));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
exec_mult(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])*strm_value_int(args[1]));
    return STRM_OK;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])*strm_value_flt(args[1]));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
exec_div(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_flt_value(strm_value_flt(args[0])/strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_gt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_ge(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>=strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_lt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_le(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<=strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_eq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_eq(args[0], args[1]));
  return STRM_OK;
}

static int
exec_neq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(!strm_value_eq(args[0], args[1]));
  return STRM_OK;
}

static int blk_exec(strm_stream* strm, strm_value data);

struct array_data {
  int n;
  strm_array arr;
};

static int arr_exec(strm_stream* strm, strm_value data);
static int cfunc_exec(strm_stream* strm, strm_value data);

static int
exec_bar(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value lhs, rhs;

  assert(argc == 2);
  /* int x int */
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])|strm_value_int(args[1]));
    return STRM_OK;
  }

  lhs = args[0];
  /* lhs: io */
  if (strm_io_p(lhs)) {
    lhs = strm_stream_value(strm_io_stream(lhs, STRM_IO_READ));
  }
  /* lhs: lambda */
  else if (strm_lambda_p(lhs)) {
    strm_lambda lmbd = strm_value_lambda(lhs);
    lhs = strm_stream_value(strm_stream_new(strm_filter, blk_exec, NULL, (void*)lmbd));
  }
  /* lhs: array */
  else if (strm_array_p(lhs)) {
    struct array_data *arrd = malloc(sizeof(struct array_data));
    arrd->arr = strm_value_ary(lhs);
    arrd->n = 0;
    lhs = strm_stream_value(strm_stream_new(strm_producer, arr_exec, NULL, (void*)arrd));
  }
  /* lhs: should be stream */

  rhs = args[1];
  /* rhs: io */
  if (strm_io_p(rhs)) {
    rhs = strm_stream_value(strm_io_stream(rhs, STRM_IO_WRITE));
  }
  /* rhs: lambda */
  else if (strm_lambda_p(rhs)) {
    strm_lambda lmbd = strm_value_lambda(rhs);
    rhs = strm_stream_value(strm_stream_new(strm_filter, blk_exec, NULL, (void*)lmbd));
  }
  /* rhs: cfunc */
  else if (strm_cfunc_p(rhs)) {
    strm_cfunc func = strm_value_cfunc(rhs);
    rhs = strm_stream_value(strm_stream_new(strm_filter, cfunc_exec, NULL, func));
  }

  /* stream x stream */
  if (strm_stream_p(lhs) && strm_stream_p(rhs)) {
    strm_stream* lstrm = strm_value_stream(lhs);
    strm_stream* rstrm = strm_value_stream(rhs);
    if (lstrm == NULL || rstrm == NULL ||
        lstrm->mode == strm_consumer ||
        lstrm->mode == strm_killed ||
        rstrm->mode == strm_producer ||
        rstrm->mode == strm_killed) {
      strm_raise(strm, "stream error");
      return STRM_NG;
    }
    strm_stream_connect(strm_value_stream(lhs), strm_value_stream(rhs));
    *ret = rhs;
    return STRM_OK;
  }

  strm_raise(strm, "type error");
  return STRM_NG;
}

static int
exec_mod(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])%strm_value_int(args[1]));
    return STRM_OK;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value((int)strm_value_flt(args[0])%(int)strm_value_flt(args[1]));
    return STRM_OK;
  }
  return STRM_NG;
}

static int exec_expr(strm_stream* strm, strm_state* state, node* np, strm_value* val);

static int
ary_get(strm_stream* strm, strm_value ary, int argc, strm_value* argv, strm_value* ret)
{
  struct strm_array* a;
  strm_value idx;

  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  a = strm_ary_struct(ary);
  idx = argv[0];
  if (strm_num_p(idx)) {
    strm_int i = strm_value_int(idx);

    if (i>=a->len)
      return STRM_NG;
    *ret = a->ptr[i];
    return STRM_OK;
  }
  if (strm_string_p(idx)) {
    if (a->headers) {
      strm_int i, len = a->len;

      for (i=0; i<len; i++) {
        if (strm_str_eq(strm_value_str(idx),
                        strm_value_str(strm_ary_ptr(a->headers)[i]))) {
          *ret = a->ptr[i];
          return STRM_OK;
        }
      }
    }
  }
  return STRM_NG;
}

static int
lambda_call(strm_stream* strm, strm_value func, int argc, strm_value* argv, strm_value* ret)
{
  strm_lambda lambda = strm_value_lambda(func);
  node_lambda* nlbd = lambda->body;
  node_args* args = (node_args*)nlbd->args;
  strm_state c = {0};
  int i, n;
  node_error* exc;

  c.prev = lambda->state;
  if ((args == NULL && argc != 0) || (args->len != argc)) {
    if (strm) {
      strm_raise(strm, "wrong number of arguments");
      strm->exc->fname = nlbd->fname;
      strm->exc->lineno = nlbd->lineno;
    }
    return STRM_NG;
  }
  for (i=0; i<argc; i++) {
    n = strm_var_set(&c, node_to_sym(args->data[i]), argv[i]);
    if (n) return n;
  }
  n = exec_expr(strm, &c, nlbd->compstmt, ret);
  if (n == STRM_NG && strm) {
    exc = strm->exc;
    if (exc && exc->type == NODE_ERROR_RETURN) {
      *ret = exc->arg;
    }
  }
  return n;
}

int
strm_funcall(strm_stream* strm, strm_value func, int argc, strm_value* argv, strm_value* ret)
{
  switch (strm_value_tag(func)) {
  case STRM_TAG_CFUNC:
    return (strm_value_cfunc(func))(strm, argc, argv, ret);
  case STRM_TAG_ARRAY:
    return ary_get(strm, func, argc, argv, ret);
  case STRM_TAG_PTR:
    if (!strm_lambda_p(func)) {
      strm_raise(strm, "not a function");
      return STRM_NG;
    }
    else {
      return lambda_call(strm, func, argc, argv, ret);
    }
  default:
    strm_raise(strm, "not a function");
    break;
  }
  return STRM_NG;
}

static int
exec_call(strm_stream* strm, strm_state* state, strm_string name, int argc, strm_value* argv, strm_value* ret)
{
  int n = STRM_NG;
  strm_value m;

  if (argc > 0) {
    strm_state* ns = strm_value_ns(argv[0]);
    if (ns) {
      n = strm_var_get(ns, name, &m);
    }
    else if (argc == 1 && strm_array_p(argv[0])) {
      m = strm_str_value(name);
      n = ary_get(strm, argv[0], 1, &m, ret);
      if (n == STRM_OK) return STRM_OK;
    }
  }
  if (n == STRM_NG) {
    n = strm_var_get(state, name, &m);
  }
  if (n == STRM_OK) {
    return strm_funcall(strm, m, argc, argv, ret);
  }
  strm_raise(strm, "function not found");
  return STRM_NG;
}

static strm_array
ary_headers(node_string* headers, strm_int len)
{
  strm_array ary = strm_ary_new(NULL, len);
  strm_value* p = strm_ary_ptr(ary);
  strm_int i;

  for (i=0; i<len; i++) {
    p[i] = node_to_sym(headers[i]);
  }
  return ary;
}

static int
exec_expr(strm_stream* strm, strm_state* state, node* np, strm_value* val)
{
  int n;

  if (np == NULL) {
    return STRM_NG;
  }

  switch (np->type) {
/*
  case NODE_ARGS:
    break;
*/
  case NODE_NS:
    {
      node_ns* ns = (node_ns*)np;
      strm_state* s = strm_ns_find(state, node_to_sym(ns->name));

      if (!s) {
        strm_raise(strm, "failed to create namespace");
        return STRM_NG;
      }
      return exec_expr(strm, s, ns->body, val);
    }

  case NODE_IMPORT:
    {
      node_import *ns = (node_import*)np;
      strm_state* s = strm_ns_get(node_to_sym(ns->name));
      if (!s) {
        strm_raise(strm, "no such namespace");
        return STRM_NG;
      }
      n = strm_env_copy(state, s);
      if (n) {
        strm_raise(strm, "failed to import");
        return n;
      }
      return STRM_OK;
    }
    break;

  case NODE_SKIP:
    strm_set_exc(strm, NODE_ERROR_SKIP, strm_nil_value());
    return STRM_OK;
  case NODE_EMIT:
    {
      int i, n;
      node_array* v0;

      v0 = (node_array*)((node_emit*)np)->emit;
      if (!v0) {
        strm_emit(strm, strm_nil_value(), NULL);
      }
      else {
        for (i = 0; i < v0->len; i++) {
          n = exec_expr(strm, state, v0->data[i], val);
          if (n) return n;
          strm_emit(strm, *val, NULL);
        }
      }
      return STRM_OK;
    }
    break;
  case NODE_LET:
    {
      node_let *nlet = (node_let*)np;
      n = exec_expr(strm, state, nlet->rhs, val);
      if (n) {
        strm_raise(strm, "failed to assign");
        return n;
      }
      return strm_var_set(state, node_to_sym(nlet->lhs), *val);
    }
  case NODE_ARRAY:
    {
      node_array* v0 = (node_array*)np;
      strm_array arr = strm_ary_new(NULL, v0->len);
      strm_value *ptr = (strm_value*)strm_ary_ptr(arr);
      int i=0;

      for (i = 0; i < v0->len; i++, ptr++) {
        n = exec_expr(strm, state, v0->data[i], ptr);
        if (n) return n;
      }
      if (v0->headers) {
        strm_ary_headers(arr) = ary_headers(v0->headers, v0->len);
      }
      if (v0->ns) {
        strm_ary_ns(arr) = strm_ns_get(node_to_sym(v0->ns));
      }
      else {
        strm_ary_ns(arr) = strm_str_null;
      }
      *val = strm_ary_value(arr);
      return STRM_OK;
    }
  case NODE_IDENT:
    {
      node_ident* ni = (node_ident*)np;
      n = strm_var_get(state, node_to_sym(ni->name), val);
      if (n) {
        strm_raise(strm, "failed to reference variable");
      }
      return n;
    }
  case NODE_IF:
    {
      strm_value v;
      node_if* nif = (node_if*)np;
      n = exec_expr(strm, state, nif->cond, &v);
      if (n) return n;
      if (strm_bool_p(v) && strm_value_bool(v)) {
        return exec_expr(strm, state, nif->then, val);
      }
      else if (nif->opt_else != NULL) {
        return exec_expr(strm, state, nif->opt_else, val);
      }
      else {
        *val = strm_nil_value();
        return STRM_OK;
      }
    }
    break;
  case NODE_OP:
    {
      node_op* nop = (node_op*)np;
      strm_value args[2];
      int i=0;

      if (nop->lhs) {
        n = exec_expr(strm, state, nop->lhs, &args[i++]);
        if (n) return n;
      }
      if (nop->rhs) {
        n = exec_expr(strm, state, nop->rhs, &args[i++]);
        if (n) return n;
      }
      return exec_call(strm, state, node_to_sym(nop->op), i, args, val);
    }
    break;
  case NODE_LAMBDA:
    {
      strm_lambda lambda = malloc(sizeof(struct strm_lambda));

      if (!lambda) return STRM_NG;
      lambda->type = STRM_PTR_LAMBDA;
      lambda->body = (node_lambda*)np;
      lambda->state = state;
      *val = strm_ptr_value(lambda);
      return STRM_OK;
    }
    break;
  case NODE_CALL:
    {
      /* TODO: wip code of ident */
      node_call* ncall = (node_call*)np;
      int i;
      node_nodes* v0 = (node_nodes*)ncall->args;
      strm_value *args = malloc(sizeof(strm_value)*v0->len);

      for (i = 0; i < v0->len; i++) {
        n = exec_expr(strm, state, v0->data[i], &args[i]);
        if (n) return n;
      }
      return exec_call(strm, state, node_to_sym(ncall->ident), i, args, val);
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = (node_return*)np;
      node_nodes* args = (node_nodes*)nreturn->rv;
      strm_value arg;

      if (!args) {
        arg = strm_nil_value();
      }
      else {
        switch (args->len) {
        case 0:
          arg = strm_nil_value();
          break;
        case 1:
          n = exec_expr(strm, state, args->data[0], &arg);
          if (n) return n;
          break;
        default:
          {
            strm_array ary = strm_ary_new(NULL, args->len);
            strm_int i;

            for (i=0; i<args->len; i++) {
              n = exec_expr(strm, state, args->data[i], (strm_value*)&strm_ary_ptr(ary)[i]);
              if (n) return n;
            }
          }
          break;
        }
      }
      strm_set_exc(strm, NODE_ERROR_RETURN, arg);
      return STRM_OK;
    }
    break;
  case NODE_NODES:
    {
      int i;
      node_nodes* v = (node_nodes*)np;
      for (i = 0; i < v->len; i++) {
        n = exec_expr(strm, state, v->data[i], val);
        if (n) {
          if (strm) {
            node_error* exc = strm->exc;
            if (exc != NULL) {
              node* n = v->data[i];

              exc->fname = n->fname;
              exc->lineno = n->lineno;
            }
          }
          return n;
        }
      }
    }
    return STRM_OK;
  case NODE_INT:
    *val = strm_int_value(((node_int*)np)->value);
    return STRM_OK;
  case NODE_FLOAT:
    *val = strm_int_value(((node_float*)np)->value);
    return STRM_OK;
  case NODE_BOOL:
    *val = strm_bool_value(((node_bool*)np)->value);
    return STRM_OK;
  case NODE_NIL:
    *val = strm_nil_value();
    return STRM_OK;
  case NODE_STR:
    *val = strm_str_value(node_to_str(((node_str*)np)->value));
    return STRM_OK;
  default:
    break;
  }
  return STRM_NG;
}

static int
exec_cputs(strm_stream* strm, FILE* out, int argc, strm_value* args, strm_value* ret)
{
  int i;

  for (i = 0; i < argc; i++) {
    strm_string s;

    if (i != 0)
      fputs(", ", out);
    s = strm_to_str(args[i]);
    fwrite(strm_str_ptr(s), strm_str_len(s), 1, out);
  }
  fputs("\n", out);
  return STRM_OK;
}

static int
exec_puts(strm_stream* strm, int argc, strm_value* args, strm_value* ret) {
  return exec_cputs(strm, stdout, argc, args, ret);
}

void
strm_eprint(strm_stream* strm)
{
  strm_value v;
  node_error* exc = strm->exc;

  if (!exc) return;
  if (exc->fname) {
    fprintf(stderr, "%s:%d:", exc->fname, exc->lineno);
  }
  exec_cputs(strm, stderr, 1, &exc->arg, &v);
  /* TODO: garbage correct previous exception value */
  strm_clear_exc(strm);
}

#include <fcntl.h>

static int
exec_fread(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string path;
  char buf[7];

  assert(argc == 1);
  assert(strm_string_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(strm_str_cstr(path, buf), O_RDONLY);
  if (fd < 0) return STRM_NG;
  *ret = strm_io_new(fd, STRM_IO_READ);
  return STRM_OK;
}

static int
exec_fwrite(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string path;
  char buf[7];

  assert(argc == 1);
  assert(strm_string_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(strm_str_cstr(path, buf), O_WRONLY|O_CREAT, 0644);
  if (fd < 0) return STRM_NG;
  *ret = strm_io_new(fd, STRM_IO_WRITE);
  return STRM_OK;
}

void
strm_raise(strm_stream* strm, const char* msg)
{
  if (!strm) return;
  strm_set_exc(strm, NODE_ERROR_RUNTIME,
               strm_str_value(strm_str_new(msg, strlen(msg))));
}

void strm_iter_init(strm_state* state);
void strm_socket_init(strm_state* state);
void strm_csv_init(strm_state* state);
void strm_kvs_init(strm_state* state);

static void
node_init(strm_state* state)
{
  strm_var_def(state, "stdin", strm_io_new(0, STRM_IO_READ));
  strm_var_def(state, "stdout", strm_io_new(1, STRM_IO_WRITE));
  strm_var_def(state, "stderr", strm_io_new(2, STRM_IO_WRITE));
  strm_var_def(state, "puts", strm_cfunc_value(exec_puts));
  strm_var_def(state, "+", strm_cfunc_value(exec_plus));
  strm_var_def(state, "-", strm_cfunc_value(exec_minus));
  strm_var_def(state, "*", strm_cfunc_value(exec_mult));
  strm_var_def(state, "/", strm_cfunc_value(exec_div));
  strm_var_def(state, "<", strm_cfunc_value(exec_lt));
  strm_var_def(state, "<=", strm_cfunc_value(exec_le));
  strm_var_def(state, ">", strm_cfunc_value(exec_gt));
  strm_var_def(state, ">=", strm_cfunc_value(exec_ge));
  strm_var_def(state, "==", strm_cfunc_value(exec_eq));
  strm_var_def(state, "!=", strm_cfunc_value(exec_neq));
  strm_var_def(state, "|", strm_cfunc_value(exec_bar));
  strm_var_def(state, "%", strm_cfunc_value(exec_mod));
  strm_var_def(state, "fread", strm_cfunc_value(exec_fread));
  strm_var_def(state, "fwrite", strm_cfunc_value(exec_fwrite));

  strm_iter_init(state);
  strm_socket_init(state);
  strm_csv_init(state);
  strm_kvs_init(state);
}

int
node_run(parser_state* p)
{
  strm_value v;
  strm_state c = {0};
  strm_stream t = {0};
  node_error* exc;

  node_init(&c);

  exec_expr(&t, &c, (node*)p->lval, &v);
  exc = t.exc;
  if (exc != NULL) {
    if (exc->type != NODE_ERROR_RETURN) {
      strm_eprint(&t);
    }
    strm_clear_exc(&t);
  }
  return STRM_OK;
}

static int
blk_exec(strm_stream* strm, strm_value data)
{
  strm_lambda lambda = strm->data;
  strm_value ret = strm_nil_value();
  node_args* args = (node_args*)lambda->body->args;
  node_error* exc;
  int n;
  strm_state c = {0};

  c.prev = lambda->state;
  if (args) {
    assert(args->len == 1);
    strm_var_set(&c, node_to_sym(args->data[0]), data);
  }

  n = exec_expr(strm, &c, lambda->body->compstmt, &ret);
  exc = strm->exc;
  if (exc) {
    if (exc->type == NODE_ERROR_RETURN) {
      ret = exc->arg;
      strm_clear_exc(strm);
    }
    else {
      if (strm_option_verbose) {
        strm_eprint(strm);
      }
      return STRM_NG;
    }
  }
  if (n) return STRM_NG;
  strm_emit(strm, ret, NULL);
  return STRM_OK;
}

static int
arr_exec(strm_stream* strm, strm_value data)
{
  struct array_data *arrd = strm->data;

  if (arrd->n == strm_ary_len(arrd->arr)) {
    strm_stream_close(strm);
    return STRM_OK;
  }
  strm_emit(strm, strm_ary_ptr(arrd->arr)[arrd->n++], arr_exec);
  return STRM_OK;
}

static int
cfunc_exec(strm_stream* strm, strm_value data)
{
  strm_value ret;
  strm_cfunc func = strm->data;

  if ((*func)(strm, 1, &data, &ret) == STRM_OK) {
    strm_emit(strm, ret, NULL);
    return STRM_OK;
  }
  return STRM_NG;
}
