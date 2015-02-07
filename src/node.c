#include "strm.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

extern FILE *yyin, *yyout;
extern int yyparse(parser_state*);
extern int yydebug;

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

node*
node_value_new(node* v)
{
  /* TODO */
  return NULL;
}

node*
node_array_new()
{
  node_array* arr = malloc(sizeof(node_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  node* np = malloc(sizeof(node));
  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_ARRAY;
  np->value.v.p = arr;
  return np;
}

node*
node_array_of(node* np)
{
  if (np == NULL)
    np = node_array_new();
  return np;
}

void
node_array_add(node* arr, node* np)
{
  /* TODO: error check */
  node_array* arr0 = arr->value.v.p;
  if (arr0->len == arr0->max) {
    arr0->max = arr0->len + 10;
    arr0->data = realloc(arr0->data, sizeof(node*) * arr0->max);
  }
  arr0->data[arr0->len] = np;
  arr0->len++;
}

void
node_array_free(node* np)
{
  int i;
  node_array* arr0 = np->value.v.p;
  for (i = 0; i < arr0->len; i++)
    node_free(arr0->data[i]);
  free(arr0);
  free(np);
}

node*
node_pair_new(node* key, node* value)
{
  node_pair* npair = malloc(sizeof(node_pair));
  npair->key = key;
  npair->value = value;

  node* np = malloc(sizeof(node));
  np->type = NODE_PAIR;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = npair;
  return np;
}

node*
node_map_new()
{
  node_array* arr = malloc(sizeof(node_array));
  /* TODO: error check */
  arr->len = 0;
  arr->max = 0;
  arr->data = NULL;

  node* np = malloc(sizeof(node));
  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_MAP;
  np->value.v.p = arr;
  return np;
}

node*
node_map_of(node* np)
{
  if (np == NULL)
    np = node_map_new();
  return np;
}

void
node_map_free(node* np)
{
  int i;
  node_array* arr0 = np->value.v.p;
  for (i = 0; i < arr0->len; i++) {
    node* pair = arr0->data[i];
    node_pair* pair0 = pair->value.v.p;
    node_free(pair0->key);
    node_free(pair0->value);
    free(pair0);
  }
  free(arr0);
  free(np);
}

node*
node_let_new(node* lhs, node* rhs)
{
  node_let* nlet = malloc(sizeof(node_let));
  nlet->lhs = lhs;
  nlet->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_LET;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = nlet;
  return np;
}

node*
node_op_new(char* op, node* lhs, node* rhs)
{
  node_op* nop = malloc(sizeof(node_op));
  nop->lhs = lhs;
  nop->op = op;
  nop->rhs = rhs;

  node* np = malloc(sizeof(node));
  np->type = NODE_OP;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = nop;
  return np;
}

node*
node_block_new(node* args, node* compstmt)
{
  node_block* block = malloc(sizeof(node_block));
  block->args = args;
  block->compstmt = compstmt;

  node* np = malloc(sizeof(node));
  np->type = NODE_BLOCK;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = block;
  return np;
}

node*
node_call_new(node* cond, node* ident, node* args, node* blk)
{
  node_call* ncall = malloc(sizeof(node_call));
  ncall->cond = cond;
  ncall->ident = ident;
  ncall->args = args;
  ncall->blk = blk;

  node* np = malloc(sizeof(node));
  np->type = NODE_CALL;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = ncall;
  return np;
}

node*
node_double_new(strm_double d)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_DOUBLE;
  np->value.v.d = d;
  return np;
}

node*
node_string_new(strm_string s)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_STRING;
  np->value.v.s = strdup0(s);
  return np;
}

node*
node_string_len_new(strm_string s, size_t l)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_VALUE;
  np->value.t = STRM_VALUE_STRING;
  np->value.v.s = strndup0(s, l);
  return np;
}

node*
node_ident_new(strm_id id)
{
  node* np = malloc(sizeof(node));

  np->type = NODE_IDENT;
  np->value.t = STRM_VALUE_FIXNUM;
  np->value.v.id = id;
  return np;
}

strm_id
node_ident_of(char* s)
{
  /* TODO: get id of the identifier which named as s */
  return (strm_id) strdup0(s);
}

node*
node_nil()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_NIL, {0} } };
  return &nd;
}

node*
node_true()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_BOOL, {1} } };
  return &nd;
}

node*
node_false()
{
  static node nd = { NODE_VALUE, { STRM_VALUE_BOOL, {0} } };
  return &nd;
}

node*
node_if_new(node* cond, node* compstmt, node* opt_else)
{
  node_if* nif = malloc(sizeof(node_if));
  nif->cond = cond;
  nif->compstmt = compstmt;
  nif->opt_else = opt_else;

  node* np = malloc(sizeof(node));
  np->type = NODE_IF;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = nif;
  return np;
}


node*
node_emit_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_EMIT;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = value;
  return np;
}

node*
node_return_new(node* value)
{
  node* np = malloc(sizeof(node));
  np->type = NODE_RETURN;
  np->value.t = STRM_VALUE_USER;
  np->value.v.p = value;
  return np;
}

node*
node_break_new(node* value)
{
  static node nd = { NODE_BREAK };
  return &nd;
}

int
strm_parse_init(parser_state *p)
{
  p->nerr = 0;
  p->lval = NULL;
  p->fname = NULL;
  p->lineno = 1;
  p->tline = 1;
  p->ctx.env = kh_init(value);
  return 0;
}

int
strm_parse_file(parser_state* p, const char* fname)
{
  int r;
  FILE* fp = fopen(fname, "rb");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }
  r = strm_parse_input(p, fp, fname);
  fclose(fp);
  return r;
}

int
strm_parse_input(parser_state* p, FILE *f, const char *fname)
{
  int n;

  /* yydebug = 1; */
  yyin = f;
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
    node_array_free(np);
    break;
  case NODE_IF:
    node_free(((node_if*)np->value.v.p)->cond);
    node_free(((node_if*)np->value.v.p)->compstmt);
    node_free(((node_if*)np->value.v.p)->opt_else);
    free(np);
    break;
  case NODE_EMIT:
    node_free((node*) np->value.v.p);
    free(np);
    break;
  case NODE_OP:
    node_free(((node_op*) np->value.v.p)->lhs);
    node_free(((node_op*) np->value.v.p)->rhs);
    free(np);
    break;
  case NODE_BLOCK:
    node_free(((node_block*) np->value.v.p)->args);
    node_free(((node_block*) np->value.v.p)->compstmt);
    free(np);
    break;
  case NODE_CALL:
    node_free(((node_call*) np->value.v.p)->cond);
    node_free(((node_call*) np->value.v.p)->ident);
    node_free(((node_call*) np->value.v.p)->args);
    node_free(((node_call*) np->value.v.p)->blk);
    free(np);
    break;
  case NODE_IDENT:
    free(np);
    break;
  case NODE_VALUE:
    switch (np->value.t) {
    case STRM_VALUE_DOUBLE:
      free(np);
      break;
    case STRM_VALUE_STRING:
      free(np->value.v.s);
      free(np);
      break;
    case STRM_VALUE_BOOL:
      break;
    case STRM_VALUE_NIL:
      break;
    case STRM_VALUE_ARRAY:
      node_array_free(np);
      break;
    case STRM_VALUE_MAP:
      node_map_free(np);
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
strm_parse_free(parser_state* p)
{
  node_free(p->lval);
}

strm_value*
node_expr(strm_ctx* ctx, node* np)
{
  if (!np) {
    return NULL;
  }

  switch (np->type) {
/*
  case NODE_ARGS:
    break;
  case NODE_IF:
    break;
  case NODE_EMIT:
    break;
  case NODE_OP:
    break;
  case NODE_BLOCK:
    break;
  case NODE_IDENT:
    break;
*/
  case NODE_OP:
    {
      node_op* nop = np->value.v.p;
      strm_value* lhs = node_expr(ctx, nop->lhs);
	  if (*nop->op == '+' && *(nop->op+1) == '\0') {
		strm_value* rhs = node_expr(ctx, nop->rhs);
		if (lhs->t == STRM_VALUE_STRING && rhs->t == STRM_VALUE_STRING) {
		  strm_value* new = malloc(sizeof(strm_value));
		  char *p = malloc(strlen(lhs->v.s) + strlen(rhs->v.s) + 1);
		  strcpy(p, lhs->v.s);
		  strcat(p, rhs->v.s);
		  new->t = STRM_VALUE_STRING;
		  new->v.s = p;
		  return new;
		} else if (lhs->t == STRM_VALUE_DOUBLE && rhs->t == STRM_VALUE_DOUBLE) {
		  strm_value* new = malloc(sizeof(strm_value));
		  new->t = STRM_VALUE_DOUBLE;
		  new->v.d = lhs->v.d + rhs->v.d;
		  return new;
		}
	  }
	  if (*nop->op == '-' && *(nop->op+1) == '\0') {
		strm_value* rhs = node_expr(ctx, nop->rhs);
		if (lhs->t == STRM_VALUE_DOUBLE && rhs->t == STRM_VALUE_DOUBLE) {
		  strm_value* new = malloc(sizeof(strm_value));
		  new->t = STRM_VALUE_DOUBLE;
		  new->v.d = lhs->v.d - rhs->v.d;
		  return new;
		}
	  }
	  if (*nop->op == '*' && *(nop->op+1) == '\0') {
		strm_value* rhs = node_expr(ctx, nop->rhs);
		if (lhs->t == STRM_VALUE_DOUBLE && rhs->t == STRM_VALUE_DOUBLE) {
		  strm_value* new = malloc(sizeof(strm_value));
		  new->t = STRM_VALUE_DOUBLE;
		  new->v.d = lhs->v.d * rhs->v.d;
		  return new;
		}
	  }
	  if (*nop->op == '/' && *(nop->op+1) == '\0') {
		strm_value* rhs = node_expr(ctx, nop->rhs);
		if (lhs->t == STRM_VALUE_DOUBLE && rhs->t == STRM_VALUE_DOUBLE) {
		  strm_value* new = malloc(sizeof(strm_value));
		  new->t = STRM_VALUE_DOUBLE;
          /* TODO: zero divide */
		  new->v.d = lhs->v.d / rhs->v.d;
		  return new;
		}
	  }
      /* TODO: invalid operator */
    }
    break;
  case NODE_CALL:
    {
      /* TODO: wip code of ident */
      node_call* ncall = np->value.v.p;
      if (ncall->ident != NULL) {
        khint_t k = kh_get(value, ctx->env, (char*) ncall->ident->value.v.id);
        if (k != kh_end(ctx->env)) {
          strm_value* v = kh_value(ctx->env, k);
          if (v->t == STRM_VALUE_CFUNC) {
            ((strm_cfunc) v->v.p)(ctx, ncall->args->value.v.p);
          }
        } else {
          strm_raise(ctx, "function not found");
        }
      }
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

strm_value*
strm_puts(strm_ctx* ctx, strm_array* args) {
  int i;
  for (i = 0; i < args->len; i++) {
    strm_value* v;
    if (i != 0)
      printf(", ");
    v = node_expr(ctx, args->data[i]);
    if (v != NULL) {
      switch (v->t) {
      case STRM_VALUE_DOUBLE:
        printf("%f", v->v.d);
        break;
      case STRM_VALUE_STRING:
        printf("'%s'", v->v.s);
        break;
      case STRM_VALUE_NIL:
        printf("nil");
        break;
      case STRM_VALUE_BOOL:
        printf(v->v.b ? "true" : "false");
        break;
      default:
        printf("<%p>", v->v.p);
        break;
      }
    } else {
        printf("nil");
    }
  }
  printf("\n");
  return NULL;
}

int
strm_run(parser_state* p)
{
  int r;
  khiter_t k;

  static strm_value vputs;
  k = kh_put(value, p->ctx.env, "puts", &r);
  vputs.t = STRM_VALUE_CFUNC;
  vputs.v.p = strm_puts;

  kh_value(p->ctx.env, k) = &vputs;

  int i;
  node_array* arr0 = ((node*)p->lval)->value.v.p;
  for (i = 0; i < arr0->len; i++)
    node_expr(&p->ctx, arr0->data[i]);
  return 0;
}

void
strm_raise(strm_ctx* ctx, const char* msg) {
  ctx->exc = malloc(sizeof(strm_value));
  ctx->exc->t = STRM_VALUE_STRING;
  ctx->exc->v.s = strdup0(msg);
}
