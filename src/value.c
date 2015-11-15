#include <ctype.h>
#include "strm.h"
#include <assert.h>

strm_value
strm_ptr_value(void* p)
{
  strm_value v;

  v.type = STRM_VALUE_PTR;
  v.val.p = p;
  return v;
}

strm_value
strm_cfunc_value(void* p)
{
  strm_value v;

  v.type = STRM_VALUE_CFUNC;
  v.val.p = p;
  return v;
}

strm_value
strm_task_value(void* p)
{
  strm_value v;

  v.type = STRM_VALUE_TASK;
  v.val.p = p;
  return v;
}

strm_value
strm_bool_value(int i)
{
  strm_value v;

  v.type = STRM_VALUE_BOOL;
  v.val.i = i ? 1 : 0;
  return v;
}

strm_value
strm_int_value(long i)
{
  strm_value v;

  v.type = STRM_VALUE_INT;
  v.val.i = i;
  return v;
}

strm_value
strm_flt_value(double f)
{
  strm_value v;

  v.type = STRM_VALUE_FLT;
  v.val.f = f;
  return v;
}

strm_value
strm_blk_value(void* blk)
{
  strm_value v;

  v.type = STRM_VALUE_BLK;
  v.val.p = blk;
  return v;
}

void*
strm_value_ptr(strm_value v)
{
  assert(v.type == STRM_VALUE_PTR);
  return v.val.p;
}

void*
strm_value_obj(strm_value v, enum strm_obj_type t)
{
  struct strm_object *p;

  assert(v.type == STRM_VALUE_PTR);
  p = v.val.p;
  assert(p->type == t);
  return v.val.p;
}

int
strm_value_bool(strm_value v)
{
  if (v.type == STRM_VALUE_BOOL) {
    return v.val.i ? TRUE : FALSE;
  }
  return TRUE;
}

long
strm_value_int(strm_value v)
{
  switch (v.type) {
  case STRM_VALUE_FLT:
    return (long)v.val.f;
  case STRM_VALUE_INT:
    return v.val.i;
  default:
    assert(v.type == STRM_VALUE_INT);
    break;
  }
  /* not reached */
  return 0;
}

double
strm_value_flt(strm_value v)
{
  switch (v.type) {
  case STRM_VALUE_FLT:
    return v.val.f;
  case STRM_VALUE_INT:
    return (double)v.val.i;
  default:
    assert(v.type == STRM_VALUE_FLT);
    break;
  }
  /* not reached */
  return 0.0;
}

strm_task*
strm_value_task(strm_value v)
{
  assert(v.type == STRM_VALUE_TASK);
  return (strm_task*)v.val.p;
}

int
strm_num_p(strm_value v)
{
  switch (v.type) {
  case STRM_VALUE_FLT:
  case STRM_VALUE_INT:
    return TRUE;
  default:
    return FALSE;
  }
}

int
strm_nil_p(strm_value v)
{
  return v.type == STRM_VALUE_PTR && v.val.p == NULL;
}

int
strm_int_p(strm_value v)
{
  return v.type == STRM_VALUE_INT;
}

int
strm_flt_p(strm_value v)
{
  return v.type == STRM_VALUE_FLT;
}

int
strm_cfunc_p(strm_value v)
{
  return v.type == STRM_VALUE_CFUNC;
}

int
strm_lambda_p(strm_value v)
{
  return v.type == STRM_VALUE_PTR && v.val.p &&
         ((struct strm_object*)v.val.p)->type == STRM_OBJ_LAMBDA;
}

int
strm_array_p(strm_value v)
{
  return v.type == STRM_VALUE_PTR && v.val.p &&
         ((struct strm_object*)v.val.p)->type == STRM_OBJ_ARRAY;
}

int
strm_task_p(strm_value v)
{
  return v.type == STRM_VALUE_TASK;
}

int
strm_io_p(strm_value v)
{
  return v.type == STRM_VALUE_PTR && v.val.p &&
         ((struct strm_object*)v.val.p)->type == STRM_OBJ_IO;
}

int
strm_ptr_eq(struct strm_object* a, struct strm_object* b)
{
  if (a == b) return TRUE;
  if (a == NULL) return FALSE;
  if (a->type != b->type) return FALSE;
  switch (a->type) {
  case STRM_OBJ_ARRAY:
    return strm_ary_eq((struct strm_array*)a, (struct strm_array*)b);
  case STRM_OBJ_STRING:
    return strm_str_eq((struct strm_string*)a, (struct strm_string*)b);
  default:
    return FALSE;
  }
}

int
strm_value_eq(strm_value a, strm_value b)
{
  if (a.type != b.type) return FALSE;

  switch (a.type) {
  case STRM_VALUE_BOOL:
  case STRM_VALUE_INT:
    return a.val.i == b.val.i;
  case STRM_VALUE_FLT:
    return a.val.f == b.val.f;
  case STRM_VALUE_PTR:
    return strm_ptr_eq(a.val.p, b.val.p);
  default:
    return FALSE;
  }
}

static int
str_symbol_p(strm_string str)
{
  const char *p = str->ptr;
  const char *pend = p + str->len;

  if (!isalpha((int)*p)) return 0;
  p++;
  while (p<pend) {
    if (!isalnum((int)*p)) return 0;
  }
  return 1;
}

static size_t
str_dump_len(strm_string str)
{
  size_t len = 2;               /* first and last quotes */
  const unsigned char *p = (unsigned char*)str->ptr;
  const unsigned char *pend = p + str->len;

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
str_dump(strm_string str, size_t len)
{
  char *buf = malloc(len);
  char *s = buf;
  char *p = (char*)str->ptr;
  char *pend = p + str->len;

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
  if (v.type == STRM_VALUE_PTR) {
    if (v.val.p) {
      switch (((struct strm_object*)v.val.p)->type) {
      case STRM_OBJ_STRING:
        {
          strm_string str = (strm_string)v.val.p;
          return str_dump(str, str_dump_len(str));
        }
      case STRM_OBJ_ARRAY:
        {
          char *buf = malloc(32);
          size_t i, bi = 0, capa = 32;
          strm_array *a = (strm_array*)v.val.p;

          for (i=0; i<a->len; i++) {
            strm_string str = strm_inspect(a->ptr[i]);
            strm_string key = (a->headers && strm_str_p(a->headers->ptr[i])) ?
              strm_value_ptr(a->headers->ptr[i]) : NULL;
            size_t slen = (key ? (key->len+1) : 0) + str->len + 3;

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
              memcpy(buf+bi, key->ptr, key->len);
              bi += key->len;
              buf[bi++] = ':';
            }
            memcpy(buf+bi, str->ptr, str->len);
            bi += str->len;
          }
          buf[bi++] = ']';
          return strm_str_new(buf, bi);
        }
      case STRM_OBJ_IO:
        {
          char buf[32];
          strm_io* io = (strm_io*)v.val.p;
          int n;
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
          return strm_str_new(buf, n);
        }
      default:
        {
          char buf[12];
          int n = sprintf(buf, "<%p>", v.val.p);
          return strm_str_new(buf, n);
        }
      }
    }
  }
  return strm_to_str(v);
}

strm_string
strm_to_str(strm_value v)
{
  char buf[32];
  int n;

  switch (v.type) {
  case STRM_VALUE_FLT:
    n = sprintf(buf, "%g", v.val.f);
    return strm_str_new(buf, n);
  case STRM_VALUE_INT:
    n = sprintf(buf, "%ld", v.val.i);
    return strm_str_new(buf, n);
  case STRM_VALUE_BOOL:
    n = sprintf(buf, v.val.i ? "true" : "false");
    return strm_str_new(buf, n);
  case STRM_VALUE_CFUNC:
    n = sprintf(buf, "<cfunc:%p>", v.val.p);
    return strm_str_new(buf, n);
  case STRM_VALUE_BLK:
    n = sprintf(buf, "<blk:%p>", v.val.p);
    return strm_str_new(buf, n);
  case STRM_VALUE_TASK:
    n = sprintf(buf, "<task:%p>", v.val.p);
    return strm_str_new(buf, n);
  case STRM_VALUE_PTR:
    if (v.val.p == NULL)
      return strm_str_new("nil", 3);
    switch (((struct strm_object*)v.val.p)->type) {
    case STRM_OBJ_STRING:
      return (strm_string)v.val.p;
    case STRM_OBJ_ARRAY:
    default:
      return strm_inspect(v);
    }
  default:
    n = sprintf(buf, "<%p>", v.val.p);
    return strm_str_new(buf, n);
  }
  /* not reached */
  return NULL;
}
