#include "strm.h"
#include "node.h"

int strm_int_p(strm_value);
int strm_flt_p(strm_value);

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1
#define NODE_ERROR_SKIP 2

static int
exec_plus(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_str_p(*args)) {
    strm_string *str1 = strm_value_str(args[0]);
    strm_string *str2 = strm_value_str(args[1]);
    strm_string *str3 = strm_str_new(NULL, str1->len + str2->len);
    char *p;

    p = (char*)str3->ptr;
    memcpy(p, str1->ptr, str1->len);
    memcpy(p+str1->len, str2->ptr, str2->len);
    p[str3->len] = '\0';
    *ret = strm_ptr_value(str3);
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
  strm_array* arr;
};

static int arr_exec(strm_task* strm, strm_value data);
static int arr_finish(strm_task* strm, strm_value data);
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
    strm_io *io = strm_value_io(lhs);
    lhs = strm_task_value(strm_io_open(io, STRM_IO_READ));
  }
  /* lhs: lambda */
  else if (strm_lambda_p(lhs)) {
    strm_lambda *lmbd = strm_value_lambda(lhs)
    lhs = strm_task_value(strm_task_new(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }
  /* lhs: array */
  else if (strm_array_p(lhs)) {
    struct array_data *arrd = malloc(sizeof(struct array_data));
    arrd->arr = strm_value_array(lhs);
    arrd->n = 0;
    lhs = strm_task_value(strm_task_new(strm_task_prod, arr_exec, arr_finish, (void*)arrd));
  }
  /* lhs: should be task */

  rhs = args[1];
  /* rhs: io */
  if (strm_io_p(rhs)) {
    strm_io *io = strm_value_io(rhs);
    rhs = strm_task_value(strm_io_open(io, STRM_IO_WRITE));
  }
  /* rhs: lambda */
  else if (strm_lambda_p(rhs)) {
    strm_lambda *lmbd = strm_value_lambda(rhs);
    rhs = strm_task_value(strm_task_new(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }
  /* rhs: cfunc */
  else if (strm_cfunc_p(rhs)) {
    void *func = rhs.val.p;
    rhs = strm_task_value(strm_task_new(strm_task_filt, cfunc_exec, NULL, func));
  }

  /* task x task */
  if (strm_task_p(lhs) && strm_task_p(rhs)) {
    if (lhs.val.p == NULL || rhs.val.p == NULL) {
      node_raise(state, "task error");
      return STRM_NG;
    }
    strm_task_connect(strm_value_task(lhs), strm_value_task(rhs));
    *ret = rhs;
    return STRM_OK;
  }

  node_raise(state, "type error");
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

typedef int (*exec_cfunc)(strm_state*, int, strm_value*, strm_value*);
static int exec_expr(strm_state* state, node* np, strm_value* val);

static int
exec_call(strm_state* state, strm_string* name, int argc, strm_value* argv, strm_value* ret)
{
  int n;
  strm_value m;

  n = strm_var_get(state, name, &m);
  if (n == 0) {
    switch (m.type) {
    case STRM_VALUE_CFUNC:
      return ((exec_cfunc)m.val.p)(state, argc, argv, ret);
    case STRM_VALUE_PTR:
      {
        strm_lambda* lambda = strm_value_ptr(m);
        node_lambda* nlbd = lambda->body;
        node_values* args = (node_values*)nlbd->args;
        strm_state c = {0};
        int i;

        c.prev = lambda->state;
        if ((args == NULL && argc != 0) &&
            (args->len != argc)) return STRM_NG;
        for (i=0; i<argc; i++) {
          n = strm_var_set(&c, (strm_string*)args->data[i], argv[i]);
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
  }
  node_raise(state, "function not found");
  return STRM_NG;
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
      node_values* v0;

      if (!state->task) {
        node_raise(state, "failed to emit");
      }
      v0 = (node_values*)np->value.v.p;

      for (i = 0; i < v0->len; i++) {
        n = exec_expr(state, v0->data[i], val);
        if (n) return n;
        strm_emit(state->task, *val, NULL);
      }
      return STRM_OK;
    }
    break;
  case NODE_LET:
    {
      node_let *nlet = (node_let*)np;
      n = exec_expr(state, nlet->rhs, val);
      if (n) {
        node_raise(state, "failed to assign");
        return n;
      }
      return strm_var_set(state, nlet->lhs, *val);
    }
  case NODE_ARRAY:
    {
      node_values* v0 = (node_values*)np;
      strm_array *arr = strm_ary_new(NULL, v0->len);
      strm_value *ptr = (strm_value*)arr->ptr;
      int i=0;

      for (i = 0; i < v0->len; i++, ptr++) {
        n = exec_expr(state, v0->data[i], ptr);
        if (n) return n;
      }
      arr->headers = v0->headers;
      *val = strm_ptr_value(arr);
      return STRM_OK;
    }
  case NODE_IDENT:
    n = strm_var_get(state, np->value.v.s, val);
    if (n) {
      node_raise(state, "failed to reference variable");
    }
    return n;
  case NODE_IF:
    {
      strm_value v;
      node_if* nif = (node_if*)np;
      n = exec_expr(state, nif->cond, &v);
      if (n) return n;
      if (strm_value_bool(v) && v.val.i) {
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
      return exec_call(state, nop->op, i, args, val);
    }
    break;
  case NODE_LAMBDA:
    {
      struct strm_lambda* lambda = malloc(sizeof(strm_lambda));

      if (!lambda) return STRM_NG;
      lambda->type = STRM_OBJ_LAMBDA;
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
      node_values* v0 = (node_values*)ncall->args;
      strm_value *args = malloc(sizeof(strm_value)*v0->len);

      for (i = 0; i < v0->len; i++) {
        n = exec_expr(state, v0->data[i], &args[i]);
        if (n) return n;
      }
      return exec_call(state, ncall->ident, i, args, val);
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = (node_return*)np;
      node_values* args = (node_values*)nreturn->rv;

      state->exc = malloc(sizeof(node_error));
      state->exc->type = NODE_ERROR_RETURN;
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
          strm_array* ary = strm_ary_new(NULL, args->len);
          size_t i;

          for (i=0; i<args->len; i++) {
            n = exec_expr(state, args->data[i], (strm_value*)&ary->ptr[i]);
            if (n) return n;
          }
        }
        break;
      }
      return STRM_NG;
    }
    break;
  case NODE_STMTS:
    {
      int i;
      node_values* v = (node_values*)np;
      for (i = 0; i < v->len; i++) {
        n = exec_expr(state, v->data[i], val);
        if (state->exc != NULL) return STRM_NG;
        if (n) return n;
      }
    }
    return STRM_OK;
  case NODE_VALUE:
    switch (np->value.t) {
    case NODE_VALUE_BOOL:
      *val = strm_bool_value(np->value.v.b);
      return STRM_OK;
    case NODE_VALUE_NIL:
      *val = strm_nil_value();
      return STRM_OK;
    case NODE_VALUE_STRING:
    case NODE_VALUE_IDENT:
      *val = strm_ptr_value(np->value.v.s);
      return STRM_OK;
    case NODE_VALUE_DOUBLE:
      *val = strm_flt_value(np->value.v.d);
      return STRM_OK;
    case NODE_VALUE_INT:
      *val = strm_int_value(np->value.v.i);
      return STRM_OK;
      /* following type should not be evaluated */
    case NODE_VALUE_ERROR:
    case NODE_VALUE_USER:
    default:
      return STRM_NG;
    }
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
    strm_string *s;

    if (i != 0)
      fputs(", ", out);
    s = strm_to_str(args[i]);
    fwrite(s->ptr, s->len, 1, out);
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
  strm_string *path;

  assert(argc == 1);
  assert(strm_str_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(path->ptr, O_RDONLY);
  if (fd < 0) return STRM_NG;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_READ));
  return STRM_OK;
}

static int
exec_fwrite(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string *path;

  assert(argc == 1);
  assert(strm_str_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(path->ptr, O_WRONLY|O_CREAT, 0644);
  if (fd < 0) return STRM_NG;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_WRITE));
  return STRM_OK;
}

void
node_raise(strm_state* state, const char* msg) {
  state->exc = malloc(sizeof(node_error));
  state->exc->type = NODE_ERROR_RUNTIME;
  state->exc->arg = strm_str_value(msg, strlen(msg));
}

void strm_seq_init(strm_state* state);
void strm_socket_init(strm_state* state);
void strm_csv_init(strm_state* state);

static void
node_init(strm_state* state)
{
  strm_var_def("stdin", strm_ptr_value(strm_io_new(0, STRM_IO_READ)));
  strm_var_def("stdout", strm_ptr_value(strm_io_new(1, STRM_IO_WRITE)));
  strm_var_def("stderr", strm_ptr_value(strm_io_new(2, STRM_IO_WRITE)));
  strm_var_def("puts", strm_cfunc_value(exec_puts));
  strm_var_def("+", strm_cfunc_value(exec_plus));
  strm_var_def("-", strm_cfunc_value(exec_minus));
  strm_var_def("*", strm_cfunc_value(exec_mult));
  strm_var_def("/", strm_cfunc_value(exec_div));
  strm_var_def("<", strm_cfunc_value(exec_lt));
  strm_var_def("<=", strm_cfunc_value(exec_le));
  strm_var_def(">", strm_cfunc_value(exec_gt));
  strm_var_def(">=", strm_cfunc_value(exec_ge));
  strm_var_def("==", strm_cfunc_value(exec_eq));
  strm_var_def("!=", strm_cfunc_value(exec_neq));
  strm_var_def("|", strm_cfunc_value(exec_bar));
  strm_var_def("%", strm_cfunc_value(exec_mod));
  strm_var_def("fread", strm_cfunc_value(exec_fread));
  strm_var_def("fwrite", strm_cfunc_value(exec_fwrite));

  strm_seq_init(state);
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
  strm_lambda *lambda = task->data;
  strm_value ret = strm_nil_value();
  node_values* args = (node_values*)lambda->body->args;
  int n;
  strm_state c = {0};

  c.task = task;
  c.prev = lambda->state;
  assert(args->len == 1);
  strm_var_set(&c, (strm_string*)args->data[0], data);

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

  if (arrd->n == arrd->arr->len) {
    strm_task_close(task);
    return STRM_OK;
  }
  strm_emit(task, arrd->arr->ptr[arrd->n++], arr_exec);
  return STRM_OK;
}

static int
arr_finish(strm_task* task, strm_value data)
{
  struct array_data *d = task->data;
  free(d);
  return STRM_OK;
}

static int
cfunc_exec(strm_task* task, strm_value data)
{
  strm_value ret;
  exec_cfunc func = task->data;
  strm_state c = {0};

  if ((*func)(&c, 1, &data, &ret) == STRM_OK) {
    strm_emit(task, ret, NULL);
    return STRM_OK;
  }
  return STRM_NG;
}
