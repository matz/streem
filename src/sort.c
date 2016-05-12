#define _GNU_SOURCE
#include "strm.h"
#include <stdlib.h>

static int
num_cmp(strm_value x, strm_value y)
{
  double a = strm_value_flt(x);
  double b = strm_value_flt(y);
  if(a > b)
    return 1;
  else if(a < b)
    return -1;
  return 0;
}

static int
str_cmp(strm_value x, strm_value y)
{
  strm_string a = strm_value_str(x);
  strm_string b = strm_value_str(y);
  strm_int alen = strm_str_len(a);
  strm_int blen = strm_str_len(b);
  strm_int len, cmp;

  if (alen > blen) {
    len = blen;
  }
  else {
    len = alen;
  }
  cmp = memcmp(strm_str_ptr(a), strm_str_ptr(b), len);
  if (cmp == 0) {
    if (alen > len) return 1;
    if (blen > len) return -1;
  }
  return cmp;
}

/* compare function */
/* num < str < other */
static int
strm_cmp(strm_value a, strm_value b)
{
  if (strm_num_p(a)) {
    if (strm_num_p(b)) {
      return num_cmp(a,b);
    }
    return -1;
  }
  if (strm_string_p(a)) {
    if (strm_string_p(b)) {
      return str_cmp(a,b);
    }
    if (strm_num_p(b)) {
      return 1;
    }
    return 1;
  }
  return 1;
}

static int
sort_cmp(const void* a_p, const void* b_p)
{
  strm_value a = *(strm_value*)a_p;
  strm_value b = *(strm_value*)b_p;

  return strm_cmp(a, b);
}

#if defined(__APPLE__) || defined(__FreeBSD__)
#define qsort_arg(p,nmem,size,cmp,arg) qsort_r(p,nmem,size,arg,cmp)
#define cmp_args(a,b,c) (c,a,b)
#else
#define qsort_arg(p,nmem,size,cmp,arg) qsort_r(p,nmem,size,cmp,arg)
#define cmp_args(a,b,c) (a,b,c)
#endif

struct sort_arg {
  strm_stream* strm;
  strm_value func;
};

static int
sort_cmpf cmp_args(const void* a_p, const void* b_p, void* arg)
{
  strm_value args[2];
  struct sort_arg* a = arg;
  strm_value val;
  strm_int cmp;

  args[0] = *(strm_value*)a_p;
  args[1] = *(strm_value*)b_p;

  if (strm_funcall(a->strm, a->func, 2, args, &val) == STRM_NG) {
    return 0;
  }
  if (!strm_num_p(val)) {
    return 0;
  }
  cmp = strm_value_int(val);
  if(cmp > 0)
    return 1;
  else if(cmp < 0)
    return -1;
  return 0;
}

static void
mem_sort(strm_value* p, strm_int len, struct sort_arg *arg)
{
  if (arg) {                    /* sort(ary, func) */
    qsort_arg(p, len, sizeof(strm_value), sort_cmpf, arg);
  }
  else {                        /* sort(ary) */
    qsort(p, len, sizeof(strm_value), sort_cmp);
  }
}

struct sort_data {
  strm_int len;
  strm_int capa;
  strm_value* buf;
  strm_value func;
};

#define SORT_FIRST_CAPA 1024

static int
iter_sort(strm_stream* strm, strm_value data)
{
  struct sort_data* d = strm->data;

  if (d->len >= d->capa) {
    d->capa *= 2;
    d->buf = realloc(d->buf, sizeof(strm_value)*d->capa);
  }
  d->buf[d->len++] = data;
  return STRM_OK;
}

static int
finish_sort(strm_stream* strm, strm_value data)
{
  struct sort_data* d = strm->data;
  strm_int i, len;

  if (strm_nil_p(d->func)) {
    mem_sort(d->buf, d->len, NULL);
  }
  else {
    struct sort_arg arg;

    arg.strm = strm;
    arg.func = d->func;
    mem_sort(d->buf, d->len, &arg);
  }

  for (i=0,len=d->len; i<len; i++) {
    strm_emit(strm, d->buf[i], NULL);
  }
  free(d->buf);
  free(d);
  return STRM_OK;
}

static int
exec_sort(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sort_data* d;
  strm_value func;

  switch (argc) {
  case 0:
    func = strm_nil_value();
    break;
  case 1:
    func = args[0];
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  d = malloc(sizeof(struct sort_data));
  if (!d) return STRM_NG;
  d->func = func;
  d->len = 0;
  d->capa = SORT_FIRST_CAPA;
  d->buf = malloc(sizeof(strm_value)*SORT_FIRST_CAPA);
  if (!d->buf) {
    free(d);
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sort,
                                           finish_sort, (void*)d));
  return STRM_OK;
}

static int
ary_sort(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_array ary;
  strm_value* p;
  strm_int len;

  switch (argc) {
  case 1:
  case 2:
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  ary = strm_ary_new(strm_ary_ptr(args[0]), strm_ary_len(args[0]));
  p = strm_ary_ptr(ary);
  len = strm_ary_len(ary);
  if (argc == 1) {
    mem_sort(p, len, NULL);
  }
  else {
    struct sort_arg arg;

    arg.strm = strm;
    arg.func = args[1];
    mem_sort(p, len, &arg);
  }
  *ret = strm_ary_value(ary);
  return STRM_OK;
}

struct sortby_value {
  strm_value v;
  strm_value o;
};

struct sortby_data {
  strm_int len;
  strm_int capa;
  struct sortby_value* buf;
  strm_stream* strm;
  strm_value func;
};

static int
sortby_cmp(const void* a_p, const void* b_p)
{
  struct sortby_value* av = (struct sortby_value*)a_p;
  struct sortby_value* bv = (struct sortby_value*)b_p;
  double a, b;

  if (strm_num_p(av->v)) {
    a = strm_value_flt(av->v);
  }
  else {
    if (strm_num_p(bv->v)) {
      return 1;
    }
    return 0;
  }
  if (strm_num_p(bv->v)) {
    b = strm_value_flt(bv->v);
  }
  else {
    return -1;
  }
  if(a > b)
    return 1;
  else if(a < b)
    return -1;
  return 0;
}

static int
iter_sortby(strm_stream* strm, strm_value data)
{
  struct sortby_data* d = strm->data;

  if (d->len >= d->capa) {
    d->capa *= 2;
    d->buf = realloc(d->buf, sizeof(struct sortby_value)*d->capa);
  }
  d->buf[d->len].o = data;
  if (strm_funcall(d->strm, d->func, 1, &data, &d->buf[d->len].v) == STRM_NG) {
    return STRM_NG;;
  }
  d->len++;
  return STRM_OK;
}

static int
finish_sortby(strm_stream* strm, strm_value data)
{
  struct sortby_data* d = strm->data;
  strm_int i, len;

  qsort(d->buf, d->len, sizeof(struct sortby_value), sortby_cmp);
  for (i=0,len=d->len; i<len; i++) {
    strm_emit(strm, d->buf[i].o, NULL);
  }
  free(d->buf);
  free(d);
  return STRM_OK;
}

static int
exec_sortby(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sortby_data* d;

  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  d = malloc(sizeof(struct sortby_data));
  if (!d) return STRM_NG;
  d->strm = strm;
  d->func = args[0];
  d->len = 0;
  d->capa = SORT_FIRST_CAPA;
  d->buf = malloc(sizeof(struct sortby_value)*SORT_FIRST_CAPA);
  if (!d->buf) {
    free(d);
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sortby,
                                           finish_sortby, (void*)d));
  return STRM_OK;
}

static int
ary_sortby(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sortby_value* buf;
  strm_array ary;
  strm_value* p;
  strm_int len;
  strm_int i;

  if (argc != 2) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  p = strm_ary_ptr(args[0]);
  len = strm_ary_len(args[0]);
  buf = malloc(sizeof(struct sortby_value)*len);
  if (!buf) return STRM_NG;
  for (i=0; i<len; i++) {
    buf[i].o = p[i];
    if (strm_funcall(strm, args[1], 1, &p[i], &buf[i].v) == STRM_NG) {
      free(buf);
      return STRM_NG;;
    }
  }
  qsort(buf, len, sizeof(struct sortby_value), sortby_cmp);
  ary = strm_ary_new(NULL, len);
  p = strm_ary_ptr(ary);
  for (i=0; i<len; i++) {
    p[i] = buf[i].o;
  }
  free(buf);
  *ret = strm_ary_value(ary);
  return STRM_OK;
}

static int
mem_median(strm_stream* strm, strm_value* buf, strm_int len, strm_value* ret)
{
  if (len % 2 == 1) {
    *ret = buf[len/2];
    goto good;
  }
  else {
    strm_value a = buf[len/2-1];
    strm_value b = buf[len/2];

    if (strm_num_p(a) && strm_num_p(b)) {
      double x = strm_value_flt(a);
      double y = strm_value_flt(b);

      x = (x + y)/2;
      *ret = strm_flt_value(x);
      goto good;
    }
    if (a == b) {
      *ret = a;
      goto good;
    }
    if (strm_string_p(a) && strm_string_p(b) && str_cmp(a,b) == 0) {
      *ret = a;
      goto good;
    }
    strm_raise(strm, "no median value");
    goto error;
  }

 good:
  free(buf);
  return STRM_OK;

 error:
  free(buf);
  return STRM_NG;
}

static int
iter_median(strm_stream* strm, strm_value data)
{
  struct sort_data* d = strm->data;

  if (d->len >= d->capa) {
    d->capa *= 2;
    d->buf = realloc(d->buf, sizeof(strm_value)*d->capa);
  }
  if (strm_nil_p(d->func)) {
    d->buf[d->len++] = data;
  }
  else if (strm_funcall(strm, d->func, 1, &data, &d->buf[d->len++]) == STRM_NG) {
    return STRM_NG;
  }
  return STRM_OK;
}

static int
finish_median(strm_stream* strm, strm_value data)
{
  struct sort_data* d = strm->data;
  strm_int n;
  strm_value v;

  qsort(d->buf, d->len, sizeof(strm_value), sort_cmp);
  n = mem_median(strm, d->buf, d->len, &v);
  strm_emit(strm, v, NULL);
  free(d);
  return n;
}

static int
exec_median(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sort_data* d;
  strm_value func;

  switch (argc) {
  case 0:
    func = strm_nil_value();
    break;
  case 1:
    func = args[0];
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  d = malloc(sizeof(struct sort_data));
  if (!d) return STRM_NG;
  d->func = func;
  d->len = 0;
  d->capa = SORT_FIRST_CAPA;
  d->buf = malloc(sizeof(strm_value)*SORT_FIRST_CAPA);
  if (!d->buf) {
    free(d);
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_median,
                                           finish_median, (void*)d));
  return STRM_OK;
}

static int
ary_median(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* buf;
  strm_value* p;
  strm_int len;
  strm_int i;

  switch (argc) {
  case 1:
  case 2:
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }

  p = strm_ary_ptr(args[0]);
  len = strm_ary_len(args[0]);
  if (len == 0) {
    strm_raise(strm, "empty array");
    return STRM_NG;
  }
  buf = malloc(sizeof(strm_value)*len);
  if (!buf) return STRM_NG;
  if (argc == 1) {              /* median(ary) */
    memcpy(buf, p, sizeof(strm_value)*len);
  }
  else {                        /* median(ary,func) */
    strm_value func = args[1];

    for (i=0; i<len; i++) {
      if (strm_funcall(strm, func, 1, &p[i], &buf[i]) == STRM_NG) {
        free(buf);
        return STRM_NG;
      }
    }
  }
  qsort(buf, len, sizeof(strm_value), sort_cmp);
  return mem_median(strm, buf, len, ret);
}

static int
exec_cmp(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_int cmp;

  if (argc != 2) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  cmp = strm_cmp(args[0], args[1]);
  *ret = strm_int_value(cmp);
  return STRM_OK;
}

void
strm_sort_init(strm_state* state)
{
  strm_var_def(strm_array_ns, "sort", strm_cfunc_value(ary_sort));
  strm_var_def(strm_array_ns, "sort_by", strm_cfunc_value(ary_sortby));
  strm_var_def(strm_array_ns, "median", strm_cfunc_value(ary_median));
  strm_var_def(state, "cmp", strm_cfunc_value(exec_cmp));
  strm_var_def(state, "sort", strm_cfunc_value(exec_sort));
  strm_var_def(state, "sort_by", strm_cfunc_value(exec_sortby));
  strm_var_def(state, "median", strm_cfunc_value(exec_median));
}
