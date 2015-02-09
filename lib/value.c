#include "strm.h"
#include <assert.h>

strm_value
strm_ptr_value(void *p)
{
  strm_value v;

  v.type = STRM_VALUE_PTR;
  v.val.p = p;
  return v;
}

strm_value
strm_bool_value(int i)
{
  strm_value v;

  v.type = STRM_VALUE_BOOL;
  v.val.i = i ? 1 : 0;
  return v;
}

strm_value
strm_int_value(long i)
{
  strm_value v;

  v.type = STRM_VALUE_INT;
  v.val.i = i;
  return v;
}

strm_value
strm_flt_value(double f)
{
  strm_value v;

  v.type = STRM_VALUE_FLT;
  v.val.f = f;
  return v;
}

void*
strm_value_ptr(strm_value v)
{
  assert(v.type == STRM_VALUE_PTR);
  return v.val.p;
}

void*
strm_value_obj(strm_value v, enum strm_obj_type t)
{
  struct strm_object *p;

  assert(v.type == STRM_VALUE_PTR);
  p = v.val.p;
  assert(p->type == t);
  return v.val.p;
}

int
strm_value_bool(strm_value v)
{
  assert(v.type == STRM_VALUE_BOOL);
  return v.val.i ? 1 : 0;
}

long
strm_value_int(strm_value v)
{
  assert(v.type == STRM_VALUE_INT);
  return v.val.i;
}

double
strm_value_flt(strm_value v)
{
  assert(v.type == STRM_VALUE_FLT);
  return v.val.f;
}
