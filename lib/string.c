#include "strm.h"

#ifdef NO_ETEXT_EDATA

static inline int
ro_data_p(const char *s)
{
  return 0;
}

#else

extern char _etext[];
extern char __init_array_start[];

static inline int
readonly_data_p(const char *p)
{
  return _etext < p && p < (char*)&__init_array_start;
}
#endif

struct strm_string*
strm_str_new(const char *p, size_t len)
{
  struct strm_string *str;

  str = malloc(sizeof(struct strm_string));
  if (readonly_data_p(p)) {
    str->ptr = p;
  }
  else {
    char *buf = malloc(len);

    if (p) {
      memcpy(buf, p, len);
    }
    str->ptr = buf;
  }
  str->len = len;

  return str;
}
