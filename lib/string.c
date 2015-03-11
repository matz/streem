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

struct sym_key {
  const char *ptr;
  size_t len;
};

static khint_t
sym_hash(struct sym_key key)
{
  const char *s = key.ptr;
  khint_t h = (khint_t)*s;
  size_t len = key.len;

  s++;
  while (len--) {
    h = (h << 5) - h + (khint_t)*s++;
  }
  return h;
}

static khint_t
sym_eq(struct sym_key a, struct sym_key b)
{
  if (a.len != b.len) return FALSE;
  if (memcmp(a.ptr, b.ptr, a.len) == 0) return TRUE;
  return FALSE;
}


KHASH_INIT(sym, struct sym_key, struct strm_string*, 1, sym_hash, sym_eq);

static khash_t(sym) *sym_table;

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
  khiter_t k;
  struct sym_key key;
  int ret;

  if (!sym_table) {
    sym_table = kh_init(sym);
  }
  key.ptr = p;
  key.len = len;
  k = kh_put(sym, sym_table, key, &ret);

  if (ret == 0) {               /* found */
    return kh_value(sym_table, k);
  }
  else {
    struct strm_string *str;

    /* allocate strm_string */
    if (readonly_data_p(p)) {
      str = strm_str_alloc(p, len);
    }
    else {
      char *buf = malloc(len);
      if (p) {
        memcpy(buf, p, len);
      }
      else {
        memset(buf, 0, len);
      }
      str = strm_str_alloc(buf, len);
    }
    kh_value(sym_table, k) = str;
    return str;
  }
}

int
strm_str_eq(struct strm_string *a, struct strm_string *b)
{
  if (a == b) return TRUE;
  if (a->len != b->len) return FALSE;
  if (memcmp(a->ptr, b->ptr, a->len) == 0) return TRUE;
  return FALSE;
}
