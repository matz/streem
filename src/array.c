#include "strm.h"

int
strm_array_p(strm_value v)
{
  switch (strm_value_tag(v)) {
  case STRM_TAG_ARRAY:
  case STRM_TAG_STRUCT:
    return TRUE;
  default:
    return FALSE;
  }
}

strm_array
strm_ary_new(const strm_value* p, strm_int len)
{
  struct strm_array* ary;
  strm_value *buf;

  ary = malloc(sizeof(struct strm_array)+sizeof(strm_value)*len);
  buf = (strm_value*)&ary[1];

  if (p) {
    memcpy(buf, p, sizeof(strm_value)*len);
  }
  else {
    memset(buf, 0, sizeof(strm_value)*len);
  }
  ary->ptr = buf;
  ary->len = len;
  ary->ns = NULL;
  ary->headers = strm_ary_null;
  return STRM_TAG_ARRAY | (strm_value)((intptr_t)ary & STRM_VAL_MASK);
}

int
strm_ary_eq(strm_array a, strm_array b)
{
  strm_int i, len;

  if (a == b) return TRUE;
  if (strm_ary_len(a) != strm_ary_len(b)) return FALSE;
  for (i=0, len=strm_ary_len(a); i<len; i++) {
    if (!strm_value_eq(strm_ary_ptr(a)[i], strm_ary_ptr(b)[i])) {
      return FALSE;
    }
  }
  return TRUE;
}

struct strm_array*
strm_ary_struct(strm_value v)
{
  return (struct strm_array*)strm_value_vptr(v);
}

strm_state* strm_ns_array;

static int
ary_length(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* v;
  strm_int len;

  strm_get_args(strm, argc, args, "a", &v, &len);
  *ret = strm_int_value(len);
  return STRM_OK;
}

static int
ary_reverse(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_value* v;
  strm_value* v2;
  strm_int len;
  strm_array ary;
  strm_int i;

  strm_get_args(strm, argc, args, "a", &v, &len);
  ary = strm_ary_new(NULL, len);
  v2 = strm_ary_ptr(ary);
  for (i=0; i<len; i++) {
    v2[len-i-1] = v[i];
  }
  *ret = strm_ary_value(ary);
  return STRM_OK;
}

static int
ary_minmax(strm_stream* strm, int argc, strm_value* args, strm_value* ret, int min)
{
  strm_value func = strm_nil_value();
  int i, len;
  strm_value* v;
  strm_value e, val;
  double num, f;

  strm_get_args(strm, argc, args, "a|v", &v, &len, &func);
  if (len == 0) {
    *ret = strm_nil_value();
    return STRM_OK;
  }
  val = v[0];
  if (argc == 2) {
    if (strm_funcall(strm, func, 1, &v[0], &e) == STRM_NG) {
      return STRM_NG;
    }
  }
  else {
    e = v[0];
  }
  num = strm_value_flt(e);
  for (i=1; i<len; i++) {
    if (argc == 2) {
      if (strm_funcall(strm, func, 1, &v[i], &e) == STRM_NG) {
        return STRM_NG;
      }
    }
    else {
      e = v[0];
    }
    f = strm_value_flt(e);
    if (min) {
      if (num > f) {
        num = f;
        val = v[i];
      }
    }
    else {
      if (num < f) {
        num = f;
        val = v[i];
      }
    }
  }
  *ret = val;
  return STRM_OK;
}

static int
ary_min(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_minmax(strm, argc, args, ret, TRUE);
}

static int
ary_max(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  return ary_minmax(strm, argc, args, ret, FALSE);
}

void
strm_array_init(strm_state* state)
{
  strm_ns_array = strm_ns_new(NULL);
  strm_var_def(strm_ns_array, "length", strm_cfunc_value(ary_length));
  strm_var_def(strm_ns_array, "reverse", strm_cfunc_value(ary_reverse));
  strm_var_def(strm_ns_array, "min", strm_cfunc_value(ary_min));
  strm_var_def(strm_ns_array, "max", strm_cfunc_value(ary_max));
}
