#include "strm.h"
#include "node.h"

int strm_int_p(strm_value);
int strm_flt_p(strm_value);

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1
#define NODE_ERROR_SKIP 2

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
exec_plus(strm_state* state, int argc, strm_value* args, strm_value* ret)
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
exec_minus(strm_state* state, int argc, strm_value* args, strm_value* ret)
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
exec_mult(strm_state* state, int argc, strm_value* args, strm_value* ret)
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
exec_div(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_flt_value(strm_value_flt(args[0])/strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_gt(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_ge(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>=strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_lt(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_le(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<=strm_value_flt(args[1]));
  return STRM_OK;
}

static int
exec_eq(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_eq(args[0], args[1]));
  return STRM_OK;
}

static int
exec_neq(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(!strm_value_eq(args[0], args[1]));
  return STRM_OK;
}

static int blk_exec(strm_task* strm, strm_value data);

struct array_data {
  int n;
  strm_array arr;
};

static int arr_exec(strm_task* strm, strm_value data);
static int cfunc_exec(strm_task* strm, strm_value data);

static int
exec_bar(strm_state* state, int argc, strm_value* args, strm_value* ret)
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
    strm_io io = strm_value_io(lhs);
    lhs = strm_task_value(strm_io_open(io, STRM_IO_READ));
  }
  /* lhs: lambda */
  else if (strm_lambda_p(lhs)) {
    strm_lambda lmbd = strm_value_lambda(lhs);
    lhs = strm_task_value(strm_task_new(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }
  /* lhs: array */
  else if (strm_array_p(lhs)) {
    struct array_data *arrd = malloc(sizeof(struct array_data));
    arrd->arr = strm_value_ary(lhs);
    arrd->n = 0;
    lhs = strm_task_value(strm_task_new(strm_task_prod, arr_exec, NULL, (void*)arrd));
  }
  /* lhs: should be task */

  rhs = args[1];
  /* rhs: io */
  if (strm_io_p(rhs)) {
    strm_io io = strm_value_io(rhs);
    rhs = strm_task_value(strm_io_open(io, STRM_IO_WRITE));
  }
  /* rhs: lambda */
  else if (strm_lambda_p(rhs)) {
    strm_lambda lmbd = strm_value_lambda(rhs);
    rhs = strm_task_value(strm_task_new(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }
  /* rhs: cfunc */
  else if (strm_cfunc_p(rhs)) {
    strm_cfunc func = strm_value_cfunc(rhs);
    rhs = strm_task_value(strm_task_new(strm_task_filt, cfunc_exec, NULL, func));
  }

  /* task x task */
  if (strm_task_p(lhs) && strm_task_p(rhs)) {
    if (strm_value_task(lhs) == NULL || strm_value_task(rhs) == NULL) {
      strm_raise(state, "task error");
      return STRM_NG;
    }
    strm_task_connect(strm_value_task(lhs), strm_value_task(rhs));
    *ret = rhs;
    return STRM_OK;
  }

  strm_raise(state, "type error");
  return STRM_NG;
}

static int
exec_mod(strm_state* state, int argc, strm_value* args, strm_value* ret)
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

static int exec_expr(strm_state* state, node* np, strm_value* val);

int
strm_funcall(strm_state* state, strm_value func, int argc, strm_value* argv, strm_value* ret)
{
  switch (strm_value_tag(func)) {
  case STRM_TAG_CFUNC:
    return (strm_value_cfunc(func))(state, argc, argv, ret);
  case STRM_TAG_PTR:
    if (!strm_lambda_p(func)) {
      strm_raise(state, "not a function");
      return STRM_NG;
    }
    else {
      strm_lambda lambda = strm_value_lambda(func);
      node_lambda* nlbd = lambda->body;
      node_args* args = (node_args*)nlbd->args;
      strm_state c = {0};
      int i, n;

      c.prev = lambda->state;
      if ((args == NULL && argc != 0) &&
          (args->len != argc)) return STRM_NG;
      for (i=0; i<argc; i++) {
        n = strm_var_set(&c, node_to_sym(args->data[i]), argv[i]);
        if (n) return n;
      }
      n = exec_expr(&c, nlbd->compstmt, ret);
      if (c.exc && c.exc->type == NODE_ERROR_RETURN) {
        *ret = c.exc->arg;
        return STRM_OK;
      }
      return n;
    }
  default:
    break;
  }
  return STRM_NG;
}

static int
exec_call(strm_state* state, strm_string name, int argc, strm_value* argv, strm_value* ret)
{
  int n = STRM_NG;
  strm_value m;

  if (argc > 0) {
    strm_state* ns = strm_value_ns(argv[0]);
    if (ns) {
      n = strm_var_get(ns, name, &m);
    }
  }
  if (n == STRM_NG) {
    n = strm_var_get(state, name, &m);
  }
  if (n == STRM_OK) {
    return strm_funcall(state, m, argc, argv, ret);
  }
  strm_raise(state, "function not found");
  return STRM_NG;
}

static strm_array
ary_headers(node_string* headers, size_t len)
{
  strm_array ary = strm_ary_new(NULL, len);
  strm_value* p = strm_ary_ptr(ary);
  size_t i;

  for (i=0; i<len; i++) {
    p[i] = node_to_sym(headers[i]);
  }
  return ary;
}

static int
exec_expr(strm_state* state, node* np, strm_value* val)
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
      node_ns *ns = (node_ns*)np;
      strm_state *s = strm_ns_find(state, node_to_sym(ns->name));

      if (!s) {
        strm_raise(state, "failed to create namespace");
        return STRM_NG;
      }
      return exec_expr(s, ns->body, val);
    }

  case NODE_IMPORT:
    {
      node_import *ns = (node_import*)np;
      strm_state* s = strm_ns_get(node_to_sym(ns->name));
      if (!s) {
        strm_raise(state, "no such namespace");
        return STRM_NG;
      }
      n = strm_env_copy(state, s);
      if (n) {
        strm_raise(state, "failed to import");
        return n;
      }
      return STRM_OK;
    }
    break;

  case NODE_SKIP:
    {
       state->exc = malloc(sizeof(node_error));
       state->exc->type = NODE_ERROR_SKIP;
       state->exc->arg = strm_nil_value();
       return STRM_OK;
    }
  case NODE_EMIT:
    {
      int i, n;
      node_array* v0;

      if (!state->task) {
        strm_raise(state, "failed to emit");
      }
      v0 = (node_array*)((node_emit*)np)->emit;
      if (!v0) {
        strm_emit(state->task, strm_nil_value(), NULL);
      }
      else {
        for (i = 0; i < v0->len; i++) {
          n = exec_expr(state, v0->data[i], val);
          if (n) return n;
          strm_emit(state->task, *val, NULL);
        }
      }
      return STRM_OK;
    }
    break;
  case NODE_LET:
    {
      node_let *nlet = (node_let*)np;
      n = exec_expr(state, nlet->rhs, val);
      if (n) {
        strm_raise(state, "failed to assign");
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
        n = exec_expr(state, v0->data[i], ptr);
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
        strm_raise(state, "failed to reference variable");
      }
      return n;
    }
  case NODE_IF:
    {
      strm_value v;
      node_if* nif = (node_if*)np;
      n = exec_expr(state, nif->cond, &v);
      if (n) return n;
      if (strm_bool_p(v) && strm_value_bool(v)) {
        return exec_expr(state, nif->then, val);
      }
      else if (nif->opt_else != NULL) {
        return exec_expr(state, nif->opt_else, val);
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
        n = exec_expr(state, nop->lhs, &args[i++]);
        if (n) return n;
      }
      if (nop->rhs) {
        n = exec_expr(state, nop->rhs, &args[i++]);
        if (n) return n;
      }
      return exec_call(state, node_to_sym(nop->op), i, args, val);
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
        n = exec_expr(state, v0->data[i], &args[i]);
        if (n) return n;
      }
      return exec_call(state, node_to_sym(ncall->ident), i, args, val);
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = (node_return*)np;
      node_nodes* args = (node_nodes*)nreturn->rv;

      state->exc = malloc(sizeof(node_error));
      state->exc->type = NODE_ERROR_RETURN;
      if (!args) {
        state->exc->arg = strm_nil_value();
        return STRM_OK;
      }
      switch (args->len) {
      case 0:
        state->exc->arg = strm_nil_value();
        break;
      case 1:
        n = exec_expr(state, args->data[0], &state->exc->arg);
        if (n) return n;
        break;
      default:
        {
          strm_array ary = strm_ary_new(NULL, args->len);
          size_t i;

          for (i=0; i<args->len; i++) {
            n = exec_expr(state, args->data[i], (strm_value*)&strm_ary_ptr(ary)[i]);
            if (n) return n;
          }
        }
        break;
      }
      return STRM_NG;
    }
    break;
  case NODE_NODES:
    {
      int i;
      node_nodes* v = (node_nodes*)np;
      for (i = 0; i < v->len; i++) {
        n = exec_expr(state, v->data[i], val);
        if (state->exc != NULL) return STRM_NG;
        if (n) return n;
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
exec_cputs(strm_state* state, FILE* out, int argc, strm_value* args, strm_value* ret)
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
exec_puts(strm_state* state, int argc, strm_value* args, strm_value* ret) {
  return exec_cputs(state, stdout, argc, args, ret);
}

#include <fcntl.h>

static int
exec_fread(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string path;
  char buf[7];

  assert(argc == 1);
  assert(strm_string_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(strm_str_cstr(path, buf), O_RDONLY);
  if (fd < 0) return STRM_NG;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_READ));
  return STRM_OK;
}

static int
exec_fwrite(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string path;
  char buf[7];

  assert(argc == 1);
  assert(strm_string_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(strm_str_cstr(path, buf), O_WRONLY|O_CREAT, 0644);
  if (fd < 0) return STRM_NG;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_WRITE));
  return STRM_OK;
}

void
strm_raise(strm_state* state, const char* msg) {
  state->exc = malloc(sizeof(node_error));
  state->exc->type = NODE_ERROR_RUNTIME;
  state->exc->arg = strm_str_value(strm_str_new(msg, strlen(msg)));
}

void strm_iter_init(strm_state* state);
void strm_socket_init(strm_state* state);
void strm_csv_init(strm_state* state);

static void
node_init(strm_state* state)
{
  strm_var_def(state, "stdin", strm_ptr_value(strm_io_new(0, STRM_IO_READ)));
  strm_var_def(state, "stdout", strm_ptr_value(strm_io_new(1, STRM_IO_WRITE)));
  strm_var_def(state, "stderr", strm_ptr_value(strm_io_new(2, STRM_IO_WRITE)));
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
}

int
node_run(parser_state* p)
{
  strm_value v;
  strm_state *state = malloc(sizeof(strm_state));

  memset(state, 0, sizeof(strm_state));
  node_init(state);

  exec_expr(state, (node*)p->lval, &v);
  if (state->exc != NULL) {
    if (state->exc->type != NODE_ERROR_RETURN) {
      strm_value v;
      exec_cputs(state, stderr, 1, &state->exc->arg, &v);
      /* TODO: garbage correct previous exception value */
      state->exc = NULL;
    }
  }
  return STRM_OK;
}

static int
blk_exec(strm_task* task, strm_value data)
{
  strm_lambda lambda = task->data;
  strm_value ret = strm_nil_value();
  node_args* args = (node_args*)lambda->body->args;
  int n;
  strm_state c = {0};

  c.task = task;
  c.prev = lambda->state;
  if (args) {
    assert(args->len == 1);
    strm_var_set(&c, node_to_sym(args->data[0]), data);
  }

  n = exec_expr(&c, lambda->body->compstmt, &ret);
  if (n) return STRM_NG;
  if (lambda->state->exc) {
    if (lambda->state->exc->type == NODE_ERROR_RETURN) {
      ret = lambda->state->exc->arg;
      free(lambda->state->exc);
      lambda->state->exc = NULL;
    } else {
      return STRM_NG;
    }
  }
  strm_emit(task, ret, NULL);
  return STRM_OK;
}

static int
arr_exec(strm_task* task, strm_value data)
{
  struct array_data *arrd = task->data;

  if (arrd->n == strm_ary_len(arrd->arr)) {
    strm_task_close(task);
    return STRM_OK;
  }
  strm_emit(task, strm_ary_ptr(arrd->arr)[arrd->n++], arr_exec);
  return STRM_OK;
}

static int
cfunc_exec(strm_task* task, strm_value data)
{
  strm_value ret;
  strm_cfunc func = task->data;
  strm_state c = {0};

  if ((*func)(&c, 1, &data, &ret) == STRM_OK) {
    strm_emit(task, ret, NULL);
    return STRM_OK;
  }
  return STRM_NG;
}
