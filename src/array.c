#include "strm.h"

strm_array*
strm_ary_new(const strm_value* p, size_t len)
{
  strm_array *ary;
  strm_value *buf;

  ary = malloc(sizeof(strm_array)+sizeof(strm_value)*len);
  ary->type = STRM_OBJ_ARRAY;
  buf = (strm_value*)&ary[1];

  if (p) {
    memcpy(buf, p, sizeof(strm_value)*len);
  }
  else {
    memset(buf, 0, sizeof(strm_value)*len);
  }
  ary->ptr = buf;
  ary->len = len;

  return ary;
}

int
strm_ary_eq(strm_array* a, strm_array* b)
{
  size_t i, len;

  if (a == b) return TRUE;
  if (a->len != b->len) return FALSE;
  for (i=0, len=a->len; i<len; i++) {
    if (!strm_value_eq(a->ptr[i], b->ptr[i])) {
      return FALSE;
    }
  }
  return TRUE;
}
