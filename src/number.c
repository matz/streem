#include "strm.h"

static int
num_plus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "vv", &x, &y);
  if (strm_int_p(x) && strm_int_p(y)) {
    *ret = strm_int_value(strm_value_int(x)+strm_value_int(y));
    return STRM_OK;
  }
  if (strm_number_p(x) && strm_number_p(y)) {
    *ret = strm_flt_value(strm_value_flt(x)+strm_value_flt(y));
    return STRM_OK;
  }
  strm_raise(strm, "number required");
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
    if (strm_flt_p(args[0])) {
      *ret = strm_flt_value(-strm_value_flt(args[0]));
      return STRM_OK;
    }
  }
  else {
    strm_value x, y;

    strm_get_args(strm, argc, args, "vv", &x, &y);
    if (strm_int_p(x) && strm_int_p(y)) {
      *ret = strm_int_value(strm_value_int(x)-strm_value_int(y));
      return STRM_OK;
    }
    if (strm_number_p(x) && strm_number_p(y)) {
      *ret = strm_flt_value(strm_value_flt(x)-strm_value_flt(y));
      return STRM_OK;
    }
  }
  strm_raise(strm, "number required");
  return STRM_NG;
}

static int
num_mult(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value x, y;

  strm_get_args(strm, argc, args, "vv", &x, &y);
  if (strm_int_p(x) && strm_int_p(y)) {
    *ret = strm_int_value(strm_value_int(x)*strm_value_int(y));
    return STRM_OK;
  }
  if (strm_flt_p(x) && strm_flt_p(y)) {
    *ret = strm_flt_value(strm_value_flt(x)*strm_value_flt(y));
    return STRM_OK;
  }
  return STRM_NG;
}

static int
num_div(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_flt_value(x/y);
  return STRM_OK;
}

static int
num_number(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x;

  strm_get_args(strm, argc, args, "f", &x);
  *ret = strm_flt_value(x);
  return STRM_OK;
}

strm_state* strm_ns_number;

void
strm_number_init(strm_state* state)
{
  strm_ns_number = strm_ns_new(NULL);
  strm_var_def(strm_ns_number, "+", strm_cfunc_value(num_plus));
  strm_var_def(strm_ns_number, "-", strm_cfunc_value(num_minus));
  strm_var_def(strm_ns_number, "*", strm_cfunc_value(num_mult));
  strm_var_def(strm_ns_number, "/", strm_cfunc_value(num_div));
  strm_var_def(state, "number", strm_cfunc_value(num_number));
}
