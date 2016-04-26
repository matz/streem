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
  d->sum =  t;
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

void
strm_stat_init(strm_state* state)
{
  strm_var_def(state, "sum", strm_cfunc_value(exec_sum));
  strm_var_def(state, "average", strm_cfunc_value(exec_avg));
  strm_var_def(state, "stdev", strm_cfunc_value(exec_stdev));
  strm_var_def(state, "variance", strm_cfunc_value(exec_variance));
}
