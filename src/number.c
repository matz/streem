#include "strm.h"

static int
num_number(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  double x;

  strm_get_args(strm, argc, args, "f", &x);
  *ret = strm_flt_value(x);
  return STRM_OK;
}

void
strm_number_init(strm_state* state)
{
  strm_var_def(state, "number", strm_cfunc_value(num_number));
}
