#include "strm.h"
#include <math.h>

struct sum_data {
  double sum;
  double c;
  strm_int num;
  strm_value func;
};

static int
iter_sum(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;
  double x, t;

  if (!strm_number_p(data)) {
    return STRM_NG;
  }
  x = strm_value_float(data);
  t = d->sum + x;
  if (fabs(d->sum) >= fabs(x))
    d->c += ((d->sum - t) + x);
  else
    d->c += ((x - t) + d->sum);
  d->sum = t;
  d->num++;
  return STRM_OK;
}

static strm_value
convert_number(strm_stream* strm, strm_value data, strm_value func)
{
  strm_value val;

  if (strm_funcall(strm, func, 1, &data, &val) == STRM_NG) {
    return strm_nil_value();
  }
  if (!strm_number_p(val)) {
    strm_raise(strm, "number required");
    return strm_nil_value();
  }
  return val;
}

static int
iter_sumf(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;
  double x, t;

  data = convert_number(strm, data, d->func);
  if (!strm_number_p(data)) {
    return STRM_NG;
  }
  x = strm_value_float(data);
  t = d->sum + x;
  if (fabs(d->sum) >= fabs(x))
    d->c += ((d->sum - t) + x);
  else
    d->c += ((x - t) + d->sum);
  d->sum = t;
  d->num++;
  return STRM_OK;
}

static int
sum_finish(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  strm_emit(strm, strm_float_value(d->sum+d->c), NULL);
  return STRM_OK;
}

static int
avg_finish(strm_stream* strm, strm_value data)
{
  struct sum_data* d = strm->data;

  strm_emit(strm, strm_float_value((d->sum+d->c)/d->num), NULL);
  return STRM_OK;
}

static int
exec_sum_avg(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int avg)
{
  struct sum_data* d;
  strm_value func;

  strm_get_args(strm, argc, args, "|v", &func);
  d = malloc(sizeof(struct sum_data));
  if (!d) return STRM_NG;
  d->sum = 0;
  d->c = 0;
  d->num = 0;
  if (argc == 0) {
    d->func = strm_nil_value();
    *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sum,
                                             avg ? avg_finish : sum_finish, (void*)d));
  }
  else {
    d->func = func;
    *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sumf,
                                             avg ? avg_finish : sum_finish, (void*)d));
  }
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
  int i, len;
  strm_value* v;
  double sum = 0;
  double c = 0;
  strm_value func;

  strm_get_args(strm, argc, args, "a|v", &v, &len, &func);
  if (argc == 1) {
    for (i=0; i<len; i++) {
      double x, t;

      if (!strm_number_p(v[i])) {
        return STRM_NG;
      }
      x = strm_value_float(v[i]);
      t = sum + x;
      if (fabs(sum) >= fabs(x))
        c += ((sum - t) + x);
      else
        c += ((x - t) + sum);
      sum =  t;
    }
  }
  else {
    for (i=0; i<len; i++) {
      strm_value val;
      double x, t;

      val = convert_number(strm, v[i], func);
      if (!strm_number_p(val)) {
        return STRM_NG;
      }
      x = strm_value_float(val);
      t = sum + x;
      if (fabs(sum) >= fabs(x))
        c += ((sum - t) + x);
      else
        c += ((x - t) + sum);
      sum =  t;
    }
  }
  sum += c;
  if (avg) {
    *ret = strm_float_value(sum/len);
  }
  else {
    *ret = strm_float_value(sum);
  }
  return STRM_OK;
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

struct mvavg_data {
  strm_int num;
  strm_int i;
  strm_int filled;
  strm_value func;
  strm_int func_p;
  double data[0];
};

static int
iter_mvavg(strm_stream* strm, strm_value data)
{
  struct mvavg_data* d = strm->data;
  double sum = 0;
  double c = 0;
  strm_int i, len = d->num;

  if (d->func_p) {
    data = convert_number(strm, data, d->func);
  }
  if (!strm_number_p(data)) {
    strm_raise(strm, "number required");
    return STRM_NG;
  }
  d->data[d->i++] = strm_value_float(data);
  if (d->i == d->num) {
    d->filled = TRUE;
    d->i = 0;
  }
  if (!d->filled) {
    strm_emit(strm, strm_nil_value(), NULL);
    return STRM_OK;
  }
  for (i=0; i<len; i++) {
    double x = d->data[i];
    double t = sum + x;
    if (fabs(sum) >= fabs(x))
      c += ((sum - t) + x);
    else
      c += ((x - t) + sum);
    sum =  t;
  }
  strm_emit(strm, strm_float_value((sum+c)/d->num), NULL);
  return STRM_OK;
}

static int
exec_mvavg(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct mvavg_data* d;
  strm_int n;
  strm_value func;

  strm_get_args(strm, argc, args, "i|v", &n, &func);
  d = malloc(sizeof(struct mvavg_data)+n*sizeof(double));
  if (!d) return STRM_NG;
  d->num = n;
  d->i = 0;
  d->filled = FALSE;
  if (argc == 1) {
    d->func = strm_nil_value();
    d->func_p = FALSE;
  }
  else {
    d->func = func;
    d->func_p = TRUE;
  }
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_mvavg,
                                           NULL, (void*)d));
  return STRM_OK;
}

enum stdev_mode {
  mode_stdev,
  mode_variance,
  mode_mean_stdev,
  mode_mean_variance,
};

struct stdev_data {
  strm_int num;
  double s1, s2;
  strm_value func;
  enum stdev_mode mode;
};

static int
iter_stdev(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double x = strm_value_float(data);

  d->num++;
  x -= d->s1;
  d->s1 += x/d->num;
  d->s2 += (d->num-1) * x * x / d->num;
  return STRM_OK;
}

static int
iter_stdevf(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double x;

  data = convert_number(strm, data, d->func);
  if (!strm_number_p(data)) {
    return STRM_NG;
  }
  x = strm_value_float(data);
  d->num++;
  x -= d->s1;
  d->s1 += x/d->num;
  d->s2 += (d->num-1) * x * x / d->num;
  return STRM_OK;
}

static strm_value
float2(double m, double s)
{
  strm_value buf[2];

  fprintf(stderr, "f2[%f, %f]\n", m, s);
  buf[0] = strm_float_value(m);
  buf[1] = strm_float_value(s);
  return strm_ary_new(buf, 2);
}

static int
stdev_finish(strm_stream* strm, strm_value data)
{
  struct stdev_data* d = strm->data;
  double s;

  switch (d->mode) {
  case mode_stdev:
    s = sqrt(d->s2 / (d->num-1));
    strm_emit(strm, strm_float_value(s), NULL);
    break;
  case mode_variance:
    s = d->s2 / (d->num-1);
    strm_emit(strm, strm_float_value(s), NULL);
    break;
  case mode_mean_stdev:
    s = sqrt(d->s2 / (d->num-1));
    strm_emit(strm, float2(d->s1, s), NULL);
    break;
  case mode_mean_variance:
    s = d->s2 / (d->num-1);
    strm_emit(strm, float2(d->s1, s), NULL);
    break;
  }
  return STRM_OK;
}

static int
exec_var_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret, enum stdev_mode mode)
{
  struct stdev_data* d;
  strm_value func;

  strm_get_args(strm, argc, args, "|v", &func);
  d = malloc(sizeof(struct stdev_data));
  if (!d) return STRM_NG;
  d->num = 0;
  d->s1 = d->s2 = 0.0;
  d->mode = mode;
  if (argc == 0) {
    *ret = strm_stream_value(strm_stream_new(strm_filter, iter_stdev, stdev_finish, (void*)d));
  }
  else {
    d->func = func;
    *ret = strm_stream_value(strm_stream_new(strm_filter, iter_stdevf, stdev_finish, (void*)d));
  }
  return STRM_OK;
}

static int
exec_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, mode_stdev);
}

static int
exec_variance(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, mode_variance);
}

static int
exec_mean_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, mode_mean_stdev);
}

static int
exec_mean_variance(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return exec_var_stdev(strm, argc, args, ret, mode_mean_variance);
}

static int
ary_var_stdev(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int stdev)
{
  strm_value func;
  strm_value* v;
  int i, len;
  double s1, s2;

  strm_get_args(strm, argc, args, "a|v", &v, &len, &func);

  s1 = s2 = 0.0;
  if (argc == 0) {
    for (i=0; i<len; i++) {
      double x = strm_value_float(v[i]);
      x -= s1;
      s1 += x/(i+1);
      s2 += i * x * x / (i+1);
    }
  }
  else {
    for (i=0; i<len; i++) {
      strm_value val;
      double x;

      val = convert_number(strm, v[i], func);
      if (!strm_number_p(val)) {
        return STRM_NG;
      }
      x = strm_value_float(val);
      x -= s1;
      s1 += x/(i+1);
      s2 += i * x * x / (i+1);
    }
  }
  s2 = s2 / (i-1);
  if (stdev) {
    s2 = sqrt(s2);
  }
  *ret = strm_float_value(s2);
  return STRM_OK;
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

struct correl_data {
  strm_int n;
  double sx, sy, sxx, syy, sxy;
};

static int
iter_correl(strm_stream* strm, strm_value data)
{
  struct correl_data* d = strm->data;
  strm_value *v;
  double dx, dy;

  if (!strm_array_p(data) || strm_ary_len(data) != 2) {
    strm_raise(strm, "invalid data");
    return STRM_NG;
  }

  v = strm_ary_ptr(data);
  if (!strm_number_p(v[0]) || !strm_number_p(v[1])) {
    strm_raise(strm, "correl() requires [num, num]");
    return STRM_NG;
  }
  d->n++;
  dx = strm_value_float(v[0]) - d->sx; d->sx += dx / d->n;
  dy = strm_value_float(v[1]) - d->sy; d->sy += dy / d->n;
  d->sxx += (d->n-1) * dx * dx / d->n;
  d->syy += (d->n-1) * dy * dy / d->n;
  d->sxy += (d->n-1) * dx * dy / d->n;
  return STRM_OK;
}

static int
correl_finish(strm_stream* strm, strm_value data)
{
  struct correl_data* d = strm->data;

  d->n--;
  double sxx = sqrt(d->sxx / d->n);
  double syy = sqrt(d->syy / d->n);
  double sxy = d->sxy / (d->n * sxx * syy);
  strm_emit(strm, strm_float_value(sxy), NULL);
  return STRM_OK;
}

static int
exec_correl(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct correl_data* d;

  strm_get_args(strm, argc, args, "");
  d = malloc(sizeof(struct correl_data));
  if (!d) return STRM_NG;
  d->n = 0;
  d->sx = d->sy = d->sxx = d->syy = d->sxy = 0;
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_correl,
                                           correl_finish, (void*)d));
  return STRM_OK;
}

static int
ary_correl(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* v;
  int i, len;
  double sx, sy, sxx, syy, sxy;

  strm_get_args(strm, argc, args, "a", &v, &len);
  sx = sy = sxx = syy = sxy = 0;
  for (i=0; i<len; i++) {
    strm_value data = v[i];
    strm_value* dv;
    double dx, dy;

    if (!strm_array_p(data) || strm_ary_len(data) != 2) {
      /* skip invalid data */
      continue;
    }
    dv = strm_ary_ptr(data);
    dx = strm_value_float(dv[0]) - sx; sx += dx / (i+1);
    dy = strm_value_float(dv[1]) - sy; sy += dy / (i+1);
    sxx += i * dx * dx / (i+1);
    syy += i * dy * dy / (i+1);
    sxy += i * dx * dy / (i+1);
  }
  sxx = sqrt(sxx / (len-1));
  syy = sqrt(syy / (len-1));
  sxy /= (len-1) * sxx * syy;
  *ret = strm_float_value(sxy);
  return STRM_OK;
}

void
strm_stat_init(strm_state* state)
{
  strm_var_def(state, "sum", strm_cfunc_value(exec_sum));
  strm_var_def(state, "average", strm_cfunc_value(exec_avg));
  strm_var_def(state, "mean", strm_cfunc_value(exec_avg));
  strm_var_def(state, "moving_average", strm_cfunc_value(exec_mvavg));
  strm_var_def(state, "rolling_mean", strm_cfunc_value(exec_mvavg));
  strm_var_def(state, "stdev", strm_cfunc_value(exec_stdev));
  strm_var_def(state, "variance", strm_cfunc_value(exec_variance));
  strm_var_def(state, "mean_stdev", strm_cfunc_value(exec_mean_stdev));
  strm_var_def(state, "mean_variance", strm_cfunc_value(exec_mean_variance));
  strm_var_def(state, "correl", strm_cfunc_value(exec_correl));

  strm_var_def(strm_ns_array, "sum", strm_cfunc_value(ary_sum));
  strm_var_def(strm_ns_array, "average", strm_cfunc_value(ary_avg));
  strm_var_def(strm_ns_array, "stdev", strm_cfunc_value(ary_stdev));
  strm_var_def(strm_ns_array, "variance", strm_cfunc_value(ary_var));
  strm_var_def(strm_ns_array, "correl", strm_cfunc_value(ary_correl));
}
