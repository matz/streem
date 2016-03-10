#include "strm.h"
#include "khash.h"

struct seq_data {
  strm_int n;
  strm_int end;
  strm_int inc;
};

static int
gen_seq(strm_stream* strm, strm_value data)
{
  struct seq_data* d = strm->data;

  if (d->end > 0 && d->n > d->end) {
    strm_stream_close(strm);
    return STRM_OK;
  }
  strm_emit(strm, strm_int_value(d->n), gen_seq);
  d->n += d->inc;
  return STRM_OK;
}

static int
exec_seq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_int start=1, end=-1, inc=1;
  struct seq_data* d;

  switch (argc) {
  case 0:
    break;
  case 1:
    end = strm_value_int(args[0]);
    break;
  case 2:
    start = strm_value_int(args[0]);
    end = strm_value_int(args[1]);
    break;
  case 3:
    start = strm_value_int(args[0]);
    inc = strm_value_int(args[1]);
    end = strm_value_int(args[2]);
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  d = malloc(sizeof(struct seq_data));
  d->n = start;
  d->inc = inc;
  d->end = end;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_seq, NULL, (void*)d));
  return STRM_OK;
}

#include <sys/time.h>

static uint64_t x; /* the state must be seeded with a nonzero value. */

static uint64_t
xorshift64star(void)
{
  x ^= x >> 12; /* a */
  x ^= x << 25; /* b */
  x ^= x >> 27; /* c */
  return x * UINT64_C(2685821657736338717);
}

static int
gen_rand(strm_stream* strm, strm_value data)
{
  strm_int n = (strm_int)(intptr_t)strm->data;
  uint64_t r = xorshift64star();
  
  strm_emit(strm, strm_int_value(r % n), gen_rand);
  return STRM_OK;
}

static int
fin_rand(strm_stream* strm, strm_value data)
{
  free(strm->data);
  return STRM_OK;
}

static int
exec_rand(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct timeval tv;
  strm_int n;

  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  n = strm_value_int(args[0]);
  gettimeofday(&tv, NULL);
  x ^= (uint32_t)tv.tv_usec;
  x ^= x >> 11; x ^= x << 17; x ^= x >> 4;
  x *= 2685821657736338717LL;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_rand, fin_rand,
                                       (void*)(intptr_t)n));
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

  if (strm_funcall(NULL, d->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  return STRM_OK;
}

static int
exec_each(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d = malloc(sizeof(struct map_data));

  d->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_consumer, iter_each, NULL, (void*)d));
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
  struct map_data* d = malloc(sizeof(struct map_data));

  d->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_map, NULL, (void*)d));
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
exec_flatmap(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* d = malloc(sizeof(struct map_data));

  d->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_flatmap, NULL, (void*)d));
  return STRM_OK;
}

static int
iter_filter(strm_stream* strm, strm_value data)
{
  struct map_data* d = strm->data;
  strm_value val;

  if (strm_funcall(NULL, d->func, 1, &data, &val) == STRM_NG) {
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
  struct map_data* d = malloc(sizeof(struct map_data));

  d->func = args[0];
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
  struct count_data* d = malloc(sizeof(struct count_data));
  d->count = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_count, count_finish, (void*)d));
  return STRM_OK;
}

struct sum_data {
  double sum;
  strm_value func;
};

static int
iter_sum(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  if (!strm_nil_p(d->func)) {
    if (strm_funcall(NULL, d->func, 1, &data, &data) == STRM_NG) {
      return STRM_NG;
    }
  }
  d->sum += strm_value_flt(data);
  return STRM_OK;
}

static int
sum_finish(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  strm_emit(strm, strm_flt_value(d->sum), NULL);
  return STRM_OK;
}

static int
exec_sum(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sum_data* d;
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
  d = malloc(sizeof(struct sum_data));
  if (!d) return STRM_NG;
  d->sum = 0;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sum, sum_finish, (void*)d));
  return STRM_OK;
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
  if (strm_funcall(NULL, d->func, 2, args, &data) == STRM_NG) {
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
  strm_emit(strm, strm_int_value(d->acc), NULL);
  return STRM_OK;
}

static int
exec_reduce(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct reduce_data* d;
  strm_int init = 0;
  strm_value acc;
  strm_value func;

  switch (argc) {
  case 1:
    func = args[0];
    break;
  case 2:
    init = 1;
    acc = args[0];
    func = args[1];
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  d = malloc(sizeof(struct reduce_data));
  if (!d) return STRM_NG;
  d->init = init;
  d->acc = (init) ? acc : strm_nil_value();
  d->func = func;
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
    strm_value args[2];

    args[0] = kh_value(d->tbl, i);
    args[1] = v;
    if (strm_funcall(NULL, d->func, 2, args, &v) == STRM_NG) {
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

  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  func = args[0];
  t = kh_init(rbk);
  if (!t) return STRM_NG;
  d = malloc(sizeof(struct rbk_data));
  d->tbl = t;
  d->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_rbk, rbk_finish, (void*)d));
  return STRM_OK;
}

struct split_data {
  strm_value sep;
};

static int
iter_split(strm_stream* strm, strm_value data)
{
  struct split_data* d = strm->data;
  const char* s;
  strm_int slen;
  const char* t;
  const char* p;
  const char* pend;
  char c;

  if (!strm_string_p(data)) {
    return STRM_NG;
  }
  s = strm_str_ptr(d->sep);
  slen = strm_str_len(d->sep);
  c = s[0];
  t = p = strm_str_ptr(data);
  pend = p + strm_str_len(data) - slen;
  while (p<pend) {
    if (*p == c) {
      if (memcmp(p, s, slen) == 0) {
        if (!(slen == 1 && c == ' ' && (p-t) == 0)) {
          strm_emit(strm, strm_str_new(t, p-t), NULL);
        }
        t = p + slen;
      }
    }
    p++;
  }
  pend = strm_str_ptr(data) + strm_str_len(data);
  strm_emit(strm, strm_str_new(t, pend-t), NULL);
  return STRM_OK;
}

static int
exec_split(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct split_data* d = malloc(sizeof(struct split_data));

  switch (argc) {
  case 0:
    d->sep = strm_str_lit(" ");
    break;
  case 1:
    if (!strm_string_p(args[0])) {
      strm_raise(strm, "need string separator");
      return STRM_NG;
    }
    d->sep = args[0];
    break;
  default:
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  if (strm_str_len(d->sep) < 1) {
    strm_raise(strm, "separator string too short");
    return STRM_NG;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_split, NULL, (void*)d));
  return STRM_OK;
}

void
strm_iter_init(strm_state* state)
{
  strm_var_def(state, "seq", strm_cfunc_value(exec_seq));
  strm_var_def(state, "rand", strm_cfunc_value(exec_rand));
  strm_var_def(state, "each", strm_cfunc_value(exec_each));
  strm_var_def(state, "map", strm_cfunc_value(exec_map));
  strm_var_def(state, "flatmap", strm_cfunc_value(exec_flatmap));
  strm_var_def(state, "filter", strm_cfunc_value(exec_filter));
  strm_var_def(state, "count", strm_cfunc_value(exec_count));
  strm_var_def(state, "sum", strm_cfunc_value(exec_sum));
  strm_var_def(state, "reduce", strm_cfunc_value(exec_reduce));
  strm_var_def(state, "reduce_by_key", strm_cfunc_value(exec_rbk));
  strm_var_def(state, "split", strm_cfunc_value(exec_split));
}
