#include "strm.h"
#include "khash.h"

#ifdef NO_READONLY_DATA_CHECK

static inline int
readonly_data_p(const char *s)
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

static struct strm_string*
strm_str_alloc(const char *p, size_t len)
{
  struct strm_string *str = malloc(sizeof(struct strm_string));

  str->ptr = p;
  str->len = len;
  str->type = STRM_OBJ_STRING;

  return str;
}

struct strm_string*
strm_str_new(const char *p, size_t len)
{
  if (readonly_data_p(p)) {
    return strm_str_alloc(p, len);
  }
  else {
    char *buf = malloc(len);
    if (p) {
      memcpy(buf, p, len);
    }
    return strm_str_alloc(buf, len);
  }
}

int
strm_str_eq(struct strm_string *a, struct strm_string *b)
{
  if (a->len != b->len) return FALSE;
  if (memcmp(a->ptr, b->ptr, a->len) == 0) return TRUE;
  return FALSE;
}
