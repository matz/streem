#include "strm.h"
#include "node.h"

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1

static int exec_expr(node_ctx*, node*, strm_value*);

static int
exec_expr_stmt(node_ctx* ctx, node* np, strm_value* val)
{
  int i, n;
  node_values* v = np->value.v.p;
  for (i = 0; i < v->len; i++) {
    n = exec_expr(ctx, v->data[i], val);
    if (n) return n;
  }
  return 0;
}

static int
exec_plus(strm_value* a, strm_value* b, strm_value* c)
{
  if (strm_str_p(*a)) {
    strm_string *str1 = strm_value_str(*a);
    strm_string *str2 = strm_value_str(*b);
    strm_string *str3 = strm_str_new(NULL, str1->len + str2->len);
    char *p;

    p = (char*)str3->ptr;
    memcpy(p, str1->ptr, str1->len);
    memcpy(p+str1->len, str2->ptr, str2->len);
    p[str3->len] = '\0';
    *c = strm_ptr_value(str3);
    return 0;
  }
  else if (strm_int_p(*a) && strm_int_p(*b)) {
    *c = strm_int_value(strm_value_int(*a)+strm_value_int(*b));
    return 0;
  }
  else if (strm_num_p(*a)) {
    *c = strm_flt_value(strm_value_flt(*a)+strm_value_flt(*b));
    return 0;
  }
  return 1;
}

static int
exec_minus(strm_value* a, strm_value* b, strm_value* c)
{
  if (strm_int_p(*a) && strm_int_p(*b)) {
    *c = strm_int_value(strm_value_int(*a)-strm_value_int(*b));
    return 0;
  }
  else if (strm_num_p(*a)) {
    *c = strm_flt_value(strm_value_flt(*a)-strm_value_flt(*b));
    return 0;
  }
  return 1;
}

static int
exec_mul(strm_value* a, strm_value* b, strm_value* c)
{
  if (strm_int_p(*a) && strm_int_p(*b)) {
    *c = strm_int_value(strm_value_int(*a)*strm_value_int(*b));
    return 0;
  }
  else if (strm_num_p(*a)) {
    *c = strm_flt_value(strm_value_flt(*a)*strm_value_flt(*b));
    return 0;
  }
  return 1;
}

static int
exec_div(strm_value* a, strm_value* b, strm_value* c)
{
  *c = strm_flt_value(strm_value_flt(*a)/strm_value_flt(*b));
  return 0;
}

static int
exec_gt(strm_value* a, strm_value* b, strm_value* c)
{
  *c = strm_bool_value(strm_value_flt(*a)>strm_value_flt(*b));
  return 0;
}

static int
exec_ge(strm_value* a, strm_value* b, strm_value* c)
{
  *c = strm_bool_value(strm_value_flt(*a)>=strm_value_flt(*b));
  return 0;
}

static int
exec_lt(strm_value* a, strm_value* b, strm_value* c)
{
  *c = strm_bool_value(strm_value_flt(*a)<strm_value_flt(*b));
  return 0;
}

static int
exec_le(strm_value* a, strm_value* b, strm_value* c)
{
  *c = strm_bool_value(strm_value_flt(*a)<=strm_value_flt(*b));
  return 0;
}

typedef int (*exec_cfunc)(node_ctx*, int, strm_value*, strm_value*);

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
      node_if* nif = np->value.v.p;
      n = exec_expr(ctx, nif->cond, &v);
      if (n) return n;
      if (strm_value_bool(v)) {
        return exec_expr_stmt(ctx, nif->compstmt, val);
      }
      else if (nif->opt_else != NULL) {
        return exec_expr_stmt(ctx, nif->compstmt, val);
      }
      else {
        *val = strm_null_value();
        return 0;
      }
    }
    break;
  case NODE_OP:
    {
      node_op* nop = np->value.v.p;
      strm_value lhs, rhs;

      n = exec_expr(ctx, nop->lhs, &lhs);
      if (n) return n;
      n = exec_expr(ctx, nop->rhs, &rhs);
      if (n) return n;
      if (*nop->op == '+' && *(nop->op+1) == '\0') {
        return exec_plus(&lhs, &rhs, val);
      }
      if (*nop->op == '-' && *(nop->op+1) == '\0') {
        return exec_minus(&lhs, &rhs, val);
      }
      if (*nop->op == '*' && *(nop->op+1) == '\0') {
        return exec_mul(&lhs, &rhs, val);
      }
      if (*nop->op == '/' && *(nop->op+1) == '\0') {
        return exec_div(&lhs, &rhs, val);
      }
      if (*nop->op == '<') {
        if (*(nop->op+1) == '=')
          return exec_le(&lhs, &rhs, val);
        else
          return exec_lt(&lhs, &rhs, val);
      }
      if (*nop->op == '>') {
        if (*(nop->op+1) == '=')
          return exec_ge(&lhs, &rhs, val);
        else
          return exec_gt(&lhs, &rhs, val);
      }
      if (*nop->op == '=' && (*(nop->op+1)) == '=') {
        *val = strm_bool_value(strm_value_eq(rhs, lhs));
        return 0;
      }
      if (*nop->op == '!' && (*(nop->op+1)) == '=') {
        *val = strm_bool_value(!strm_value_eq(rhs, lhs));
        return 0;
      }
      /* TODO: invalid operator */
      node_raise(ctx, "invalid operator");
    }
    break;
  case NODE_CALL:
    {
      /* TODO: wip code of ident */
      node_call* ncall = np->value.v.p;
      if (ncall->ident != NULL) {
        strm_value v = strm_var_get(ncall->ident->value.v.id);

        if (v.vtype == STRM_VALUE_CFUNC) {
          node_values* v0 = ncall->args->value.v.p;
          strm_value *args = malloc(sizeof(strm_value)*v0->len);
          int i;

          for (i = 0; i < v0->len; i++) {
            n = exec_expr(ctx, v0->data[i], &args[i]);
            if (n) return n;
          }
          return ((exec_cfunc)v.val.p)(ctx, v0->len, args, val);
        }
        else {
          node_raise(ctx, "function not found");
        }
      }
      else {
        node_block* nblk = ncall->blk->value.v.p;
        strm_value v;
        int n;
        n = exec_expr_stmt(ctx, nblk->compstmt, &v);
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
      node_return* nreturn = np->value.v.p;
      ctx->exc = malloc(sizeof(node_error));
      ctx->exc->type = NODE_ERROR_RETURN;
      n = exec_expr(ctx, nreturn->rv, &ctx->exc->arg);
      return n;
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
    case NODE_VALUE_ARRAY:
    case NODE_VALUE_MAP:
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
  strm_var_def("puts", strm_cfunc_value(exec_puts));
  strm_var_def("STDIN", strm_task_value(strm_readio(0 /* stdin*/)));
  strm_var_def("STDOUT", strm_task_value(strm_writeio(1 /* stdout*/)));
  strm_var_def("STDERR", strm_task_value(strm_writeio(2 /* stdout*/)));
}

int
node_run(parser_state* p)
{
  strm_value v;

  node_init(&p->ctx);
  exec_expr_stmt(&p->ctx, (node*)p->lval, &v);
  if (p->ctx.exc != NULL) {
    strm_value v;
    exec_cputs(&p->ctx, stderr, 1, &p->ctx.exc->arg, &v);
    /* TODO: garbage correct previous exception value */
    p->ctx.exc = NULL;
  }
  return 0;
}
