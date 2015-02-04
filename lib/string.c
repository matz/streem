#include "strm.h"

struct strm_string*
strm_str_new(const char *p,size_t len)
{
  struct strm_string *str;
  char *buf;

  str = malloc(sizeof(struct strm_string));
  buf = malloc(len);
  if (p) {
    memcpy(buf, p, len);
  }
  str->ptr = buf;
  str->len = len;

  return str;
}
