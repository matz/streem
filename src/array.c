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
  return STRM_TAG_ARRAY | ((strm_value)ary & STRM_VAL_MASK);
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
  return (struct strm_array*)strm_value_val(v);
}
