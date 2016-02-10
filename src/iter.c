#include "strm.h"

struct seq_data {
  strm_int n;
  strm_int end;
  strm_int inc;
};

static int
gen_seq(strm_stream* strm, strm_value data)
{
  struct seq_data *s = strm->data;

  if (s->end > 0 && s->n > s->end) {
    strm_stream_close(strm);
    return STRM_OK;
  }
  strm_emit(strm, strm_int_value(s->n), gen_seq);
  s->n += s->inc;
  return STRM_OK;
}

static int
exec_seq(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_int start=1, end=-1, inc=1;
  struct seq_data *s;

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
  s = malloc(sizeof(struct seq_data));
  s->n = start;
  s->inc = inc;
  s->end = end;
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_seq, NULL, (void*)s));
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
  strm->data = NULL;
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
  struct map_data *m = strm->data;
  strm_value val;

  if (strm_funcall(NULL, m->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  return STRM_OK;
}

static int
exec_each(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_consumer, iter_each, NULL, (void*)m));
  return STRM_OK;
}

static int
iter_map(strm_stream* strm, strm_value data)
{
  struct map_data *m = strm->data;
  strm_value val;

  if (strm_funcall(strm, m->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  strm_emit(strm, val, NULL);
  return STRM_OK;
}

static int
exec_map(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_map, NULL, (void*)m));
  return STRM_OK;
}

static int
iter_flatmap(strm_stream* strm, strm_value data)
{
  struct map_data *m = strm->data;
  strm_value val;
  strm_int i, len;
  strm_value* e;

  if (strm_funcall(strm, m->func, 1, &data, &val) == STRM_NG) {
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
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_flatmap, NULL, (void*)m));
  return STRM_OK;
}

static int
iter_filter(strm_stream* strm, strm_value data)
{
  struct map_data *m = strm->data;
  strm_value val;

  if (strm_funcall(NULL, m->func, 1, &data, &val) == STRM_NG) {
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
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_filter, NULL, (void*)m));
  return STRM_OK;
}

struct count_data {
  strm_int count;
};

static int
iter_count(strm_stream* strm, strm_value data)
{
  struct count_data *s = strm->data;

  s->count++;
  return STRM_OK;
}

static int
count_finish(strm_stream* strm, strm_value data)
{
  struct count_data *s = strm->data;

  strm_emit(strm, strm_int_value(s->count), NULL);
  return STRM_OK;
}

static int
exec_count(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct count_data* c = malloc(sizeof(struct count_data));
  c->count = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_count, count_finish, (void*)c));
  return STRM_OK;
}

struct sum_data {
  strm_int sum;
  strm_value func;
};

static int
iter_sum(strm_stream* strm, strm_value data)
{
  struct sum_data *s = strm->data;

  if (!strm_nil_p(s->func)) {
    if (strm_funcall(NULL, s->func, 1, &data, &data) == STRM_NG) {
      return STRM_NG;
    }
  }
  s->sum += strm_value_int(data);
  return STRM_OK;
}

static int
sum_finish(strm_stream* strm, strm_value data)
{
  struct sum_data *s = strm->data;

  strm_emit(strm, strm_int_value(s->sum), NULL);
  return STRM_OK;
}

static int
exec_sum(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sum_data* s;
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
  s = malloc(sizeof(struct sum_data));
  if (!s) return STRM_NG;
  s->sum = 0;
  s->func = func;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sum, sum_finish, (void*)s));
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
}
