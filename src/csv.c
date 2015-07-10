/* copied and modified from https://github.com/semitrivial/csv_parser */
/*                                                        MIT license */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "strm.h"
#include "node.h"

static int
count_fields(const strm_string* line)
{
  const char *ptr = line->ptr;
  const char *pend = ptr + line->len;
  int cnt, fQuote = 0;

  for (cnt = 1; ptr<pend; ptr++) {
    if (fQuote) {
      if (ptr[0] == '\"') {
        if (ptr[1] == '\"') {
          ptr++;
          continue;
        }
        fQuote = 0;
      }
      continue;
    }

    switch(*ptr) {
    case '\"':
      fQuote = 1;
      continue;
    case ',':
      cnt++;
      continue;
    default:
      continue;
    }
  }

  if (fQuote)
    return -1;

  return cnt;
}

enum csv_type {
  STRING_TYPE,
  NUMBER_TYPE,
};

static strm_value
csv_value(const char* p, size_t len)
{
  const char *s = p;
  const char *send = s+len;
  long i=0;
  double f, pow = 1;
  int type = 0;                 /* 0: string, 1: int, 2: float */

  /* skip preceding white spaces */
  while (isspace(*s)) s++;

  /* check if numbers */
  while (s<send) {
    switch (*s) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (type == STRING_TYPE) type = 1;
      i = i*10 + (*s - '0');
      pow *= 10;
      break;
    case '.':
      type = 2;
      f = i;
      i = 0;
      pow = 1;
      break;
    default:
      return strm_str_value(p, len);
    }
    s++;
  }

  switch (type) {
  case 1:                       /* int */
    return strm_int_value(i);
  case 2:                       /* float */
    f += i / pow;
    return strm_flt_value(f);
  default:
    return strm_str_value(p, len);
  }
  /* not reached */
}

struct csv_data {
  strm_array *headers;
  enum csv_type *types;
  strm_string *prev;
  int n;
};

enum csv_type
csv_type(strm_value v)
{
  if (strm_int_p(v)) return NUMBER_TYPE;
  else if (strm_flt_p(v)) return NUMBER_TYPE;
  else return STRING_TYPE;
}

static void
csv_accept(strm_task* task, strm_value data)
{
  strm_array *ary;
  strm_string *line = strm_value_str(data);
  strm_value *bp;
  char *tmp, *tptr;
  const char *ptr;
  const char *pend;
  int fieldcnt, len;
  int in_quote = 0, quoted = 0, all_str = 1;;
  struct csv_data *cd = task->data;

  if (cd->prev) {
    strm_string *str = strm_str_new(NULL, cd->prev->len+line->len+1);

    tmp = (char*)str->ptr;
    memcpy(tmp, cd->prev->ptr, cd->prev->len);
    *(tmp+cd->prev->len) = '\n';
    memcpy(tmp+cd->prev->len+1, line->ptr, line->len);
    line = str;
    cd->prev = NULL;
  }
  fieldcnt = count_fields(line);
  if (fieldcnt == -1) {
    cd->prev = line;
    return;
  }
  if (cd->n > 0 && fieldcnt != cd->n)
    return;

  ptr = line->ptr;
  pend = ptr + line->len;
  ary = strm_ary_new(NULL, fieldcnt);
  if (!ary) return;
  bp = (strm_value*)ary->ptr;

  len = line->len;
  tmp = malloc(len+1);
  if (!tmp) return;
  *tmp='\0';

  ptr=line->ptr;
  tptr=tmp;
  for (;ptr<pend; ptr++) {
    if (in_quote) {
      if (*ptr == '\"') {
        if (ptr[1] == '\"') {
          *tptr++ = '\"';
          ptr++;
          continue;
        }
        in_quote = 0;
      }
      else
        *tptr++ = *ptr;
      continue;
    }

    switch(*ptr) {
    case '\"':
      in_quote = 1;
      quoted = 1;
      continue;
    case ',':
      if (quoted) {
        *bp = strm_str_value(tmp, tptr-tmp);
      }
      else {
        *bp = csv_value(tmp, tptr-tmp);
      }
      if (!strm_str_p(*bp)) all_str = 0;
      bp++;
      tptr = tmp;
      quoted = 0;
      break;

    default:
      *tptr++ = *ptr;
      continue;
    }
  }
  /* trim newline at the end */
  if (tptr > tmp && tptr[-1] == '\n') {
    tptr--;
  }
  /* trim carriage return at the end */
  if (tptr > tmp && tptr[-1] == '\r') {
    tptr--;
  }
  *bp = csv_value(tmp, tptr-tmp);
  if (!strm_str_p(*bp)) all_str = 0;

  free(tmp);

  /* check headers */
  if (!cd->headers) {
    if (all_str) { /* intern header strings */
      strm_value *p = (strm_value*)ary->ptr;
      int i;

      for (i=0; i<ary->len; i++) {
        strm_string *str = strm_value_str(p[i]);

        p[i] = strm_ptr_value(strm_str_intern(str->ptr, str->len));
      }
      cd->headers = ary;
      ary = NULL;
    }
    cd->n = fieldcnt;
  }
  if (ary) {
    int i;

    /* set headers if any */
    if (cd->headers)
      ary->headers = cd->headers;
    if (!cd->types) {
      /* initialize types (determined by first line) */
      cd->types = malloc(sizeof(enum csv_type)*fieldcnt);
      if (!cd->types) return;
      for (i=0; i<fieldcnt; i++) {
        cd->types[i] = csv_type(ary->ptr[i]);
      }
    }
    else {
      /* type check */
      for (i=0; i<fieldcnt; i++) {
        if (cd->types[i] != csv_type(ary->ptr[i])) {
          if (cd->types[i] == STRING_TYPE) {
            /* convert value to string */
            ((strm_value*)ary->ptr)[i] = strm_ptr_value(strm_to_str(ary->ptr[i]));
          }
          else {
            /* type mismatch (error); skip this line */
            return;
          }
        }
      }
    }
    strm_emit(task, strm_ptr_value(ary), NULL);
  }
}

static int
csv(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  strm_task *task;
  struct csv_data *cd = malloc(sizeof(struct csv_data));

  if (!cd) return STRM_NG;
  cd->headers = NULL;
  cd->types = NULL;
  cd->prev = NULL;
  cd->n = 0;

  task = strm_task_new(strm_task_filt, csv_accept, NULL, (void*)cd);
  *ret = strm_task_value(task);
  return STRM_OK;
}

void
strm_csv_init(strm_state* state)
{
  strm_var_def("csv", strm_cfunc_value(csv));
}
