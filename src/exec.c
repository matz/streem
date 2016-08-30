#include "strm.h"
#include "node.h"

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
exec_eq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "vv", &x, &y);
  *ret = strm_bool_value(strm_value_eq(x, y));
  return STRM_OK;
}

static int
exec_neq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "vv", &x, &y);
  *ret = strm_bool_value(!strm_value_eq(x, y));
  return STRM_OK;
}

static int blk_exec(strm_stream* strm, strm_value data);

struct array_data {
  int n;
  strm_array arr;
};

static int arr_exec(strm_stream* strm, strm_value data);
static int cfunc_exec(strm_stream* strm, strm_value data);
static int cfunc_closer(strm_stream* strm, strm_value data) { return STRM_OK; }

int
strm_connect(strm_stream* strm, strm_value src, strm_value dst, strm_value* ret)
{
  /* src: io */
  if (strm_io_p(src)) {
    src = strm_stream_value(strm_io_stream(src, STRM_IO_READ));
  }
  /* src: lambda */
  else if (strm_lambda_p(src)) {
    struct strm_lambda* lmbd = strm_value_lambda(src);
    src = strm_stream_value(strm_stream_new(strm_filter, blk_exec, NULL, (void*)lmbd));
  }
  /* src: array */
  else if (strm_array_p(src)) {
    struct array_data *arrd = malloc(sizeof(struct array_data));
    arrd->arr = strm_value_ary(src);
    arrd->n = 0;
    src = strm_stream_value(strm_stream_new(strm_producer, arr_exec, NULL, (void*)arrd));
  }
  /* src: should be stream */

  /* dst: io */
  if (strm_io_p(dst)) {
    dst = strm_stream_value(strm_io_stream(dst, STRM_IO_WRITE));
  }
  /* dst: lambda */
  else if (strm_lambda_p(dst)) {
    struct strm_lambda* lmbd = strm_value_lambda(dst);
    dst = strm_stream_value(strm_stream_new(strm_filter, blk_exec, NULL, (void*)lmbd));
  }
  /* dst: cfunc */
  else if (strm_cfunc_p(dst)) {
    strm_cfunc func = strm_value_cfunc(dst);
    dst = strm_stream_value(strm_stream_new(strm_filter, cfunc_exec, cfunc_closer, func));
  }

  /* stream x stream */
  if (strm_stream_p(src) && strm_stream_p(dst)) {
    strm_stream* lstrm = strm_value_stream(src);
    strm_stream* rstrm = strm_value_stream(dst);
    if (lstrm == NULL || rstrm == NULL ||
        lstrm->mode == strm_consumer ||
        rstrm->mode == strm_producer) {
      strm_raise(strm, "stream error");
      return STRM_NG;
    }
    strm_stream_connect(strm_value_stream(src), strm_value_stream(dst));
    *ret = dst;
    return STRM_OK;
  }
  return STRM_NG;
}

static int
exec_bar(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "vv", &x, &y);
  return strm_connect(strm, x, y, ret);
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
  if (strm_number_p(idx)) {
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
pattern_match(strm_stream* strm, strm_state* state, node* npat, int argc, strm_value* argv);
static int
pmatch(strm_stream* strm, strm_state* state, node* pat, strm_value val);

static int
pattern_placeholder_p(node_string name){
  if (name->len == 1 && name->buf[0] == '_')
    return TRUE;
  return FALSE;
}

static int
pmatch_struct(strm_stream* strm, strm_state* state, node* pat, strm_value val, uint64_t* tbl, strm_int* len)
{
  node_nodes* pstr = (node_nodes*)pat;
  strm_array ary = strm_value_ary(val);
  struct strm_array* a = strm_ary_struct(ary);
  strm_value* headers;

  if (!a->headers) return STRM_NG;
  if (pstr->len > a->len) return STRM_NG;
  headers = strm_ary_ptr(a->headers);
  for (int i=0; i<pstr->len; i++) {
    node_pair* npair = (node_pair*)pstr->data[i];
    strm_string key;

    assert(npair->type == NODE_PAIR);
    key = node_to_sym(npair->key);
    for (int j=0; i<a->len; j++) {
      if (headers[j] == key) {
        if (pmatch(strm, state, npair->value, a->ptr[j]) == STRM_NG)
          return STRM_NG;
        if (tbl) {
          uint64_t n = 1<<(j%64);
          if (tbl[j/64] & n) (*len)--;
          tbl[j/64] |= n;
        }
        break;
      }
    }
  }
  return STRM_OK;
}

static int
pmatch(strm_stream* strm, strm_state* state, node* pat, strm_value val)
{
  switch (pat->type) {
  case NODE_IDENT:
    {
      node_ident* ni = (node_ident*)pat;
      if (pattern_placeholder_p(ni->name))
        return STRM_OK;
      return strm_var_match(state, node_to_sym(ni->name), val);
    }
  case NODE_STR:
    {
      if (strm_string_p(val)) {
        if (strm_str_eq(strm_value_str(val), node_to_str(((node_str*)pat)->value)))
          return STRM_OK;
      }
    }
    break;
  case NODE_INT:
    {
      strm_int n = ((node_int*)pat)->value;

      if (strm_int_p(val)) {
        if (n == strm_value_int(val))
          return STRM_OK;
        return STRM_NG;
      }
      if (strm_float_p(val)) {
        if (n == strm_value_float(val))
          return STRM_OK;
        return STRM_NG;
      }
    }
    break;
  case NODE_NIL:
    if (strm_nil_p(val))
      return STRM_OK;
    return STRM_NG;
  case NODE_BOOL:
    if (strm_value_bool(val) == ((node_bool*)pat)->value)
      return STRM_OK;
    return STRM_NG;
  case NODE_FLOAT:
    if (strm_number_p(val)) {
      if ((((node_float*)pat)->value) == strm_value_float(val))
        return STRM_OK;
      return STRM_NG;
    }
    break;
  case NODE_NS:
    {
      node_ns* ns = (node_ns*)pat;
      strm_state* s1 = strm_ns_get(node_to_sym(ns->name));
      strm_state* s2 = strm_value_ns(val);

      if (s1 != s2) return STRM_NG;
      return pmatch(strm, state, ns->body, val);
    }
    break;
  case NODE_PARRAY:
    if (strm_array_p(val)) {
      strm_array ary = strm_value_ary(val);
      return pattern_match(strm, state, pat, strm_ary_len(ary), strm_ary_ptr(ary));
    }
    break;
  case NODE_PSPLAT:
    if (strm_array_p(val)) {
      strm_array ary = strm_value_ary(val);
      node_psplat* psp = (node_psplat*)pat;

      if (psp->head && psp->head->type == NODE_PSTRUCT) {
        node_nodes* pstr = (node_nodes*)psp->head;
        strm_int len = pstr->len;
        uint64_t buf = 0;
        uint64_t *tbl = &buf;
        if (len > 64) {
          tbl = malloc(len/64+1);
          memset(tbl, 0, len/64+1);
        }
        if (pmatch_struct(strm, state, psp->head, val, tbl, &len) == STRM_NG) return STRM_NG;
        assert(psp->tail == NULL);
        {
          struct strm_array* a = strm_ary_struct(ary);
          strm_value* hdr = strm_ary_ptr(a->headers);
          strm_array splat = strm_ary_new(NULL, a->len-len);
          strm_array nhdr = strm_ary_new(NULL, a->len-len);
          int n = 0;

          for (int i=0; i<a->len; i++) {
            if (tbl[i/64] & (1<<(i%64))) continue;
            strm_ary_ptr(nhdr)[n] = hdr[i];
            strm_ary_ptr(splat)[n] = a->ptr[i];
            n++;
          }
          strm_ary_headers(splat) = nhdr;
          return pmatch(strm, state, psp->mid, strm_ary_value(splat));
        }
      }
      return pattern_match(strm, state, pat, strm_ary_len(ary), strm_ary_ptr(ary));
    }
    break;
  case NODE_PSTRUCT:
    if (!strm_array_p(val)) return STRM_NG;
    return pmatch_struct(strm, state, pat, val, NULL, NULL);
  default:
    break;
  }
  return STRM_NG;
}

static int
pattern_match(strm_stream* strm, strm_state* state, node* npat, int argc, strm_value* argv)
{
  node_nodes* pat = (node_nodes*)npat;
  int i;

  if (pat == NULL) return STRM_OK; /* case else */
  if (npat->type == NODE_PSPLAT) {
    node_psplat* psp = (node_psplat*)pat;
    node_nodes* head = (node_nodes*)psp->head;
    node_nodes* tail = (node_nodes*)psp->tail;
    node* rest = psp->mid;
    strm_int hlen = head ? head->len : 0;

    if (argc < hlen) return STRM_NG;
    if (head) {
      if (pattern_match(strm, state, (node*)head, hlen, argv) == STRM_NG)
        return STRM_NG;
    }
    if (tail == NULL) {
      if (pmatch(strm, state, rest, strm_ary_new(argv+hlen, argc-hlen)) == STRM_NG)
        return STRM_NG;
    }
    else {
      if (argc < hlen+tail->len) return STRM_NG;
      if (pattern_match(strm, state, rest, argc-hlen-tail->len, argv+hlen) == STRM_NG)
        return STRM_NG;
      if (pattern_match(strm, state, (node*)tail, tail->len, argv+argc-tail->len) == STRM_NG)
        return STRM_NG;
    }
    return STRM_OK;
  }
  if (pat->len != argc) return STRM_NG;
  for (i=0; i<pat->len; i++) {
    if (pmatch(strm, state, pat->data[i], argv[i]) == STRM_NG)
      return STRM_NG;
  }
  return STRM_OK;
}

static int
lambda_call(strm_stream* strm, strm_value func, int argc, strm_value* argv, strm_value* ret)
{
  struct strm_lambda* lambda = strm_value_lambda(func);
  strm_state c = {0};
  int i, n;
  node_error* exc;

  c.prev = lambda->state;
  if (lambda->body->type == NODE_LAMBDA) {
    node_lambda* nlmbd = (node_lambda*)lambda->body;
    node_args* args = (node_args*)nlmbd->args;

    if (args == NULL) {
      if (argc > 0) goto argerr;
    }
    else if (args->len != argc) {
    argerr:
      strm_raise(strm, "wrong number of arguments");
      goto err;
    }
    for (i=0; i<argc; i++) {
      n = strm_var_set(&c, node_to_sym(args->data[i]), argv[i]);
      if (n) return n;
    }
    n = exec_expr(strm, &c, nlmbd->body, ret);
  }
  else if (lambda->body->type == NODE_PLAMBDA) {
    node_plambda* plmbd = (node_plambda*)lambda->body;
    int nexec = 0;

    while (plmbd) {
      if (pattern_match(strm, &c, plmbd->pat, argc, argv) == STRM_OK) {
        strm_value cond;

        if (plmbd->cond) {
          n = exec_expr(strm, &c, plmbd->cond, &cond);
          if (n == STRM_OK && strm_value_bool(cond)) {
            nexec++;
            n = exec_expr(strm, &c, plmbd->body, ret);
            break;
          }
        }
        else {
          nexec++;
          n = exec_expr(strm, &c, plmbd->body, ret);
          break;
        }
      }
      c.env = NULL;
      plmbd = (node_plambda*)plmbd->next;
    }
    if (nexec == 0) {
      strm_raise(strm, "match failure");
      goto err;
    }
  }
  if (n == STRM_NG && strm) {
    exc = strm->exc;
    if (exc && exc->type == NODE_ERROR_RETURN) {
      *ret = exc->arg;
      return STRM_OK;
    }
  }
  return n;
 err:
  if (strm && strm->exc) {
    strm->exc->fname = lambda->body->fname;
    strm->exc->lineno = lambda->body->lineno;
  }
  return STRM_NG;
}

static struct strm_genfunc*
genfunc_new(strm_state* state, strm_string id)
{
  struct strm_genfunc *gf = malloc(sizeof(struct strm_genfunc));

  if (!gf) return NULL;
  gf->type = STRM_PTR_GENFUNC;
  gf->state = state;
  gf->id = id;
  return gf;
}

static int exec_call(strm_stream* strm, strm_state* state, strm_string name, int argc, strm_value* argv, strm_value* ret);

int
strm_funcall(strm_stream* strm, strm_value func, int argc, strm_value* argv, strm_value* ret)
{
  switch (strm_value_tag(func)) {
  case STRM_TAG_CFUNC:
    return (strm_value_cfunc(func))(strm, argc, argv, ret);
  case STRM_TAG_ARRAY:
    return ary_get(strm, func, argc, argv, ret);
  case STRM_TAG_PTR:
    if (strm_ptr_tag_p(func, STRM_PTR_GENFUNC)) {
      struct strm_genfunc *gf = strm_value_vptr(func);
      return exec_call(strm, gf->state, gf->id, argc, argv, ret);
    }
    else if (strm_lambda_p(func)) {
      return lambda_call(strm, func, argc, argv, ret);
    }
    break;
  default:
    break;
  }
  strm_raise(strm, "not a function");
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
      strm_string name = node_to_sym(ns->name);
      strm_state* s = strm_ns_create(state, name);

      if (!s) {
        if (strm_ns_get(name)) {
          strm_raise(strm, "namespace already exists");
        }
        else {
          strm_raise(strm, "failed to create namespace");
        }
        return STRM_NG;
      }
      STRM_NS_UDEF_SET(s);
      if (ns->body)
        return exec_expr(strm, s, ns->body, val);
      return STRM_OK;
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
    return STRM_NG;
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
      strm_value *ptr = strm_ary_ptr(arr);
      int splat = FALSE;

      for (int i = 0; i < v0->len; i++) {
        if (v0->data[i]->type == NODE_SPLAT) {
          node_splat* s = (node_splat*)v0->data[i];
          n = exec_expr(strm, state, s->node, &ptr[i]);
          if (n) return n;
          if (!strm_array_p(ptr[i])) {
            strm_raise(strm, "splat requires array");
            return STRM_NG;
          }
          splat = TRUE;
        }
        else {
          n = exec_expr(strm, state, v0->data[i], &ptr[i]);
          if (n) return n;
        }
      }
      if (splat) {
        int len = v0->len;

        if (v0->headers) {
          strm_raise(strm, "label(s) and splat(s) in an array");
          return STRM_NG;
        }
        for (int i = 0; i < v0->len; i++) {
          if (v0->data[i]->type == NODE_SPLAT) {
            strm_array a = strm_value_ary(ptr[i]);
            len += strm_ary_len(a)-1;
          }
        }
        if (len > v0->len) {
          strm_value* nptr;

          arr = strm_ary_new(NULL, len);
          nptr = strm_ary_ptr(arr);
          for (int i = 0; i < v0->len; i++) {
            if (v0->data[i]->type == NODE_SPLAT) {
              strm_array a = strm_value_ary(ptr[i]);
              int alen = strm_ary_len(a);
              strm_value* aptr = strm_ary_ptr(a);
              for (int j=0; j<alen; j++) {
                *nptr++ = aptr[j];
              }
            }
            else {
              *nptr++ = ptr[i];
            }
          }
        }
      }
      else if (v0->headers) {
        strm_ary_headers(arr) = ary_headers(v0->headers, v0->len);
      }
      if (v0->ns) {
        strm_state* ns = strm_ns_get(node_to_sym(v0->ns));
        if (!STRM_NS_UDEF_GET(ns)) {
          strm_raise(strm, "instantiating primitive class");
          return STRM_NG;
        }
        strm_ary_ns(arr) = ns;
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
  case NODE_PLAMBDA:
    {
      struct strm_lambda* lambda = malloc(sizeof(struct strm_lambda));

      if (!lambda) return STRM_NG;
      lambda->state = malloc(sizeof(strm_state));
      if (!lambda->state) return STRM_NG;
      *lambda->state = *state;
      lambda->type = STRM_PTR_LAMBDA;
      lambda->body = (node_lambda*)np;
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
      strm_value *args;
      int splat = FALSE;

      for (i = 0; i < v0->len; i++) {
        if (v0->data[i]->type == NODE_SPLAT) {
          splat = TRUE;
          break;
        }
      }
      if (splat) {
        strm_value aary;
        n = exec_expr(strm, state, ncall->args, &aary);
        args = strm_ary_ptr(aary);
        i = strm_ary_len(aary);
      }
      else {
        args = malloc(sizeof(strm_value)*v0->len);
        for (i = 0; i < v0->len; i++) {
          n = exec_expr(strm, state, v0->data[i], &args[i]);
          if (n == STRM_NG) {
            free(args);
            return n;
          }
        }
      }
      n = exec_call(strm, state, node_to_sym(ncall->ident), i, args, val);
      if (!splat) free(args);
      return n;
    }
    break;
  case NODE_FCALL:
    {
      node_fcall* ncall = (node_fcall*)np;
      int i;
      strm_value func;
      node_nodes* v0 = (node_nodes*)ncall->args;
      strm_value *args;
      int splat = FALSE;

      if (exec_expr(strm, state, ncall->func, &func) == STRM_NG) {
        return STRM_NG;
      }
      for (i = 0; i < v0->len; i++) {
        if (v0->data[i]->type == NODE_SPLAT) {
          splat = TRUE;
          break;
        }
      }
      if (splat) {
        strm_value aary;
        n = exec_expr(strm, state, ncall->args, &aary);
        args = strm_ary_ptr(aary);
        i = strm_ary_len(aary);
      }
      else {
        args = malloc(sizeof(strm_value)*v0->len);
        for (i = 0; i < v0->len; i++) {
          n = exec_expr(strm, state, v0->data[i], &args[i]);
          if (n == STRM_NG) {
            free(args);
            return n;
          }
        }
      }
      n = strm_funcall(strm, func, i, args, val);
      if (!splat) free(args);
      return n;
    }
    break;
  case NODE_GENFUNC:
    {
      node_genfunc* ngf = (node_genfunc*)np;
      struct strm_genfunc *gf;

      gf = genfunc_new(state, node_to_str(ngf->id));
      if (!gf) return STRM_NG;
      *val = strm_ptr_value(gf);
      return STRM_OK;
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
            arg = strm_ary_value(ary);
          }
          break;
        }
      }
      strm_set_exc(strm, NODE_ERROR_RETURN, arg);
      return STRM_NG;
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
    *val = strm_float_value(((node_float*)np)->value);
    return STRM_OK;
  case NODE_TIME:
    {
      node_time* nt = (node_time*)np;
      *val = strm_time_new(nt->sec, nt->usec, nt->utc_offset);
      if (strm_nil_p(*val)) return STRM_NG;
    }
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
    strm_raise(strm, "unknown node");
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

    if (i != 0) {
      if (!strm_string_p(args[i-1])) {
        fputs(" ", out);
      }
    }
    s = strm_to_str(args[i]);
    fwrite(strm_str_ptr(s), strm_str_len(s), 1, out);
  }
  fputs("\n", out);
  *ret = strm_nil_value();
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
  if (exc->type == NODE_ERROR_SKIP) return;
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

  strm_get_args(strm, argc, args, "S", &path);
  fd = open(strm_str_cstr(path, buf), O_RDONLY);
  if (fd < 0) {
    strm_raise(strm, "fread() failed");
    return STRM_NG;
  }
  *ret = strm_io_new(fd, STRM_IO_READ);
  return STRM_OK;
}

static int
exec_fwrite(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  int fd;
  strm_string path;
  char buf[7];

  strm_get_args(strm, argc, args, "S", &path);
  fd = open(strm_str_cstr(path, buf), O_WRONLY|O_CREAT, 0644);
  if (fd < 0) return STRM_NG;
  *ret = strm_io_new(fd, STRM_IO_WRITE);
  return STRM_OK;
}

static int
exec_exit(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_int estatus = EXIT_SUCCESS;

  strm_get_args(strm, argc, args, "|i", &estatus);
  exit(estatus);
  /* not reached */
  *ret = strm_int_value(estatus);
  return STRM_OK;
}

static int
exec_match(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value func;

  if (argc < 2) {
      strm_raise(strm, "wrong number of arguments");
      return STRM_NG;
  }
  func = args[argc-1];
  if (strm_lambda_p(func)) {
    struct strm_lambda* lambda = strm_value_lambda(func);
    if (lambda->body->type == NODE_LAMBDA) {
      strm_raise(strm, "not a case function");
      return STRM_NG;
    }
  }
  return strm_funcall(strm, func, argc-1, args, ret);
}

void
strm_raise(strm_stream* strm, const char* msg)
{
  if (!strm) return;
  strm_set_exc(strm, NODE_ERROR_RUNTIME,
               strm_str_value(strm_str_new(msg, strlen(msg))));
}

void strm_array_init(strm_state* state);
void strm_string_init(strm_state* state);
void strm_latch_init(strm_state* state);
void strm_iter_init(strm_state* state);
void strm_socket_init(strm_state* state);
void strm_csv_init(strm_state* state);
void strm_kvs_init(strm_state* state);

void strm_init(strm_state*);

static void
node_init(strm_state* state)
{
  strm_init(state);

  strm_var_def(state, "stdin", strm_io_new(0, STRM_IO_READ));
  strm_var_def(state, "stdout", strm_io_new(1, STRM_IO_WRITE));
  strm_var_def(state, "stderr", strm_io_new(2, STRM_IO_WRITE));
  strm_var_def(state, "puts", strm_cfunc_value(exec_puts));
  strm_var_def(state, "print", strm_cfunc_value(exec_puts));
  strm_var_def(state, "==", strm_cfunc_value(exec_eq));
  strm_var_def(state, "!=", strm_cfunc_value(exec_neq));
  strm_var_def(state, "|", strm_cfunc_value(exec_bar));
  strm_var_def(state, "fread", strm_cfunc_value(exec_fread));
  strm_var_def(state, "fwrite", strm_cfunc_value(exec_fwrite));
  strm_var_def(state, "exit", strm_cfunc_value(exec_exit));
  strm_var_def(state, "match", strm_cfunc_value(exec_match));
}

static strm_state top_state = {0};
static strm_stream top_strm = {0};

int
node_run(parser_state* p)
{
  strm_value v;
  node_error* exc;

  node_init(&top_state);

  exec_expr(&top_strm, &top_state, (node*)p->lval, &v);
  exc = top_strm.exc;
  if (exc != NULL) {
    if (exc->type != NODE_ERROR_RETURN) {
      strm_eprint(&top_strm);
    }
    strm_clear_exc(&top_strm);
  }
  return STRM_OK;
}

void
node_stop()
{
}

static int
blk_exec(strm_stream* strm, strm_value data)
{
  struct strm_lambda* lambda = strm->data;
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

  n = exec_expr(strm, &c, lambda->body->body, &ret);
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
