#include "strm.h"
#include "node.h"

#define NODE_ERROR_RUNTIME 0
#define NODE_ERROR_RETURN 1

node_value* node_expr(node_ctx*, node*);

node_value*
node_expr_stmt(node_ctx* ctx, node* np)
{
  int i;
  node_values* v = np->value.v.p;
  node_value* val = NULL;
  for (i = 0; i < v->len; i++) {
    if (ctx->exc != NULL) {
      return NULL;
    }
    /* TODO: garbage correct previous value in this loop */
    val = node_expr(ctx, v->data[i]);
  }
  return val;
}

node_value*
node_expr(node_ctx* ctx, node* np)
{
  if (ctx->exc != NULL) {
    return NULL;
  }

  if (np == NULL) {
    return NULL;
  }

  switch (np->type) {
/*
  case NODE_ARGS:
    break;
  case NODE_EMIT:
    break;
  case NODE_IDENT:
    break;
*/
  case NODE_IF:
    {
      node_if* nif = np->value.v.p;
      node_value* v = node_expr(ctx, nif->cond);
      if (ctx->exc != NULL) {
        return NULL;
      }
      if (v->t == NODE_VALUE_NIL || v->v.p == NULL ||
          (v->t == NODE_VALUE_STRING && v->v.s->len == 0)) {
        if (nif->opt_else != NULL)
          node_expr_stmt(ctx, nif->opt_else);
      } else {
        node_expr_stmt(ctx, nif->compstmt);
      }
    }
    break;
  case NODE_OP:
    {
      node_op* nop = np->value.v.p;
      node_value* lhs = node_expr(ctx, nop->lhs);
      if (ctx->exc != NULL) return NULL;
      if (*nop->op == '+' && *(nop->op+1) == '\0') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_STRING && rhs->t == NODE_VALUE_STRING) {
          node_value* new = malloc(sizeof(node_value));
          char* p;
          new->v.s = strm_str_new(NULL, lhs->v.s->len + rhs->v.s->len);
          new->t = NODE_VALUE_STRING;
          p = (char*)new->v.s->ptr;
          memcpy(p, lhs->v.s->ptr, lhs->v.s->len);
          memcpy(p+lhs->v.s->len, rhs->v.s, rhs->v.s->len);
          p[new->v.s->len] = '\0';
          return new;
        } else if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_DOUBLE;
          new->v.d = lhs->v.d + rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '-' && *(nop->op+1) == '\0') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_DOUBLE;
          new->v.d = lhs->v.d - rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '*' && *(nop->op+1) == '\0') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_DOUBLE;
          new->v.d = lhs->v.d * rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '/' && *(nop->op+1) == '\0') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_DOUBLE;
          /* TODO: zero divide */
          new->v.d = lhs->v.d / rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '<') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_BOOL;
          if (*(nop->op+1) == '=')
            new->v.b = lhs->v.d <= rhs->v.d;
          else
            new->v.b = lhs->v.d < rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '>') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_BOOL;
          if (*(nop->op+1) == '=')
            new->v.b = lhs->v.d >= rhs->v.d;
          else
            new->v.b = lhs->v.d > rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '=' && (*(nop->op+1)) == '=') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_BOOL;
          new->v.b = lhs->v.d == rhs->v.d;
          return new;
        }
      }
      if (*nop->op == '!' && (*(nop->op+1)) == '=') {
        node_value* rhs = node_expr(ctx, nop->rhs);
        if (ctx->exc != NULL) return NULL;
        if (lhs->t == NODE_VALUE_DOUBLE && rhs->t == NODE_VALUE_DOUBLE) {
          node_value* new = malloc(sizeof(node_value));
          new->t = NODE_VALUE_BOOL;
          new->v.b = lhs->v.d != rhs->v.d;
          return new;
        }
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
          node_values* v1 = node_values_new();
          int i;

          for (i = 0; i < v0->len; i++) {
            node_values_add(v1, node_expr(ctx, v0->data[i]));
          }
          ((node_cfunc)v.val.p)(ctx, v1);
        }
        else {
          node_raise(ctx, "function not found");
        }
      }
      else {
        node_block* nblk = ncall->blk->value.v.p;
        node_expr_stmt(ctx, nblk->compstmt);
        if (ctx->exc != NULL && ctx->exc->type == NODE_ERROR_RETURN) {
          node_value* arg = ctx->exc->arg;
          free(ctx->exc);
          ctx->exc = NULL;
          return arg;
        }
      }
    }
    break;
  case NODE_RETURN:
    {
      node_return* nreturn = np->value.v.p;
      ctx->exc = malloc(sizeof(node_error));
      ctx->exc->type = NODE_ERROR_RETURN;
      ctx->exc->arg = node_expr(ctx, nreturn->rv);
      return NULL;
    }
    break;
  case NODE_VALUE:
    return &np->value;
    break;
  default:
    break;
  }
  return NULL;
}

node_value*
node_cputs(node_ctx* ctx, FILE* out, node_values* args) {
  int i;
  for (i = 0; i < args->len; i++) {
    node_value* v;
    if (i != 0)
      fprintf(out, ", ");
    v = args->data[i];
    if (v != NULL) {
      switch (v->t) {
      case NODE_VALUE_INT:
        fprintf(out, "%ld", v->v.i);
        break;
      case NODE_VALUE_DOUBLE:
        fprintf(out, "%g", v->v.d);
        break;
      case NODE_VALUE_STRING:
        fprintf(out, "'%*s'", v->v.s->len, v->v.s->ptr);
        break;
      case NODE_VALUE_NIL:
        fprintf(out, "nil");
        break;
      case NODE_VALUE_BOOL:
        fprintf(out, v->v.b ? "true" : "false");
        break;
      case NODE_VALUE_ERROR:
        fprintf(out, "'%*s'", v->v.s->len, v->v.s->ptr);
        break;
      default:
        fprintf(out, "<%p>", v->v.p);
        break;
      }
    } else {
      fprintf(out, "nil");
    }
  }
  fprintf(out, "\n");
  return NULL;
}

node_value*
node_puts(node_ctx* ctx, node_values* args) {
  return node_cputs(ctx, stdout, args);
}

void
node_raise(node_ctx* ctx, const char* msg) {
  ctx->exc = malloc(sizeof(node_error));
  ctx->exc->type = NODE_ERROR_RUNTIME;
  ctx->exc->arg = malloc(sizeof(node_value));
  ctx->exc->arg->t = NODE_VALUE_ERROR;
  ctx->exc->arg->v.s = strm_str_new(msg, strlen(msg));
}

void
node_init(node_ctx* ctx)
{
  strm_var_def("puts", strm_cfunc_value(node_puts));
  strm_var_def("STDIN", strm_task_value(strm_readio(0 /* stdin*/)));
  strm_var_def("STDOUT", strm_task_value(strm_writeio(1 /* stdout*/)));
  strm_var_def("STDERR", strm_task_value(strm_writeio(2 /* stdout*/)));
}

int
node_run(parser_state* p)
{
  node_init(&p->ctx);
  node_expr_stmt(&p->ctx, (node*)p->lval);
  if (p->ctx.exc != NULL) {
    node_values* v = node_values_new();
    node_values_add(v, p->ctx.exc->arg);
    node_cputs(&p->ctx, stderr, v);
    /* TODO: garbage correct previous exception value */
    p->ctx.exc = NULL;
  }
  return 0;
}
