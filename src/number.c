#include "strm.h"
#include <math.h>

static int
num_plus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "NN", &x, &y);
  if (strm_int_p(x) && strm_int_p(y)) {
    *ret = strm_int_value(strm_value_int(x)+strm_value_int(y));
    return STRM_OK;
  }
  if (strm_number_p(x) && strm_number_p(y)) {
    *ret = strm_float_value(strm_value_float(x)+strm_value_float(y));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
num_minus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  if (argc == 1) {
    if (strm_int_p(args[0])) {
      *ret = strm_int_value(-strm_value_int(args[0]));
      return STRM_OK;
    }
    if (strm_float_p(args[0])) {
      *ret = strm_float_value(-strm_value_float(args[0]));
      return STRM_OK;
    }
  }
  else {
    strm_value x, y;

    strm_get_args(strm, argc, args, "NN", &x, &y);
    if (strm_int_p(x) && strm_int_p(y)) {
      *ret = strm_int_value(strm_value_int(x)-strm_value_int(y));
      return STRM_OK;
    }
    if (strm_number_p(x) && strm_number_p(y)) {
      *ret = strm_float_value(strm_value_float(x)-strm_value_float(y));
      return STRM_OK;
    }
  }
  return STRM_NG;
}

static int
num_mult(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "NN", &x, &y);
  if (strm_int_p(x) && strm_int_p(y)) {
    *ret = strm_int_value(strm_value_int(x)*strm_value_int(y));
    return STRM_OK;
  }
  *ret = strm_float_value(strm_value_float(x)*strm_value_float(y));
  return STRM_OK;
}

static int
num_div(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_float_value(x/y);
  return STRM_OK;
}

static int
num_bar(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "ii", &x, &y);
  *ret = strm_int_value(strm_value_int(x)|strm_value_int(y));
  return STRM_OK;
}

static int
num_mod(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x;
  strm_int y;

  strm_get_args(strm, argc, args, "Ni", &x, &y);
  if (strm_int_p(x)) {
    *ret = strm_int_value(strm_value_int(x)%y);
    return STRM_OK;
  }
  if (strm_float_p(x)) {
    *ret = strm_float_value(fmod(strm_value_float(x), y));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
num_gt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_bool_value(x>y);
  return STRM_OK;
}

static int
num_ge(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_bool_value(x>=y);
  return STRM_OK;
}

static int
num_lt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_bool_value(x<y);
  return STRM_OK;
}

static int
num_le(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_bool_value(x<=y);
  return STRM_OK;
}


static int
num_number(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_get_args(strm, argc, args, "N", ret);
  return STRM_OK;
}

strm_state* strm_ns_number;

void
strm_number_init(strm_state* state)
{
  strm_ns_number = strm_ns_new(NULL, "number");
  strm_var_def(strm_ns_number, "+", strm_cfunc_value(num_plus));
  strm_var_def(strm_ns_number, "-", strm_cfunc_value(num_minus));
  strm_var_def(strm_ns_number, "*", strm_cfunc_value(num_mult));
  strm_var_def(strm_ns_number, "/", strm_cfunc_value(num_div));
  strm_var_def(strm_ns_number, "%", strm_cfunc_value(num_mod));
  strm_var_def(strm_ns_number, "|", strm_cfunc_value(num_bar));
  strm_var_def(strm_ns_number, "<", strm_cfunc_value(num_lt));
  strm_var_def(strm_ns_number, "<=", strm_cfunc_value(num_le));
  strm_var_def(strm_ns_number, ">", strm_cfunc_value(num_gt));
  strm_var_def(strm_ns_number, ">=", strm_cfunc_value(num_ge));
  strm_var_def(state, "number", strm_cfunc_value(num_number));
}
