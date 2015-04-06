#include "strm.h"
#include "node.h"

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1

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
  else if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])+strm_value_int(args[1]));
    return 0;
  }
  else if (strm_num_p(args[0])) {
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
  else if (strm_num_p(args[0])) {
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

static int
exec_bar(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  assert(argc == 2);
  if (strm_int_p(args[0]) && strm_int_p(args[1])) {
    *ret = strm_int_value(strm_value_int(args[0])|strm_value_int(args[1]));
    return 0;
  }
  if (strm_task_p(args[0]) && strm_task_p(args[1])) {
    strm_connect(strm_value_task(args[0]), strm_value_task(args[1]));
    *ret = args[1];
    return 0;
  }
  node_raise(ctx, "type error");
  return 1;
}

typedef int (*exec_cfunc)(node_ctx*, int, strm_value*, strm_value*);

static int
exec_call(node_ctx* ctx, node_id id, int argc, strm_value* args, strm_value* ret)
{
  strm_value m = strm_var_get(id);

  if (m.vtype == STRM_VALUE_CFUNC) {
    return ((exec_cfunc)m.val.p)(ctx, argc, args, ret);
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
  case NODE_EMIT:
    break;
*/
  case NODE_IDENT:
    *val = strm_var_get(np->value.v.id);
    return 0;
  case NODE_IF:
    {
      strm_value v;
      node_if* nif = (node_if*)np;
      n = exec_expr(ctx, nif->cond, &v);
      if (n) return n;
      if (strm_value_bool(v)) {
        return exec_expr(ctx, nif->compstmt, val);
      }
      else if (nif->opt_else != NULL) {
        return exec_expr(ctx, nif->compstmt, val);
      }
      else {
        *val = strm_null_value();
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
  case NODE_CALL:
    {
      /* TODO: wip code of ident */
      node_call* ncall = (node_call*)np;
      if (ncall->ident != NULL) {
        int i;
        node_values* v0 = (node_values*)ncall->args;
        strm_value *args = malloc(sizeof(strm_value)*v0->len);

        for (i = 0; i < v0->len; i++) {
          n = exec_expr(ctx, v0->data[i], &args[i]);
          if (n) return n;
        }
        return exec_call(ctx, ncall->ident->value.v.id, i, args, val);
      }
      else {
        node_block* nblk = (node_block*)ncall;
        strm_value v;
        int n;
        n = exec_expr(ctx, nblk->compstmt, &v);
        if (n && ctx->exc->type == NODE_ERROR_RETURN) {
          *val = ctx->exc->arg;
          free(ctx->exc);
          return 0;
        }
      }
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = (node_return*)np;
      ctx->exc = malloc(sizeof(node_error));
      ctx->exc->type = NODE_ERROR_RETURN;
      n = exec_expr(ctx, nreturn->rv, &ctx->exc->arg);
      return n;
    }
    break;
  case NODE_STMTS:
    {
      int i, n;
      node_values* v = (node_values*)np;
      for (i = 0; i < v->len; i++) {
        n = exec_expr(ctx, v->data[i], val);
        if (n) return n;
      }
      return 0;
    }
    break;
  case NODE_VALUE:
    switch (np->value.t) {
    case NODE_VALUE_BOOL:
      *val = strm_bool_value(np->value.v.b);
      return 0;
    case NODE_VALUE_NIL:
      *val = strm_null_value();
      return 0;
    case NODE_VALUE_STRING:
      *val = strm_ptr_value(np->value.v.s);
      return 0;
    case NODE_VALUE_IDENT:
      *val = strm_ptr_value(np->value.v.id);
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
      fprintf(out, "%*s", str->len, str->ptr);
      break;
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
    switch (v.vtype) {
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

void
node_raise(node_ctx* ctx, const char* msg) {
  ctx->exc = malloc(sizeof(node_error));
  ctx->exc->type = NODE_ERROR_RUNTIME;
  ctx->exc->arg = strm_str_value(msg, strlen(msg));
}

static void
node_init(node_ctx* ctx)
{
  strm_var_def("STDIN", strm_task_value(strm_readio(0 /* stdin*/)));
  strm_var_def("STDOUT", strm_task_value(strm_writeio(1 /* stdout*/)));
  strm_var_def("STDERR", strm_task_value(strm_writeio(2 /* stdout*/)));
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
}

void strm_seq_init();

int
node_run(parser_state* p)
{
  strm_value v;

  node_init(&p->ctx);
  strm_seq_init(&p->ctx);
  exec_expr(&p->ctx, (node*)p->lval, &v);
  if (p->ctx.exc != NULL) {
    strm_value v;
    exec_cputs(&p->ctx, stderr, 1, &p->ctx.exc->arg, &v);
    /* TODO: garbage correct previous exception value */
    p->ctx.exc = NULL;
  }
  return 0;
}
