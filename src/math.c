#include <math.h>
#include "strm.h"

static int
math_sqrt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_flt_value(sqrt(strm_value_flt(args[0])));
  return STRM_OK;
}

static int
math_sin(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  f = strm_value_flt(args[0]);
  *ret = strm_flt_value(sin(f));
  return STRM_OK;
}

static int
math_cos(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  f = strm_value_flt(args[0]);
  *ret = strm_flt_value(cos(f));
  return STRM_OK;
}

static int
math_tan(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_flt_value(tan(f));
  return STRM_OK;
}

static int
math_pow(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_flt_value(pow(x, y));
  return STRM_OK;
}

void
strm_math_init(strm_state* state)
{
  strm_var_def(state, "PI", strm_flt_value(M_PI));
  strm_var_def(state, "E", strm_flt_value(M_E));
  strm_var_def(state, "sqrt", strm_cfunc_value(math_sqrt));
  strm_var_def(state, "sin", strm_cfunc_value(math_sin));
  strm_var_def(state, "cos", strm_cfunc_value(math_cos));
  strm_var_def(state, "tan", strm_cfunc_value(math_tan));
  strm_var_def(state, "pow", strm_cfunc_value(math_pow));
}
