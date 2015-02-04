#include "strm.h"
#include <assert.h>

strm_value
strm_ptr_value(void *p)
{
  strm_value v;

  v.type = STRM_TT_PTR;
  v.val.p = p;
  return v;
}

strm_value
strm_int_value(long i)
{
  strm_value v;

  v.type = STRM_TT_INT;
  v.val.i = i;
  return v;
}

strm_value
strm_flt_value(double f)
{
  strm_value v;

  v.type = STRM_TT_FLT;
  v.val.f = f;
  return v;
}

void*
strm_value_ptr(strm_value v)
{
  assert(v.type == STRM_TT_PTR);
  return v.val.p;
}

void*
strm_value_obj(strm_value v, enum strm_obj_type t)
{
  struct strm_object *p;

  assert(v.type == STRM_TT_PTR);
  p = v.val.p;
  assert(p->type == t);
  return v.val.p;
}

long
strm_value_int(strm_value v)
{
  assert(v.type == STRM_TT_INT);
  return v.val.i;
}

double
strm_value_flt(strm_value v)
{
  assert(v.type == STRM_TT_FLT);
  return v.val.f;
}
