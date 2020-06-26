#include <math.h>
#include "strm.h"

static int
math_sqrt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(sqrt(f));
  return STRM_OK;
}

static int
math_sin(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(sin(f));
  return STRM_OK;
}

static int
math_sinh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(sinh(f));
  return STRM_OK;
}

static int
math_cos(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(cos(f));
  return STRM_OK;
}

static int
math_cosh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(cosh(f));
  return STRM_OK;
}

static int
math_tan(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(tan(f));
  return STRM_OK;
}

static int
math_tanh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(tanh(f));
  return STRM_OK;
}

static int
math_pow(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_float_value(pow(x, y));
  return STRM_OK;
}

static int
fac(int n)
{
  int result = 1;
  if(n == 0 || n == 1) result = 1;
  else result = fac(n - 1) * n;
  return result;
}

static int
math_fac(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  int i;

  strm_get_args(strm, argc, args, "i", &i);
  *ret = strm_float_value(fac(i));
  return STRM_OK;
}

static int
GCD(int a, int b)
{
  return b ? GCD(b, a % b) : a;
}

static int
math_gcd(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  int x,y;

  strm_get_args(strm, argc, args, "ii", &x, &y);
  *ret = strm_float_value(GCD(x,y));
  return STRM_OK;
}

static int
math_fabs(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(abs(f));
  return STRM_OK;
}

static int
math_acosh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(acosh(f));
  return STRM_OK;
}

static int
math_asinh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
 double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(asinh(f));
  return STRM_OK;
}

static int
math_atanh(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(atanh(f));
  return STRM_OK;
}

static int
math_acos(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(acos(f));
  return STRM_OK;
}

static int
math_asin(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(asin(f));
  return STRM_OK;
}

static int
math_atan(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(atan(f));
  return STRM_OK;
}

static int
math_log(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(log(f));
  return STRM_OK;
}

static int
math_log10(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(log10(f));
  return STRM_OK;
}

static int
math_exp(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(exp(f));
  return STRM_OK;
}

static int
math_log2(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(log2(f));
  return STRM_OK;
}

static int
math_erfc(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(erfc(f));
  return STRM_OK;
}

static int
math_cbrt(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double f;

  strm_get_args(strm, argc, args, "f", &f);
  *ret = strm_float_value(cbrt(f));
  return STRM_OK;
}

static int
math_hypot(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x, y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_float_value(hypot(x, y));
  return STRM_OK;
}

static int
math_frexp(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x;
  int y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_float_value(frexp(x, &y));
  return STRM_OK;
}

static int
math_ldexp(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x;
  int y;

  strm_get_args(strm, argc, args, "ff", &x, &y);
  *ret = strm_float_value(ldexp(x, y));
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
    *ret = strm_float_value(func(x));\
  }\
  else {\
    double f = pow(10, d);\
    *ret = strm_float_value(func(x*f)/f);\
  }\
  return STRM_OK;\
}

round_func(round);
round_func(ceil);
round_func(floor);
round_func(trunc);

void
strm_math_init(strm_state* state)
{

  strm_var_def(state, "PI", strm_float_value(M_PI));
  strm_var_def(state, "E", strm_float_value(M_E));

  strm_var_def(state, "sqrt", strm_cfunc_value(math_sqrt));
  strm_var_def(state, "sin", strm_cfunc_value(math_sin));
  strm_var_def(state, "cos", strm_cfunc_value(math_cos));

  strm_var_def(state, "tan", strm_cfunc_value(math_tan));
  strm_var_def(state, "sinh", strm_cfunc_value(math_sinh));
  strm_var_def(state, "cosh", strm_cfunc_value(math_cosh));

  strm_var_def(state, "asin", strm_cfunc_value(math_asin));
  strm_var_def(state, "acos", strm_cfunc_value(math_acos));
  strm_var_def(state, "atan", strm_cfunc_value(math_atan));

  strm_var_def(state, "asinh", strm_cfunc_value(math_asinh));
  strm_var_def(state, "acosh", strm_cfunc_value(math_acosh));
  strm_var_def(state, "atanh", strm_cfunc_value(math_atanh));
  strm_var_def(state, "tanh", strm_cfunc_value(math_tanh));

  strm_var_def(state, "pow", strm_cfunc_value(math_pow));
  strm_var_def(state, "round", strm_cfunc_value(math_round));
  strm_var_def(state, "ceil", strm_cfunc_value(math_ceil));
  strm_var_def(state, "floor", strm_cfunc_value(math_floor));
  strm_var_def(state, "trunc", strm_cfunc_value(math_trunc));
  strm_var_def(state, "int", strm_cfunc_value(math_trunc));
  strm_var_def(state, "fabs", strm_cfunc_value(math_fabs));
  strm_var_def(state, "log", strm_cfunc_value(math_log));
  strm_var_def(state, "log10", strm_cfunc_value(math_log10));
  strm_var_def(state, "log2", strm_cfunc_value(math_log2));
  strm_var_def(state, "exp", strm_cfunc_value(math_exp));
  strm_var_def(state, "erfc", strm_cfunc_value(math_erfc));
  strm_var_def(state, "cbrt", strm_cfunc_value(math_cbrt));
  strm_var_def(state, "hypot", strm_cfunc_value(math_hypot));
  strm_var_def(state, "frexp", strm_cfunc_value(math_frexp));
  strm_var_def(state, "ldexp", strm_cfunc_value(math_ldexp));
  strm_var_def(state, "factorial", strm_cfunc_value(math_fac));
  strm_var_def(state, "gcd", strm_cfunc_value(math_gcd));
}
