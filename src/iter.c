#include "strm.h"
#include "khash.h"

struct seq_data {
  double n;
  double end;
  double inc;
};

static int
gen_seq(strm_stream* strm, strm_value data)
{
  struct seq_data* d = strm->data;

  if (d->end > 0 && d->n > d->end) {
    strm_stream_close(strm);
    return STRM_OK;
  }
  strm_emit(strm, strm_float_value(d->n), gen_seq);
  d->n += d->inc;
  return STRM_OK;
}

static int
exec_seq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double start=1, end=-1, inc=1, tmp;
  struct seq_data* d;

  strm_get_args(strm, argc, args, "|fff", &start, &end, &tmp);
  switch (argc) {
  case 1:
    end = start;
    start = 1;
    break;
  case 3:
    inc = end;
    end = tmp;
    break;
  default:
    break;
  }
  d = malloc(sizeof(*d));
  d->n = start;
  d->inc = inc;
  d->end = end;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_seq, NULL, (void*)d));
  return STRM_OK;
}

struct repeat_data {
  strm_value v;
  strm_int count;
};

static int
gen_repeat(strm_stream* strm, strm_value data)
{
  struct repeat_data *d = strm->data;

  d->count--;
  if (d->count == 0) {
    strm_emit(strm, d->v, NULL);
    strm_stream_close(strm);
  }
  else {
    strm_emit(strm, d->v, gen_repeat);
  }
  return STRM_OK;
}

static int
fin_repeat(strm_stream* strm, strm_value data)
{
  free(strm->data);
  return STRM_OK;
}

static int
exec_repeat(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value v;
  strm_int n = -1;
  struct repeat_data *d;

  strm_get_args(strm, argc, args, "v|i", &v, &n);
  if (argc == 2 && n <= 0) {
    strm_raise(strm, "invalid count number");
    return STRM_NG;
  }
  d = malloc(sizeof(*d));
  d->v = v;
  d->count = n;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_repeat, fin_repeat, (void*)d));
  return STRM_OK;
}

struct cycle_data {
  strm_array ary;
  strm_int count;
};

static int
gen_cycle(strm_stream* strm, strm_value data)
{
  struct cycle_data *d = strm->data;
  strm_value* p;
  strm_int i, len;

  d->count--;
  p = strm_ary_ptr(d->ary);
  len = strm_ary_len(d->ary);
  if (d->count != 0) {
    len--;
  }
  for (i=0; i<len; i++) {
    strm_emit(strm, p[i], NULL);
  }
  if (d->count == 0) {
    strm_stream_close(strm);
  }
  else {
    strm_emit(strm, p[i], gen_cycle);
  }
  return STRM_OK;
}

static int
fin_cycle(strm_stream* strm, strm_value data)
{
  free(strm->data);
  return STRM_OK;
}

static int
exec_cycle(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_array a;
  strm_int n = -1;
  struct cycle_data *d;

  strm_get_args(strm, argc, args, "A|i", &a, &n);
  if (argc == 2 && n <= 0) {
    strm_raise(strm, "invalid count number");
    return STRM_NG;
  }
  d = malloc(sizeof(*d));
  d->ary = a;
  d->count = n;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_cycle, fin_cycle, (void*)d));
  return STRM_OK;
}

struct map_data {
  strm_value func;
};

static int
iter_each(strm_stream* strm, strm_value data)
{
  struct map_data* d = strm->data;
  strm_value val;

  if (strm_funcall(strm, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  return STRM_OK;
}

static int
exec_each(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d;
  strm_value func;

  strm_get_args(strm, argc, args, "v", &func);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_each, NULL, (void*)d));
  return STRM_OK;
}

static int
ary_each(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* v;
  strm_int len;
  strm_value func;
  strm_int i;
  strm_value r;

  strm_get_args(strm, argc, args, "av", &v, &len, &func);

  for (i=0; i<len; i++) {
    if (strm_funcall(strm, func, 1, &v[i], &r) == STRM_NG) {
      return STRM_NG;
    }
  }
  *ret = strm_ary_value(args[0]);
  return STRM_OK;
}

static int
iter_map(strm_stream* strm, strm_value data)
{
  struct map_data* d = strm->data;
  strm_value val;

  if (strm_funcall(strm, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  strm_emit(strm, val, NULL);
  return STRM_OK;
}

static int
exec_map(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d;
  strm_value func;

  strm_get_args(strm, argc, args, "v", &func);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_map, NULL, (void*)d));
  return STRM_OK;
}

static int
ary_map(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* v;
  strm_int len;
  strm_value func;
  strm_int i;
  strm_array a2;
  strm_value* v2;

  strm_get_args(strm, argc, args, "av", &v, &len, &func);
  a2 = strm_ary_new(NULL, len);
  v2 = strm_ary_ptr(a2);

  for (i=0; i<len; i++) {
    if (strm_funcall(strm, func, 1, &v[i], &v2[i]) == STRM_NG) {
      return STRM_NG;
    }
  }
  *ret = strm_ary_value(a2);
  return STRM_OK;
}

static int
iter_flatmap(strm_stream* strm, strm_value data)
{
  struct map_data* d = strm->data;
  strm_value val;
  strm_int i, len;
  strm_value* e;

  if (strm_funcall(strm, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  if (!strm_array_p(val)) {
    strm_raise(strm, "no array given for flatmap");
    return STRM_NG;
  }
  len = strm_ary_len(val);
  e = strm_ary_ptr(val);
  for (i=0; i<len; i++){
    strm_emit(strm, e[i], NULL);
  }
  return STRM_OK;
}

static int
flatmap_len(strm_array ary)
{
  strm_value* v = strm_ary_ptr(ary);
  strm_int i, len, n = 0;

  len = strm_ary_len(ary);
  for (i=0; i<len; i++) {
    if (strm_array_p(v[i])) {
      n += flatmap_len(v[i]);
    }
    else {
      n++;
    }
  }
  return n;
}

static int
flatmap_push(strm_stream* strm, strm_array ary, strm_value func, strm_value** p)
{
  strm_value* v = strm_ary_ptr(ary);
  strm_int i, len;

  len = strm_ary_len(ary);
  for (i=0; i<len; i++) {
    if (strm_array_p(v[i])) {
      if (flatmap_push(strm, v[i], func, p) == STRM_NG) {
        return STRM_NG;
      }
    }
    else {
      if (strm_funcall(strm, func, 1, &v[i], *p) == STRM_NG) {
        return STRM_NG;
      }
      *p += 1;
    }
  }
  return STRM_OK;
}

static int
exec_flatmap(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d;
  strm_value func;

  strm_get_args(strm, argc, args, "v", &func);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_flatmap, NULL, (void*)d));
  return STRM_OK;
}

static int
ary_flatmap(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_array ary;
  strm_value func;
  strm_array a2;
  strm_value* v2;

  strm_get_args(strm, argc, args, "Av", &ary, &func);
  a2 = strm_ary_new(NULL, flatmap_len(ary));
  v2 = strm_ary_ptr(a2);
  if (flatmap_push(strm, ary, func, &v2) == STRM_NG) {
    return STRM_NG;
  }
  *ret = strm_ary_value(a2);
  return STRM_OK;
}

static int
iter_filter(strm_stream* strm, strm_value data)
{
  struct map_data* d = strm->data;
  strm_value val;

  if (strm_funcall(strm, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  if (strm_value_bool(val)) {
    strm_emit(strm, data, NULL);
  }
  return STRM_OK;
}

static int
exec_filter(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d = malloc(sizeof(*d));

  strm_get_args(strm, argc, args, "v", &d->func);
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_filter, NULL, (void*)d));
  return STRM_OK;
}

struct count_data {
  strm_int count;
};

static int
iter_count(strm_stream* strm, strm_value data)
{
  struct count_data* d = strm->data;

  d->count++;
  return STRM_OK;
}

static int
count_finish(strm_stream* strm, strm_value data)
{
  struct count_data* d = strm->data;

  strm_emit(strm, strm_int_value(d->count), NULL);
  free(d);
  return STRM_OK;
}

static int
exec_count(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct count_data* d;

  strm_get_args(strm, argc, args, "");
  d = malloc(sizeof(*d));
  d->count = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_count, count_finish, (void*)d));
  return STRM_OK;
}

struct minmax_data {
  int start;
  int min;
  strm_value data;
  double num;
  strm_value func;
};

static int
iter_minmax(strm_stream* strm, strm_value data)
{
  struct minmax_data* d = strm->data;
  strm_value e;
  double num;

  if (!strm_nil_p(d->func)) {
    if (strm_funcall(strm, d->func, 1, &data, &e) == STRM_NG) {
      return STRM_NG;
    }
  }
  else {
    e = data;
  }
  num = strm_value_float(e);
  if (d->start) {
    d->start = FALSE;
    d->num = num;
    d->data = data;
  }
  else if (d->min) {            /* min */
    if (d->num > num) {
      d->num = num;
      d->data = data;
    }
  }
  else {                        /* max */
    if (d->num < num) {
      d->num = num;
      d->data = data;
    }
  }
  return STRM_OK;
}

static int
minmax_finish(strm_stream* strm, strm_value data)
{
  struct minmax_data* d = strm->data;

  strm_emit(strm, d->data, NULL);
  return STRM_OK;
}

static int
exec_minmax(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int min)
{
  struct minmax_data* d;
  strm_value func = strm_nil_value();

  strm_get_args(strm, argc, args, "|v", &func);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->start = TRUE;
  d->min = min;
  d->num = 0;
  d->data = strm_nil_value();
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_minmax, minmax_finish, (void*)d));
  return STRM_OK;
}

static int
exec_min(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_minmax(strm, argc, args, ret, TRUE);
}

static int
exec_max(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_minmax(strm, argc, args, ret, FALSE);
}

struct reduce_data {
  strm_int init;
  strm_value acc;
  strm_value func;
};

static int
iter_reduce(strm_stream* strm, strm_value data)
{
  struct reduce_data* d = strm->data;
  strm_value args[2];

  /* first acc */
  if (!d->init) {
    d->init = 1;
    d->acc = data;
    return STRM_OK;
  }

  args[0] = d->acc;
  args[1] = data;
  if (strm_funcall(strm, d->func, 2, args, &data) == STRM_NG) {
    return STRM_NG;
  }
  d->acc = data;
  return STRM_OK;
}

static int
reduce_finish(strm_stream* strm, strm_value data)
{
  struct reduce_data* d = strm->data;

  if (!d->init) return STRM_NG;
  strm_emit(strm, d->acc, NULL);
  return STRM_OK;
}

static int
exec_reduce(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct reduce_data* d;
  strm_value v1, v2;

  strm_get_args(strm, argc, args, "v|v", &v1, &v2);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  if (argc == 2) {
    d->init = TRUE;
    d->acc = v1;
    d->func = v2;
  }
  else {
    d->init = FALSE;
    d->acc = strm_nil_value();
    d->func = v1;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_reduce, reduce_finish, (void*)d));
  return STRM_OK;
}


KHASH_MAP_INIT_INT64(rbk, strm_value);

struct rbk_data {
  khash_t(rbk) *tbl;
  strm_value func;
};

static int
iter_rbk(strm_stream* strm, strm_value data)
{
  struct rbk_data *d = strm->data;
  strm_value k, v;
  khiter_t i;
  int r;

  if (!strm_array_p(data) || strm_ary_len(data) != 2) {
    strm_raise(strm, "reduce_by_key element must be a key-value pair");
    return STRM_NG;
  }
  k = strm_ary_ptr(data)[0];
  v = strm_ary_ptr(data)[1];

  i = kh_put(rbk, d->tbl, k, &r);
  if (r < 0) {                  /* r<0 operation failed */
    return STRM_NG;
  }
  if (r != 0) {                 /* key does not exist */
    kh_value(d->tbl, i) = v;
  }
  else {
    strm_value args[3];

    args[0] = k;
    args[1] = kh_value(d->tbl, i);
    args[2] = v;
    if (strm_funcall(strm, d->func, 3, args, &v) == STRM_NG) {
      return STRM_NG;
    }
    kh_value(d->tbl, i) = v;
  }
  return STRM_OK;
}

static int
rbk_finish(strm_stream* strm, strm_value data)
{
  struct rbk_data *d = strm->data;
  khiter_t i;

  for (i=kh_begin(d->tbl); i!=kh_end(d->tbl); i++) {
    if (kh_exist(d->tbl, i)) {
      strm_value values[2];

      values[0] = kh_key(d->tbl, i);
      values[1] = kh_value(d->tbl, i);
      strm_emit(strm, strm_ary_new(values, 2), NULL);
    }
  }
  return STRM_OK;
}

static int
exec_rbk(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct rbk_data *d;
  khash_t(rbk) *t;
  strm_value func;

  strm_get_args(strm, argc, args, "v", &func);
  t = kh_init(rbk);
  if (!t) return STRM_NG;
  d = malloc(sizeof(*d));
  d->tbl = t;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_rbk, rbk_finish, (void*)d));
  return STRM_OK;
}

struct slice_data {
  strm_int n;
  strm_int i;
  strm_value *buf;
};

static int
iter_slice(strm_stream* strm, strm_value data)
{
  struct slice_data* d = strm->data;
  strm_int n = d->n;

  d->buf[d->i++] = data;
  if (d->i == n) {
    strm_array ary = strm_ary_new(d->buf, n);

    d->i = 0;
    strm_emit(strm, strm_ary_value(ary), NULL);
  }
  return STRM_OK;
}

static int
finish_slice(strm_stream* strm, strm_value data)
{
  struct slice_data* d = strm->data;

  if (d->i > 0) {
    strm_array ary = strm_ary_new(d->buf, d->i);
    strm_emit(strm, strm_ary_value(ary), NULL);
  }
  free(d->buf);
  free(d);
  return STRM_OK;
}

static int
exec_slice(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct slice_data* d;
  strm_int n;

  strm_get_args(strm, argc, args, "i", &n);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->n = n;
  d->i = 0;
  d->buf = malloc(n*sizeof(strm_value));
  if (!d->buf) {
    free(d);
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_slice, finish_slice, (void*)d));
  return STRM_OK;
}

static int
iter_consec(strm_stream* strm, strm_value data)
{
  struct slice_data* d = strm->data;
  strm_int n = d->n;

  if (d->i < n) {
    d->buf[d->i++] = data;
    if (d->i == n) {
      strm_array ary = strm_ary_new(d->buf, n);
      strm_emit(strm, strm_ary_value(ary), NULL);
    }
    return STRM_OK;
  }
  else {
    strm_array ary;
    strm_int i;
    strm_int len = n-1;

    for (i=0; i<len; i++) {
      d->buf[i] = d->buf[i+1];
    }
    d->buf[len] = data;
    ary = strm_ary_new(d->buf, n);
    strm_emit(strm, strm_ary_value(ary), NULL);
  }
  return STRM_OK;
}

static int
finish_consec(strm_stream* strm, strm_value data)
{
  struct slice_data* d = strm->data;

  free(d->buf);
  free(d);
  return STRM_OK;
}

static int
exec_consec(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct slice_data* d;
  strm_int n;

  strm_get_args(strm, argc, args, "i", &n);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->n = n;
  d->i = 0;
  d->buf = malloc(n*sizeof(strm_value));
  if (!d->buf) {
    free(d);
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_consec, finish_consec, (void*)d));
  return STRM_OK;
}

struct take_data {
  int n;
};

static int
iter_take(strm_stream* strm, strm_value data)
{
  struct take_data* d = strm->data;

  strm_emit(strm, data, NULL);
  d->n--;
  if (d->n == 0) {
    strm_stream_close(strm);
  }
  return STRM_OK;
}

static int
exec_take(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct take_data* d;
  strm_int n;

  strm_get_args(strm, argc, args, "i", &n);
  if (n < 0) {
    strm_raise(strm, "negative iteration");
    return STRM_NG;
  }
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->n = n;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_take, NULL, (void*)d));
  return STRM_OK;
}

static int
iter_drop(strm_stream* strm, strm_value data)
{
  struct take_data* d = strm->data;

  if (d->n > 0) {
    d->n--;
    return STRM_OK;
  }
  strm_emit(strm, data, NULL);
  return STRM_OK;
}

static int
exec_drop(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct take_data* d;
  strm_int n;

  strm_get_args(strm, argc, args, "i", &n);
  if (n < 0) {
    strm_raise(strm, "negative iteration");
    return STRM_NG;
  }
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->n = n;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_drop, NULL, (void*)d));
  return STRM_OK;
}

struct uniq_data {
  strm_value last;
  strm_value v;
  strm_value func;
  int init;
};

static int
iter_uniq(strm_stream* strm, strm_value data)
{
  struct uniq_data* d = strm->data;

  if (!d->init) {
    d->init = TRUE;
    d->last = data;
    strm_emit(strm, data, NULL);
    return STRM_OK;
  }
  if (!strm_value_eq(data, d->last)) {
    d->last = data;
    strm_emit(strm, data, NULL);
  }
  return STRM_OK;
}

static int
iter_uniqf(strm_stream* strm, strm_value data)
{
  struct uniq_data* d = strm->data;
  strm_value val;

  if (strm_funcall(strm, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  if (!d->init) {
    d->init = TRUE;
    d->last = data;
    d->v = val;
    strm_emit(strm, data, NULL);
    return STRM_OK;
  }
  if (!strm_value_eq(val, d->v)) {
    d->last = data;
    d->v = val;
    strm_emit(strm, data, NULL);
  }
  return STRM_OK;
}

static int
exec_uniq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct uniq_data* d;
  strm_value func = strm_nil_value();

  strm_get_args(strm, argc, args, "|v", &func);
  d = malloc(sizeof(*d));
  if (!d) return STRM_NG;
  d->last = strm_nil_value();
  d->func = func;
  d->init = FALSE;
  *ret = strm_stream_value(strm_stream_new(strm_filter,
                                           strm_nil_p(func) ? iter_uniq : iter_uniqf,
                                           NULL, (void*)d));
  return STRM_OK;
}

void strm_stat_init(strm_state* state);

void
strm_iter_init(strm_state* state)
{
  strm_var_def(state, "seq", strm_cfunc_value(exec_seq));
  strm_var_def(state, "repeat", strm_cfunc_value(exec_repeat));
  strm_var_def(state, "cycle", strm_cfunc_value(exec_cycle));
  strm_var_def(state, "each", strm_cfunc_value(exec_each));
  strm_var_def(state, "map", strm_cfunc_value(exec_map));
  strm_var_def(state, "flatmap", strm_cfunc_value(exec_flatmap));
  strm_var_def(state, "filter", strm_cfunc_value(exec_filter));
  strm_var_def(state, "count", strm_cfunc_value(exec_count));
  strm_var_def(state, "min", strm_cfunc_value(exec_min));
  strm_var_def(state, "max", strm_cfunc_value(exec_max));
  strm_var_def(state, "reduce", strm_cfunc_value(exec_reduce));
  strm_var_def(state, "reduce_by_key", strm_cfunc_value(exec_rbk));

  strm_var_def(state, "slice", strm_cfunc_value(exec_slice));
  strm_var_def(state, "consec", strm_cfunc_value(exec_consec));
  strm_var_def(state, "take", strm_cfunc_value(exec_take));
  strm_var_def(state, "drop", strm_cfunc_value(exec_drop));
  strm_var_def(state, "uniq", strm_cfunc_value(exec_uniq));

  strm_var_def(strm_ns_array, "each", strm_cfunc_value(ary_each));
  strm_var_def(strm_ns_array, "map", strm_cfunc_value(ary_map));
  strm_var_def(strm_ns_array, "flatmap", strm_cfunc_value(ary_flatmap));
  strm_stat_init(state);
}
