#include "strm.h"
#include "khash.h"
#include <pthread.h>

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
  khint_t h;
  size_t len = key.len;

  h = *s++;
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

KHASH_INIT(sym, struct sym_key, strm_string*, 1, sym_hash, sym_eq);

static pthread_mutex_t sym_mutex = PTHREAD_MUTEX_INITIALIZER;
static khash_t(sym) *sym_table;

static strm_string*
str_new(const char *p, size_t len)
{
  strm_string *str;

  if (readonly_data_p(p)) {
    str = malloc(sizeof(strm_string));
    str->ptr = p;
  }
  else {
    char *buf;

    str = malloc(sizeof(strm_string)+len+1);
    buf = (char*)&str[1];
    if (p) {
      memcpy(buf, p, len);
    }
    else {
      memset(buf, 0, len);
    }
    buf[len] = '\0';
    str->ptr = buf;
  }
  str->len = len;
  str->type = STRM_OBJ_STRING;

  return str;
}

static strm_string*
str_intern(const char *p, size_t len)
{
  khiter_t k;
  struct sym_key key;
  int ret;
  strm_string *str;

  if (!sym_table) {
    sym_table = kh_init(sym);
  }
  key.ptr = p;
  key.len = len;
  k = kh_put(sym, sym_table, key, &ret);

  if (ret == 0) {               /* found */
    return kh_value(sym_table, k);
  }
  str = str_new(p, len);
  str->flags |= STRM_STR_INTERNED;
  kh_key(sym_table, k).ptr = str->ptr;
  kh_value(sym_table, k) = str;

  return str;
}

#ifndef STRM_STR_INTERN_LIMIT
#define STRM_STR_INTERN_LIMIT 64
#endif

strm_string*
strm_str_new(const char *p, size_t len)
{
  if (!strm_event_loop_started) {
    /* single thread mode */
    if (len < STRM_STR_INTERN_LIMIT || readonly_data_p(p)) {
      return str_intern(p, len);
    }
  }
  return str_new(p, len);
}

strm_string*
strm_str_intern(const char *p, size_t len)
{
  strm_string *str;

  if (!strm_event_loop_started) {
    return str_intern(p, len);
  }
  pthread_mutex_lock(&sym_mutex);
  str = str_intern(p, len);
  pthread_mutex_unlock(&sym_mutex);

  return str;
}

int
strm_str_eq(strm_string *a, strm_string *b)
{
  if (a == b) return TRUE;
  if (a->flags & b->flags & STRM_STR_INTERNED) {
    /* pointer comparison is OK if strings are interned */
    return FALSE;
  }
  if (a->len != b->len) return FALSE;
  if (memcmp(a->ptr, b->ptr, a->len) == 0) return TRUE;
  return FALSE;
}
