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

#define round_func(func) \
static int \
math_ ## func(strm_stream* strm, int argc, strm_value* args, strm_value* ret)\
{\
  double x;\
  strm_int d = 0;\
\
  strm_get_args(strm, argc, args, "f|i", &x, &d);\
  if (argc == 1) {\
    *ret = strm_flt_value(func(x));\
  }\
  else {\
    double f = pow(10, d);\
    *ret = strm_flt_value(func(x*f)/f);\
  }\
  return STRM_OK;\
}

round_func(round);
round_func(ceil);
round_func(floor);
round_func(trunc);

static int
num_number(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x;

  strm_get_args(strm, argc, args, "f", &x);
  *ret = strm_flt_value(x);
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
  strm_var_def(state, "round", strm_cfunc_value(math_round));
  strm_var_def(state, "ceil", strm_cfunc_value(math_ceil));
  strm_var_def(state, "floor", strm_cfunc_value(math_floor));
  strm_var_def(state, "trunc", strm_cfunc_value(math_trunc));
  strm_var_def(state, "int", strm_cfunc_value(math_trunc));
  strm_var_def(state, "number", strm_cfunc_value(num_number));
}
