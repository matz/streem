#include "strm.h"
#include "node.h"

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1
#define NODE_ERROR_SKIP 2

static int
exec_plus(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
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
    return 0;
  }
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])+strm_value_int(args[1]));
    return 0;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])+strm_value_flt(args[1]));
    return 0;
  }
  return 1;
}

static int
exec_minus(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  if (argc == 1) {
    if (strm_int_p(args[0])) {
      *ret = strm_int_value(-strm_value_int(args[0]));
      return 0;
    }
    if (strm_flt_p(args[0])) {
      *ret = strm_flt_value(-strm_value_flt(args[0]));
      return 0;
    }
    return 1;
  }
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])-strm_value_int(args[1]));
    return 0;
  }
  if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])-strm_value_flt(args[1]));
    return 0;
  }
  return 1;
}

static int
exec_mult(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])*strm_value_int(args[1]));
    return 0;
  }
  else if (strm_num_p(args[0])) {
    *ret = strm_flt_value(strm_value_flt(args[0])*strm_value_flt(args[1]));
    return 0;
  }
  return 1;
}

static int
exec_div(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_flt_value(strm_value_flt(args[0])/strm_value_flt(args[1]));
  return 0;
}

static int
exec_gt(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>strm_value_flt(args[1]));
  return 0;
}

static int
exec_ge(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])>=strm_value_flt(args[1]));
  return 0;
}

static int
exec_lt(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<strm_value_flt(args[1]));
  return 0;
}

static int
exec_le(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_flt(args[0])<=strm_value_flt(args[1]));
  return 0;
}

static int
exec_eq(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(strm_value_eq(args[0], args[1]));
  return 0;
}

static int
exec_neq(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  *ret = strm_bool_value(!strm_value_eq(args[0], args[1]));
  return 0;
}

static void blk_exec(strm_task *strm, strm_value data);

struct array_data {
  int n;
  strm_array* arr;
};

static void arr_exec(strm_task *strm, strm_value data);
static void arr_finish(strm_task *strm, strm_value data);

static int
exec_bar(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  strm_value lhs, rhs;

  assert(argc == 2);
  /* int x int */
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])|strm_value_int(args[1]));
    return 0;
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
    lhs = strm_task_value(strm_alloc_stream(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }
  /* lhs: array */
  else if (strm_array_p(lhs)) {
    struct array_data *arrd = malloc(sizeof(struct array_data));
    arrd->arr = strm_value_array(lhs);
    arrd->n = 0;
    lhs = strm_task_value(strm_alloc_stream(strm_task_prod, arr_exec, arr_finish, (void*)arrd));
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
    rhs = strm_task_value(strm_alloc_stream(strm_task_filt, blk_exec, NULL, (void*)lmbd));
  }

  /* task x task */
  if (strm_task_p(lhs) && strm_task_p(rhs)) {
    if (lhs.val.p == NULL || rhs.val.p == NULL) {
      node_raise(ctx, "task error");
      return 1;
    }
    strm_connect(strm_value_task(lhs), strm_value_task(rhs));
    *ret = rhs;
    return 0;
  }

  node_raise(ctx, "type error");
  return 1;
}

static int
exec_mod(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])%strm_value_int(args[1]));
    return 0;
  }
  else if (strm_num_p(args[0])) {
    *ret = strm_flt_value((int)strm_value_flt(args[0])%(int)strm_value_flt(args[1]));
    return 0;
  }
  return 1;
}

typedef int (*exec_cfunc)(node_ctx*, int, strm_value*, strm_value*);
static int exec_expr(node_ctx* ctx, node* np, strm_value* val);

static int
exec_call(node_ctx* ctx, strm_string *name, int argc, strm_value* argv, strm_value* ret)
{
  int n;
  strm_value m;

  n = strm_var_get(ctx, name, &m);
  if (n == 0) {
    switch (m.type) {
    case STRM_VALUE_CFUNC:
      return ((exec_cfunc)m.val.p)(ctx, argc, argv, ret);
    case STRM_VALUE_PTR:
      {
        strm_lambda* lambda = strm_value_ptr(m);
        node_lambda* nlbd = lambda->body;
        node_values* args = (node_values*)nlbd->args;
        node_ctx c = {0};
        int i;

        c.prev = lambda->ctx;
        if ((args == NULL && argc != 0) &&
            (args->len != argc)) return 1;
        for (i=0; i<argc; i++) {
          n = strm_var_set(&c, (strm_string*)args->data[i], argv[i]);
          if (n) return n;
        }
        n = exec_expr(&c, nlbd->compstmt, ret);
        if (c.exc && c.exc->type == NODE_ERROR_RETURN) {
          *ret = c.exc->arg;
          return 0;
        }
        return n;
      }
    default:
      break;
    }
  }
  node_raise(ctx, "function not found");
  return 1;
}

static int
exec_expr(node_ctx* ctx, node* np, strm_value* val)
{
  int n;

  if (np == NULL) {
    return 1;
  }

  switch (np->type) {
/*
  case NODE_ARGS:
    break;
*/
  case NODE_SKIP:
    {
       ctx->exc = malloc(sizeof(node_error));
       ctx->exc->type = NODE_ERROR_SKIP;
       ctx->exc->arg = strm_nil_value();
       return 0;
    }
  case NODE_EMIT:
    {
      int i, n;
      node_values* v0;

      if (!ctx->strm) {
        node_raise(ctx, "failed to emit");
      }
      v0 = (node_values*)np->value.v.p;

      for (i = 0; i < v0->len; i++) {
        n = exec_expr(ctx, v0->data[i], val);
        if (n) return n;
        strm_emit(ctx->strm, *val, NULL);
      }
      return 0;
    }
    break;
  case NODE_LET:
    {
      node_let *nlet = (node_let*)np;
      n = exec_expr(ctx, nlet->rhs, val);
      if (n) {
        node_raise(ctx, "failed to assign");
        return n;
      }
      return strm_var_set(ctx, nlet->lhs, *val);
    }
  case NODE_ARRAY:
    {
      node_values* v0 = (node_values*)np;
      strm_array *arr = strm_ary_new(NULL, v0->len);
      strm_value *ptr = (strm_value*)arr->ptr;
      int i=0;

      for (i = 0; i < v0->len; i++, ptr++) {
        n = exec_expr(ctx, v0->data[i], ptr);
        if (n) return n;
      }
      *val = strm_ptr_value(arr);
      return 0;
    }
  case NODE_IDENT:
    n = strm_var_get(ctx, np->value.v.s, val);
    if (n) {
      node_raise(ctx, "failed to reference variable");
    }
    return n;
  case NODE_IF:
    {
      strm_value v;
      node_if* nif = (node_if*)np;
      n = exec_expr(ctx, nif->cond, &v);
      if (n) return n;
      if (strm_value_bool(v) && v.val.i) {
        return exec_expr(ctx, nif->then, val);
      }
      else if (nif->opt_else != NULL) {
        return exec_expr(ctx, nif->opt_else, val);
      }
      else {
        *val = strm_nil_value();
        return 0;
      }
    }
    break;
  case NODE_OP:
    {
      node_op* nop = (node_op*)np;
      strm_value args[2];
      int i=0;

      if (nop->lhs) {
        n = exec_expr(ctx, nop->lhs, &args[i++]);
        if (n) return n;
      }
      if (nop->rhs) {
        n = exec_expr(ctx, nop->rhs, &args[i++]);
        if (n) return n;
      }
      return exec_call(ctx, nop->op, i, args, val);
    }
    break;
  case NODE_LAMBDA:
    {
      struct strm_lambda* lambda = malloc(sizeof(strm_lambda));

      if (!lambda) return 1;
      lambda->type = STRM_OBJ_LAMBDA;
      lambda->body = (node_lambda*)np;
      lambda->ctx = ctx;
      *val = strm_ptr_value(lambda);
      return 0;
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
        n = exec_expr(ctx, v0->data[i], &args[i]);
        if (n) return n;
      }
      return exec_call(ctx, ncall->ident, i, args, val);
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = (node_return*)np;
      node_values* args = (node_values*)nreturn->rv;

      ctx->exc = malloc(sizeof(node_error));
      ctx->exc->type = NODE_ERROR_RETURN;
      switch (args->len) {
      case 0:
        ctx->exc->arg = strm_nil_value();
        break;
      case 1:
        n = exec_expr(ctx, args->data[0], &ctx->exc->arg);
        if (n) return n;
        break;
      default:
        {
          strm_array* ary = strm_ary_new(NULL, args->len);
          size_t i;

          for (i=0; i<args->len; i++) {
            n = exec_expr(ctx, args->data[i], (strm_value*)&ary->ptr[i]);
            if (n) return n;
          }
        }
        break;
      }
      return 1;
    }
    break;
  case NODE_STMTS:
    {
      int i;
      node_values* v = (node_values*)np;
      for (i = 0; i < v->len; i++) {
        n = exec_expr(ctx, v->data[i], val);
        if (ctx->exc != NULL) return 1;
        if (n) return n;
      }
    }
    return 0;
  case NODE_VALUE:
    switch (np->value.t) {
    case NODE_VALUE_BOOL:
      *val = strm_bool_value(np->value.v.b);
      return 0;
    case NODE_VALUE_NIL:
      *val = strm_nil_value();
      return 0;
    case NODE_VALUE_STRING:
    case NODE_VALUE_IDENT:
      *val = strm_ptr_value(np->value.v.s);
      return 0;
    case NODE_VALUE_DOUBLE:
      *val = strm_flt_value(np->value.v.d);
      return 0;
    case NODE_VALUE_INT:
      *val = strm_int_value(np->value.v.i);
      return 0;
      /* following type should not be evaluated */
    case NODE_VALUE_ERROR:
    case NODE_VALUE_USER:
    default:
      return 1;
    }
  default:
    break;
  }
  return 1;
}

static int
cputs_ptr(node_ctx* ctx, FILE* out, struct strm_object *obj)
{
  if (obj == NULL) {
    fprintf(out, "nil");
    return 0;
  }
  switch (obj->type) {
  case STRM_OBJ_ARRAY:
  case STRM_OBJ_LIST:
    fprintf(out, "<list:%p>", obj);
    break;
  case STRM_OBJ_MAP:
    fprintf(out, "<map:%p>", obj);
    break;
  case STRM_OBJ_STRING:
    {
      strm_string* str = (strm_string*)obj;
      fprintf(out, "%*s", (int)str->len, str->ptr);
    }
    break;
  case STRM_OBJ_IO:
    {
      strm_io* io = (strm_io*)obj;
      fprintf(out, "<io: fd=%d mode=%d>", io->fd, io->mode);
    }
    break;
  default:
    fprintf(out, "<%p>", obj);
    break;
  }
  return 0;
}

static int
exec_cputs(node_ctx* ctx, FILE* out, int argc, strm_value* args, strm_value *ret)
{
  int i;
  for (i = 0; i < argc; i++) {
    strm_value v;
    if (i != 0)
      fprintf(out, ", ");
    v = args[i];
    switch (v.type) {
    case STRM_VALUE_BOOL:
      fprintf(out, v.val.i ? "true" : "false");
      break;
    case STRM_VALUE_INT:
      fprintf(out, "%ld", v.val.i);
      break;
    case STRM_VALUE_FLT:
      fprintf(out, "%g", v.val.f);
      break;
    case STRM_VALUE_PTR:
      cputs_ptr(ctx, out, v.val.p);
      break;
    case STRM_VALUE_CFUNC:
      fprintf(out, "<cfunc:%p>", v.val.p);
      break;
    case STRM_VALUE_BLK:
      fprintf(out, "<blk:%p>", v.val.p);
      break;
    case STRM_VALUE_TASK:
      fprintf(out, "<task:%p>", v.val.p);
      break;
    }
  }
  fprintf(out, "\n");
  return 0;
}

static int
exec_puts(node_ctx* ctx, int argc, strm_value* args, strm_value *ret) {
  return exec_cputs(ctx, stdout, argc, args, ret);
}

#include <fcntl.h>

static int
exec_fread(node_ctx* ctx, int argc, strm_value* args, strm_value *ret)
{
  int fd;
  strm_string *path;

  assert(argc == 1);
  assert(strm_str_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(path->ptr, O_RDONLY);
  if (fd < 0) return 1;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_READ));
  return 0;
}

static int
exec_fwrite(node_ctx* ctx, int argc, strm_value* args, strm_value *ret)
{
  int fd;
  strm_string *path;

  assert(argc == 1);
  assert(strm_str_p(args[0]));
  path = strm_value_str(args[0]);
  fd = open(path->ptr, O_WRONLY|O_CREAT, 0644);
  if (fd < 0) return 1;
  *ret = strm_ptr_value(strm_io_new(fd, STRM_IO_WRITE));
  return 0;
}

void
node_raise(node_ctx* ctx, const char* msg) {
  ctx->exc = malloc(sizeof(node_error));
  ctx->exc->type = NODE_ERROR_RUNTIME;
  ctx->exc->arg = strm_str_value(msg, strlen(msg));
}

static void
node_init(node_ctx* ctx)
{
  strm_var_def("STDIN", strm_ptr_value(strm_io_new(0, STRM_IO_READ)));
  strm_var_def("STDOUT", strm_ptr_value(strm_io_new(1, STRM_IO_WRITE)));
  strm_var_def("STDERR", strm_ptr_value(strm_io_new(2, STRM_IO_WRITE)));
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
}

void strm_seq_init(node_ctx* ctx);
void strm_socket_init(node_ctx* ctx);

int
node_run(parser_state* p)
{
  strm_value v;
  node_ctx *ctx = malloc(sizeof(node_ctx));

  memset(ctx, 0, sizeof(node_ctx));
  node_init(ctx);

  strm_seq_init(ctx);
  strm_socket_init(ctx);

  exec_expr(ctx, (node*)p->lval, &v);
  if (ctx->exc != NULL) {
    if (ctx->exc->type != NODE_ERROR_RETURN) {
      strm_value v;
      exec_cputs(ctx, stderr, 1, &ctx->exc->arg, &v);
      /* TODO: garbage correct previous exception value */
      ctx->exc = NULL;
    }
  }
  return 0;
}

static void
blk_exec(strm_task *strm, strm_value data)
{
  strm_lambda *lambda = strm->data;
  strm_value ret = strm_nil_value();
  node_values* args = (node_values*)lambda->body->args;
  int n;
  node_ctx c = {0};

  c.strm = strm;
  c.prev = lambda->ctx;
  assert(args->len == 1);
  strm_var_set(&c, (strm_string*)args->data[0], data);

  n = exec_expr(&c, lambda->body->compstmt, &ret);
  if (n) return;
  if (lambda->ctx->exc) {
    if (lambda->ctx->exc->type == NODE_ERROR_RETURN) {
      ret = lambda->ctx->exc->arg;
      free(lambda->ctx->exc);
      lambda->ctx->exc = NULL;
    } else {
      return;
    }
  }
  strm_emit(strm, ret, NULL);
}

static void
arr_exec(strm_task *strm, strm_value data)
{
  struct array_data *arrd = strm->data;
  strm_emit(strm, arrd->arr->ptr[arrd->n++], NULL);
  if (arrd->n == arrd->arr->len)
    strm_close(strm);
}

static void
arr_finish(strm_task *strm, strm_value data)
{
  struct array_data *d = strm->data;
  free(d);
}
