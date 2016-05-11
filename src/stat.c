#include "strm.h"
#include <math.h>

struct sum_data {
  double sum;
  double c;
  strm_int num;
};

static int
iter_sum(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  double y = strm_value_flt(data) - d->c;
  double t = d->sum + y;
  d->c = (t - d->sum) - y;
  d->sum = t;
  d->num++;
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
avg_finish(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  strm_emit(strm, strm_flt_value(d->sum/d->num), NULL);
  return STRM_OK;
}

static int
exec_sum_avg(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int avg)
{
  struct sum_data* d;

  if (argc != 0) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  d = malloc(sizeof(struct sum_data));
  if (!d) return STRM_NG;
  d->sum = 0;
  d->c = 0;
  d->num = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sum,
                                           avg ? avg_finish : sum_finish, (void*)d));
  return STRM_OK;
}

static int
exec_sum(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_sum_avg(strm, argc, args, ret, FALSE);
}

static int
exec_avg(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_sum_avg(strm, argc, args, ret, TRUE);
}

static int
ary_sum_avg(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int avg)
{
  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  else {
    strm_array values = strm_value_ary(args[0]);
    int i, len = strm_ary_len(values);
    strm_value* v = strm_ary_ptr(values);
    double sum = 0;
    double c = 0;

    for (i=0; i<len; i++) {
      double y = strm_value_flt(v[i]) - c;
      double t = sum + y;
      c = (t - sum) - y;
      sum =  t;
    }
    if (avg) {
      *ret = strm_flt_value(sum/len);
    }
    else {
      *ret = strm_flt_value(sum);
    }
    return STRM_OK;
  }
}

static int
ary_sum(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_sum_avg(strm, argc, args, ret, FALSE);
}

static int
ary_avg(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_sum_avg(strm, argc, args, ret, TRUE);
}

struct stdev_data {
  strm_int num;
  double s1, s2;
};

static int
iter_stdev(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double x = strm_value_flt(data);

  d->num++;
  x -= d->s1;
  d->s1 += x/d->num;
  d->s2 += (d->num-1) * x * x / d->num;
  return STRM_OK;
}

static int
stdev_finish(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double s = sqrt(d->s2 / (d->num-1));
  strm_emit(strm, strm_flt_value(s), NULL);
  return STRM_OK;
}

static int
var_finish(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double s = d->s2 / (d->num-1);
  strm_emit(strm, strm_flt_value(s), NULL);
  return STRM_OK;
}

static int
exec_var_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int stdev)
{
  struct stdev_data* d;

  if (argc != 0) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  d = malloc(sizeof(struct stdev_data));
  if (!d) return STRM_NG;
  d->num = 0;
  d->s1 = d->s2 = 0.0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_stdev,
                                           stdev ? stdev_finish : var_finish, (void*)d));
  return STRM_OK;
}

static int
exec_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, TRUE);
}

static int
exec_variance(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, FALSE);
}

static int
ary_var_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int stdev)
{
  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  else {
    strm_array values = strm_value_ary(args[0]);
    int i, len = strm_ary_len(values);
    strm_value* v = strm_ary_ptr(values);
    double s1, s2;

    s1 = s2 = 0.0;
    for (i=0; i<len; i++) {
      double x = strm_value_flt(v[i]);
      x -= s1;
      s1 += x/(i+1);
      s2 += i * x * x / (i+1);
    }
    s2 = s2 / (i-1);
    if (stdev) {
      s2 = sqrt(s2);
    }
    *ret = strm_flt_value(s2);
    return STRM_OK;
  }
}

static int
ary_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_var_stdev(strm, argc, args, ret, TRUE);
}

static int
ary_var(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_var_stdev(strm, argc, args, ret, FALSE);
}

void xorshift64init(void);
uint64_t xorshift64star(void);

struct sample_data {
  strm_int i;
  strm_int len;
  strm_value samples[0];
};

static int
iter_sample(strm_stream* strm, strm_value data)
{
  struct sample_data* d = strm->data;
  uint64_t r;

  if (d->i < d->len) {
    d->samples[d->i++] = data;
    return STRM_OK;
  }
  xorshift64init();
  r = xorshift64star()%(d->i);
  if (r < d->len) {
    d->samples[r] = data;
  }
  d->i++;
  return STRM_OK;
}

static int
finish_sample(strm_stream* strm, strm_value data)
{
  struct sample_data* d = strm->data;
  strm_int i, len=d->len;

  for (i=0; i<len; i++) {
    strm_emit(strm, d->samples[i], NULL);
  }
  free(d);
  return STRM_OK;
}

static int
exec_sample(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sample_data* d;
  strm_int len;

  if (argc != 1) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  if (!strm_num_p(args[0])) {
    strm_raise(strm, "require number of samples");
    return STRM_NG;
  }
  len = strm_value_int(args[0]);
  d = malloc(sizeof(struct sample_data)+sizeof(strm_value)*len);
  if (!d) return STRM_NG;
  d->len = len;
  d->i = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sample,
                                           finish_sample, (void*)d));
  return STRM_OK;
}

void
strm_stat_init(strm_state* state)
{
  strm_var_def(state, "sum", strm_cfunc_value(exec_sum));
  strm_var_def(state, "average", strm_cfunc_value(exec_avg));
  strm_var_def(state, "stdev", strm_cfunc_value(exec_stdev));
  strm_var_def(state, "variance", strm_cfunc_value(exec_variance));
  strm_var_def(state, "sample", strm_cfunc_value(exec_sample));

  strm_var_def(strm_array_ns, "sum", strm_cfunc_value(ary_sum));
  strm_var_def(strm_array_ns, "average", strm_cfunc_value(ary_avg));
  strm_var_def(strm_array_ns, "stdev", strm_cfunc_value(ary_stdev));
  strm_var_def(strm_array_ns, "variance", strm_cfunc_value(ary_var));
}
