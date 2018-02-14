#include "strm.h"
#include "khash.h"
#include <pthread.h>

#if defined(NO_READONLY_DATA_CHECK) || defined(_WIN32) || defined(__CYGWIN__)

static inline int
readonly_data_p(const char* s)
{
  return 0;
}

#elif defined(__APPLE__) && defined(__MACH__)

#include <mach-o/getsect.h>

static inline int
readonly_data_p(const char* p)
{
  return (void*)get_etext() < (void*)p && (void*)p < (void*)get_edata();
}

#else

extern char _etext[];
extern char __init_array_start[];

static inline int
readonly_data_p(const char* p)
{
  return _etext < p && p < (char*)&__init_array_start;
}
#endif

struct sym_key {
  const char *ptr;
  strm_int len;
};

static khint_t
sym_hash(struct sym_key key)
{
  const char *s = key.ptr;
  khint_t h;
  strm_int len = key.len;

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

KHASH_INIT(sym, struct sym_key, strm_string, 1, sym_hash, sym_eq);

static pthread_mutex_t sym_mutex = PTHREAD_MUTEX_INITIALIZER;
static khash_t(sym) *sym_table;


#if __BYTE_ORDER == __LITTLE_ENDIAN
# define VALP_PTR(p) ((char*)p)
#else
/* big endian */
# define VALP_PTR(p) (((char*)p)+4)
#endif
#define VAL_PTR(v) VALP_PTR(&v)

static strm_string
str_new(const char* p, strm_int len, int foreign)
{
  strm_value tag;
  strm_value val;
  char* s;

  if (!p) goto mkbuf;
  if (len < 6) {
    tag = STRM_TAG_STRING_I;
    val = 0;
    s = VAL_PTR(val)+1;
    memcpy(s, p, len);
    s[-1] = len;
  }
  else if (len == 6) {
    tag = STRM_TAG_STRING_6;
    val = 0;
    s = VAL_PTR(val);
    memcpy(s, p, len);
  }
  else {
    struct strm_string* str;

    if (p && (foreign || readonly_data_p(p))) {
      tag = STRM_TAG_STRING_F;
      str = malloc(sizeof(struct strm_string));
      str->ptr = p;
    }
    else {
      char *buf;

    mkbuf:
      tag = STRM_TAG_STRING_O;
      str = malloc(sizeof(struct strm_string)+len+1);
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
    val = strm_tag_vptr(str, 0);
  }
  return tag | (val & STRM_VAL_MASK);
}

static strm_string
str_intern(const char *p, strm_int len, int foreign)
{
  khiter_t k;
  struct sym_key key;
  int ret;
  strm_string str;

  if (len <= 6) {
    return str_new(p, len, foreign);
  }
  if (!sym_table) {
    sym_table = kh_init(sym);
  }
  key.ptr = p;
  key.len = len;
  k = kh_put(sym, sym_table, key, &ret);

  if (ret == 0) {               /* found */
    return kh_value(sym_table, k);
  }
  str = str_new(p, len, foreign);
  kh_key(sym_table, k).ptr = strm_str_ptr(str);
  kh_value(sym_table, k) = str;

  return str;
}

#ifndef STRM_STR_INTERN_LIMIT
#define STRM_STR_INTERN_LIMIT 64
#endif

strm_string
strm_str_new(const char* p, strm_int len)
{
  if (!strm_event_loop_started) {
    /* single thread mode */
    if (p && (len < STRM_STR_INTERN_LIMIT || readonly_data_p(p))) {
      return str_intern(p, len, 0);
    }
  }
  return str_new(p, len, 0);
}

strm_string
strm_str_static(const char* p, strm_int len)
{
  return str_new(p, len, 1);
}

strm_string
strm_str_intern(const char* p, strm_int len)
{
  strm_string str;

  assert(p!=NULL);
  if (!strm_event_loop_started) {
    return str_intern(p, len, 0);
  }
  pthread_mutex_lock(&sym_mutex);
  str = str_intern(p, len, 0);
  pthread_mutex_unlock(&sym_mutex);

  return str;
}

strm_string
strm_str_intern_str(strm_string str)
{
  if (strm_str_intern_p(str)) {
    return str;
  }
  if (!strm_event_loop_started) {
    return str_intern(strm_str_ptr(str), strm_str_len(str), 0);
  }
  pthread_mutex_lock(&sym_mutex);
  str = str_intern(strm_str_ptr(str), strm_str_len(str), 0);
  pthread_mutex_unlock(&sym_mutex);

  return str;
}

strm_string
strm_str_intern_static(const char* p, strm_int len)
{
  return str_intern(p, len, 1);
}

int
strm_str_intern_p(strm_string s)
{
  switch (strm_value_tag(s)) {
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
  case STRM_TAG_STRING_F:
    return TRUE;
  case STRM_TAG_STRING_O:
  default:
    return FALSE;
  }
}

int
strm_str_eq(strm_string a, strm_string b)
{
  if (a == b) return TRUE;
  if (strm_value_tag(a) == STRM_TAG_STRING_F &&
      strm_value_tag(b) == STRM_TAG_STRING_F) {
    /* pointer comparison is OK if strings are interned */
    return FALSE;
  }
  if (strm_str_len(a) != strm_str_len(b)) return FALSE;
  if (memcmp(strm_str_ptr(a), strm_str_ptr(b), strm_str_len(a)) == 0)
    return TRUE;
  return FALSE;
}

int
strm_str_p(strm_value v)
{
  switch (strm_value_tag(v)) {
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
  case STRM_TAG_STRING_F:
  case STRM_TAG_STRING_O:
    return TRUE;
  default:
    return FALSE;
  }
}

const char*
strm_strp_ptr(strm_string* s)
{
  switch (strm_value_tag(*s)) {
  case STRM_TAG_STRING_I:
    return VALP_PTR(s)+1;
  case STRM_TAG_STRING_6:
    return VALP_PTR(s);
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    {
      struct strm_string* str = (struct strm_string*)strm_value_vptr(*s);
      return str->ptr;
    }
  default:
    return NULL;
  }
}

const char*
strm_str_cstr(strm_string s, char buf[])
{
  strm_int len;

  switch (strm_value_tag(s)) {
  case STRM_TAG_STRING_I:
    len = VAL_PTR(s)[0];
    memcpy(buf, VAL_PTR(s)+1, len);
    buf[len] = '\0';
    return buf;
  case STRM_TAG_STRING_6:
    memcpy(buf, VAL_PTR(s), 6);
    buf[6] = '\0';
    return buf;
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    {
      struct strm_string* str = (struct strm_string*)strm_value_vptr(s);
      return str->ptr;
    }
  default:
    return NULL;
  }
}

strm_int
strm_str_len(strm_string s)
{
  switch (strm_value_tag(s)) {
  case STRM_TAG_STRING_I:
    return (strm_int)VAL_PTR(s)[0];
  case STRM_TAG_STRING_6:
    return 6;
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    {
      struct strm_string* str = (struct strm_string*)strm_value_vptr(s);

      return str->len;
    }
  default:
    return 0;
  }
}

int
strm_string_p(strm_string s)
{
  switch (strm_value_tag(s)) {
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    return TRUE;
  default:
    return FALSE;
  }
}

strm_state* strm_ns_string;

static int
str_length(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  char* p;
  strm_int len;

  strm_get_args(strm, argc, args, "s", &p, &len);
  *ret = strm_int_value(len);
  return STRM_OK;
}

static int
str_split(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  const char* s;
  strm_int slen;
  const char* b;
  const char* t;
  const char* p;
  strm_int plen;
  const char* pend;
  char c;
  strm_int n = 0;
  strm_array ary;
  strm_value* sps;
  strm_int i;

  strm_get_args(strm, argc, args, "s|s", &p, &plen, &s, &slen);
  if (argc == 1) {
    s = " ";
    slen = 1;
  }

  /* count number of split strings */
  c = s[0];
  b = t = p;
  pend = p + plen - slen;
  n = 0;
  while (p<pend) {
    if (*p == c) {
      if (memcmp(p, s, slen) == 0) {
        if (!(slen == 1 && c == ' ' && (p-t) == 0)) {
          n++;
        }
        t = p + slen;
      }
    }
    p++;
  }
  n++;

  /* actual split */
  ary = strm_ary_new(NULL, n);
  sps = strm_ary_ptr(ary);
  c = s[0];
  p = t = b;
  i = 0;
  while (p<pend) {
    if (*p == c) {
      if (memcmp(p, s, slen) == 0) {
        if (!(slen == 1 && c == ' ' && (p-t) == 0)) {
          sps[i++] = strm_str_new(t, p-t);
        }
        t = p + slen;
      }
    }
    p++;
  }
  pend = b + plen;
  sps[i++] = strm_str_new(t, pend-t);
  *ret = strm_ary_value(ary);
  return STRM_OK;
}

static int
str_plus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_string str1, str2, str3;
  char *p;

  strm_get_args(strm, argc, args, "SS", &str1, &str2);
  str3 = strm_str_new(NULL, strm_str_len(str1) + strm_str_len(str2));

  p = (char*)strm_str_ptr(str3);
  memcpy(p, strm_str_ptr(str1), strm_str_len(str1));
  memcpy(p+strm_str_len(str1), strm_str_ptr(str2), strm_str_len(str2));
  p[strm_str_len(str3)] = '\0';
  *ret = strm_str_value(str3);
  return STRM_OK;
}

// https://github.com/mruby/mruby/blob/1.4.0/src/string.c#L215-L239
static const char utf8len_codepage[256] =
{
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,1,1,1,1,1,1,1,1,1,1,1,
};

static int
utf8len(const char* p, const char* e)
{
  strm_int len;
  strm_int i;

  len = utf8len_codepage[(unsigned char)*p];
  if (p + len > e) return 1;
  for (i = 1; i < len; ++i)
    if ((p[i] & 0xc0) != 0x80)
      return 1;
  return len;
}

static int
str_chars(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  const char* str;
  const char* s;
  const char* prev = NULL;
  strm_int slen;
  strm_array ary;
  strm_int n = 0;
  strm_value* sps;
  strm_int i = 0;

  strm_get_args(strm, argc, args, "s", &str, &slen);

  s = str;

  while (*s) {
    prev = s;
    s += utf8len(s, s + slen);
    n++;
  }

  ary = strm_ary_new(NULL, n);
  sps = strm_ary_ptr(ary);
  s = str;

  while (*s) {
    prev = s;
    s += utf8len(s, s + slen);
    sps[i++] = strm_str_new(prev, s - prev);
  }

  *ret = strm_ary_value(ary);
  return STRM_OK;
}

void
strm_string_init(strm_state* state)
{
  strm_ns_string = strm_ns_new(NULL, "string");
  strm_var_def(strm_ns_string, "length", strm_cfunc_value(str_length));
  strm_var_def(strm_ns_string, "split", strm_cfunc_value(str_split));
  strm_var_def(strm_ns_string, "+", strm_cfunc_value(str_plus));
  strm_var_def(strm_ns_string, "chars", strm_cfunc_value(str_chars));
}
