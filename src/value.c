#include <ctype.h>
#include <math.h>
#include "strm.h"
#include <assert.h>

strm_value
strm_ptr_value(void* p)
{
  return STRM_TAG_PTR | ((strm_value)p & STRM_VAL_MASK);
}

strm_value
strm_cfunc_value(strm_cfunc f)
{
  return STRM_TAG_CFUNC | ((strm_value)f & STRM_VAL_MASK);
}

strm_value
strm_bool_value(int i)
{
  return STRM_TAG_BOOL | (!!i);
}

strm_value
strm_int_value(int32_t i)
{
  return STRM_TAG_INT | ((uint64_t)i & STRM_VAL_MASK);
}

strm_value
strm_flt_value(double f)
{
  union {
    double f;
    uint64_t i;
  } u;

  if (isnan(f)) {
    return STRM_TAG_NAN;
  }
  u.f = f;
  return u.i;
}

strm_value
strm_foreign_value(void* p)
{
  return STRM_TAG_FOREIGN | ((strm_value)p & STRM_VAL_MASK);
}

static void*
strm_ptr(strm_value v)
{
  return (void*)strm_value_val(v);
}

static enum strm_ptr_type
strm_ptr_type(void* p)
{
  struct object {
    STRM_PTR_HEADER;
  } *obj = p;

  return obj->type;
}

void*
strm_value_ptr(strm_value v, enum strm_ptr_type e)
{
  void *p;

  assert(strm_value_tag(v) == STRM_TAG_PTR);
  p = strm_ptr(v);
  assert(p && strm_ptr_type(p) == e);
  return p;
}

void*
strm_value_foreign(strm_value v)
{
  assert(strm_value_tag(v) == STRM_TAG_FOREIGN);
  return strm_ptr(v);
}

int
strm_value_bool(strm_value v)
{
  uint64_t i = strm_value_val(v);

  if (i == 0) {
    switch (strm_value_tag(v)) {
    case STRM_TAG_BOOL:         /* false */
    case STRM_TAG_PTR:          /* nil */
      break;
    default:
      assert(strm_value_tag(v) == STRM_TAG_BOOL);
      break;
    }
    return FALSE;
  }
  else {
    return TRUE;
  }
}

int
strm_int_p(strm_value v)
{
  return strm_value_tag(v) == STRM_TAG_INT;
}

static inline int32_t
strm_to_int(strm_value v)
{
  return (int32_t)(v & ~STRM_TAG_MASK);
}

int
strm_flt_p(strm_value v)
{
  /* confirm for nan/+inf/-inf */
  return v == STRM_TAG_NAN || (v & STRM_TAG_NAN) != STRM_TAG_NAN;
}

static inline double
strm_to_flt(strm_value v)
{
  union {
    double f;
    uint64_t i;
  } u;

  u.i = v;
  return u.f;
}

int32_t
strm_value_int(strm_value v)
{
  switch (strm_value_tag(v)) {
  case STRM_TAG_INT:
    return strm_to_int(v);
  default:
    if (strm_flt_p(v))
      return strm_to_flt(v);
    assert(strm_value_tag(v) == STRM_TAG_INT);
    break;
  }
  /* not reached */
  return 0;
}


double
strm_value_flt(strm_value v)
{
  if (strm_int_p(v)) {
    return (double)strm_to_int(v);
  }
  else if (strm_flt_p(v)) {
    return strm_to_flt(v);
  }
  else {
    assert(strm_flt_p(v));
  }
  /* not reached */
  return 0.0;
}

strm_cfunc
strm_value_cfunc(strm_value v)
{
  assert(strm_value_tag(v) == STRM_TAG_CFUNC);
  return (strm_cfunc)strm_value_val(v);
}

int
strm_num_p(strm_value v)
{
  if (strm_int_p(v) || strm_flt_p(v))
    return TRUE;
  else
    return FALSE;
}

int
strm_bool_p(strm_value v)
{
  return  (strm_value_tag(v) == STRM_TAG_BOOL) ? TRUE : FALSE;
}

int
strm_nil_p(strm_value v)
{
  if (strm_value_tag(v) != STRM_TAG_PTR)
    return FALSE;
  return strm_value_val(v) == 0;
}

int
strm_cfunc_p(strm_value v)
{
  return strm_value_tag(v) == STRM_TAG_CFUNC;
}

int
strm_ptr_tag_p(strm_value v, enum strm_ptr_type e)
{
  if (strm_value_tag(v) == STRM_TAG_PTR) {
    void *p = strm_ptr(v);
    return strm_ptr_type(p) == e;
  }
  return FALSE;
}

int
strm_value_eq(strm_value a, strm_value b)
{
  if (a == b) return TRUE;
  if (strm_value_tag(a) != strm_value_tag(b)) return FALSE;
  switch (strm_value_tag(a)) {
  case STRM_TAG_ARRAY:
  case STRM_TAG_STRUCT:
    return strm_ary_eq(a, b);
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    return strm_str_eq(a, b);
  case STRM_TAG_CFUNC:
    return (strm_cfunc)strm_value_val(a) == (strm_cfunc)strm_value_val(b);
  case STRM_TAG_PTR:
    return (void*)strm_value_val(a) == (void*)strm_value_val(b);
  default:
    return FALSE;
  }
}

static int
str_symbol_p(strm_string str)
{
  switch (strm_value_tag(str)) {
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
    return TRUE;
  case STRM_TAG_STRING_O:
    return FALSE;
  case STRM_TAG_STRING_F:
    return TRUE;
  default:
    return FALSE;
  }
}

static strm_int
str_dump_len(strm_string str)
{
  strm_int len = 2;             /* first and last quotes */
  const unsigned char *p = (unsigned char*)strm_str_ptr(str);
  const unsigned char *pend = p + strm_str_len(str);

  while (p < pend) {
    switch (*p) {
    case '\n': case '\r': case '\t': case '"':
      len += 2;
      break;
    default:
      if (isprint(*p) || (*p&0xff) > 0x7f) {
        len++;
      }
      else {
        len += 3;
      }
    }
    p++;
  }
  return len;
}

static strm_string
str_dump(strm_string str, strm_int len)
{
  char *buf = malloc(len);
  char *s = buf;
  char *p = (char*)strm_str_ptr(str);
  char *pend = p + strm_str_len(str);

  *s++ = '"';
  while (p<pend) {
    switch (*p) {
    case '\n':
      *s++ = '\\';
      *s++ = 'n';
      break;
    case '\r':
      *s++ = '\\';
      *s++ = 'r';
      break;
    case '\t':
      *s++ = '\\';
      *s++ = 't';
      break;
    case 033:
      *s++ = '\\';
      *s++ = 'e';
      break;
    case '\0':
      *s++ = '\\';
      *s++ = '0';
      break;
    case '"':
      *s++ = '\\';
      *s++ = '"';
      break;
    default:
      if (isprint((int)*p) || (*p&0xff) > 0x7f) {
        *s++ = (*p&0xff);
      }
      else {
        sprintf(s, "\\x%02x", (int)*p&0xff);
        s+=4;
      }
    }
    p++;
  }
  *s++ = '"';
  
  return strm_str_new(buf, len);
}

strm_string
strm_inspect(strm_value v)
{
  if (strm_string_p(v)) {
    strm_string str = strm_value_str(v);
    return str_dump(str, str_dump_len(str));
  }
  else if (strm_array_p(v)) {
    char *buf = malloc(32);
    strm_int i, bi = 0, capa = 32;
    strm_array a = strm_value_ary(v);

    for (i=0; i<strm_ary_len(a); i++) {
      strm_string str = strm_inspect(strm_ary_ptr(a)[i]);
      strm_string key = (strm_ary_headers(a) &&
                         strm_string_p(strm_ary_ptr(strm_ary_headers(a))[i])) ?
        strm_value_str(strm_ary_ptr(strm_ary_headers(a))[i]) : strm_str_null;
      strm_int slen = (key ? (strm_str_len(key)+1) : 0) + strm_str_len(str) + 3;

      if (bi+slen > capa) {
        capa *= 2;
        buf = realloc(buf, capa);
      }
      if (bi == 0) {
        buf[bi++] = '[';
      }
      else {
        buf[bi++] = ',';
        buf[bi++] = ' ';
      }
      if (key) {
        if (!str_symbol_p(key)) {
          key = str_dump(key, str_dump_len(key));
        }
        memcpy(buf+bi, strm_str_ptr(key), strm_str_len(key));
        bi += strm_str_len(key);
        buf[bi++] = ':';
      }
      memcpy(buf+bi, strm_str_ptr(str), strm_str_len(str));
      bi += strm_str_len(str);
    }
    buf[bi++] = ']';
    return strm_str_new(buf, bi);
  }
  else {
    return strm_to_str(v);
  }
}

strm_string
strm_to_str(strm_value v)
{
  char buf[32];
  int n;

  switch (strm_value_tag(v)) {
  case STRM_TAG_INT:
    n = sprintf(buf, "%d", strm_to_int(v));
    return strm_str_new(buf, n);
  case STRM_TAG_BOOL:
    n = sprintf(buf, strm_to_int(v) ? "true" : "false");
    return strm_str_new(buf, n);
  case STRM_TAG_CFUNC:
    n = sprintf(buf, "<cfunc:%p>", (void*)strm_value_cfunc(v));
    return strm_str_new(buf, n);
  case STRM_TAG_STRING_I:
  case STRM_TAG_STRING_6:
  case STRM_TAG_STRING_O:
  case STRM_TAG_STRING_F:
    return strm_value_str(v);
  case STRM_TAG_ARRAY:
  case STRM_TAG_STRUCT:
    return strm_inspect(v);
  case STRM_TAG_PTR:
    if (strm_value_val(v) == 0)
      return strm_str_new("nil", 3);
    else {
      void *p = strm_ptr(v);
      switch (strm_ptr_type(p)) {
      case STRM_PTR_TASK:
        n = sprintf(buf, "<task:%p>", p);
        break;
      case STRM_PTR_IO: {
        strm_io io = (strm_io)p;
        char *mode;

        switch (io->mode & 3) {
        case STRM_IO_READ:
          mode = "r"; break;
        case STRM_IO_WRITE:
          mode = "w"; break;
        case STRM_IO_READ|STRM_IO_WRITE:
          mode = "rw"; break;
        }
        n = sprintf(buf, "<io: fd=%d mode=%s>", io->fd, mode);
        break;
      }
      case STRM_PTR_LAMBDA:
        n = sprintf(buf, "<lambda:%p>", p);
        break;
      case STRM_PTR_AUX:
        n = sprintf(buf, "<obj:%p>", p);
        break;
      }
      return strm_str_new(buf, n);
      break;
    }
  default:
    if (strm_flt_p(v)) {
      n = sprintf(buf, "%g", strm_to_flt(v));
      return strm_str_new(buf, n);
    }
    n = sprintf(buf, "<%p>", (void*)strm_value_val(v));
    return strm_str_new(buf, n);
  }
  /* not reached */
  return strm_str_null;
}

strm_value
strm_nil_value(void)
{
  return STRM_TAG_PTR | 0;
}

struct strm_misc {
  STRM_MISC_HEADER;
};

strm_state*
strm_value_ns(strm_value val)
{
  if (strm_array_p(val))
    return strm_ary_ns(val);
  if (strm_value_tag(val) == STRM_TAG_PTR) {
    struct strm_misc* p = strm_ptr(val);

    if (strm_ptr_type(p) == STRM_PTR_AUX) {
      return p->ns;
    }
  }
  return NULL;
}
