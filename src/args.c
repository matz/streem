#include "strm.h"
#include <stdarg.h>

/*
  parse arguments from argc/argv.

  strm_parse_args(strm, argc, argv, format, ...)

  returns STRM_OK / STRM_NG

  format specifiers:

    string  streem type    C type                 note
    ----------------------------------------------------------------------------------------------
    v:      value          [strm_value]
    S:      string         [strm_string]
    s:      string         [char*,strm_int]        takes two arguments
    A:      array          [strm_array]
    a:      array          [strm_value*,strm_int]  takes two arguments
    N:      number         [strm_value]            receive either integer or float
    f:      number         [double]
    i:      number         [strm_int]              truncate float number to integer
    b:      boolean        [strm_int]
    |:      optional                               next argument of '|' and later are optional
    ?:      optional given [strm_int]              true if preceding (optional) argument is given
 */
strm_int
strm_parse_args(strm_stream* strm, int argc, strm_value* argv, const char* format, ...)
{
  char c;
  int i = 0;
  va_list ap;
  int arg_i = 0;
  strm_int opt = FALSE;
  strm_int given = TRUE;

  va_start(ap, format);
  while ((c = *format++)) {
    switch (c) {
    case '|': case '*': case '&': case '?':
      break;
    default:
      if (argc <= i) {
        if (opt) {
          given = FALSE;
        }
        else {
          strm_raise(strm, "wrong number of arguments");
          return STRM_NG;
        }
      }
      break;
    }

    switch (c) {
    case 'v':
      {
        strm_value* p;

        p = va_arg(ap, strm_value*);
        if (i < argc) {
          *p = argv[arg_i++];
          i++;
        }
      }
      break;
    case 'N':
      {
        strm_value* p;

        p = va_arg(ap, strm_value*);
        if (i < argc) {
          strm_value nn = argv[arg_i++];
          if (!strm_number_p(nn)) {
            strm_raise(strm, "number required");
          }
          *p = nn;
          i++;
        }
      }
      break;
    case 'S':
      {
        strm_value ss;
        strm_string* p;

        p = va_arg(ap, strm_string*);
        if (i < argc) {
          ss = argv[arg_i++];
          i++;
          if (!strm_string_p(ss)) {
            strm_raise(strm, "string required");
            return STRM_NG;
          }
          *p = strm_value_str(ss);
        }
      }
      break;
    case 's':
      {
        strm_value ss;
        const char** ps;
        strm_int* pl;

        ps = va_arg(ap, const char**);
        pl = va_arg(ap, strm_int*);
        if (i < argc) {
          ss = argv[arg_i];
          if (!strm_string_p(ss)) {
            strm_raise(strm, "string required");
            return STRM_NG;
          }
          /* note: strm_str_ptr() from local variable should not
             be exported from a function use external address instead */
          *ps = strm_str_ptr(argv[arg_i]);
          *pl = strm_str_len(ss);
          i++;
          arg_i++;
        }
      }
      break;
    case 'A':
      {
        strm_array* p;
        strm_value v;

        p = va_arg(ap, strm_array*);
        if (i < argc) {
          v = argv[arg_i++];
          i++;
          if (!strm_array_p(v)) {
            strm_raise(strm, "array required");
            return STRM_NG;
          }
          *p = strm_value_ary(v);
        }
      }
      break;
    case 'a':
      {
        strm_array aa;
        strm_value** pb;
        strm_int* pl;

        pb = va_arg(ap, strm_value**);
        pl = va_arg(ap, strm_int*);
        if (i < argc) {
          aa = argv[arg_i++];
          i++;
          if (strm_nil_p(aa)) {
            *pb = NULL;
            *pl = 0;
            break;
          }
          if (!strm_array_p(aa)) {
            strm_raise(strm, "array required");
            return STRM_NG;
          }
          *pb = strm_ary_ptr(aa);
          *pl = strm_ary_len(aa);
        }
      }
      break;
    case 'f':
      {
        double* p;
        strm_value ff;

        p = va_arg(ap, double*);
        if (i < argc) {
          ff = argv[arg_i++];
          i++;
          if (!strm_number_p(ff)) {
            strm_raise(strm, "number required");
            return STRM_NG;
          }
          *p = strm_value_float(ff);
        }
      }
      break;
    case 'i':
      {
        strm_int* p;
        strm_value ff;

        p = va_arg(ap, strm_int*);
        if (i < argc) {
          ff = argv[arg_i++];
          i++;
          if (!strm_number_p(ff)) {
            strm_raise(strm, "number required");
            return STRM_NG;
          }
          *p = strm_value_int(ff);
        }
      }
      break;
    case 'b':
      {
        strm_int* p;
        strm_int bb;

        p = va_arg(ap, strm_int*);
        if (i < argc) {
          bb = argv[arg_i++];
          i++;
          if (!strm_bool_p(bb)) {
            strm_raise(strm, "boolean required");
            return STRM_NG;
          }
          *p = strm_value_bool(bb);
        }
      }
      break;
    case '|':
      opt = TRUE;
      break;
    case '?':
      {
        strm_int* p;

        p = va_arg(ap, strm_int*);
        *p = given;
      }
      break;
    default:
      strm_raise(strm, "invalid argument specifier");
      break;
    }
  }

#undef ARGV

  va_end(ap);
  if (!c && argc > i) {
    strm_raise(strm, "wrong number of arguments");
    return STRM_NG;
  }
  return STRM_OK;
}
