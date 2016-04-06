#include "strm.h"
#include <time.h>
#include <sys/time.h>
#include <math.h>

#ifndef timeradd
# define timeradd(a, b, res)                                               \
  do {                                                                     \
    (res)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (res)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    while ((res)->tv_usec >= 1000000)                                      \
      {                                                                    \
        ++(res)->tv_sec;                                                   \
        (res)->tv_usec -= 1000000;                                         \
      }                                                                    \
  } while (0)
# define timersub(a, b, res)                                               \
  do {                                                                     \
    (res)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (res)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    while ((res)->tv_usec < 0) {                                           \
      --(res)->tv_sec;                                                     \
      (res)->tv_usec += 1000000;                                           \
    }                                                                      \
  } while (0)
#endif

struct strm_time {
  STRM_AUX_HEADER;
  struct timeval tv;
  int utc_offset;
};

static strm_state* time_ns;

void
num_to_timeval(double d, struct timeval* tv)
{
  double sec = floor(d);
  tv->tv_sec = sec;
  tv->tv_usec = (d-sec)*1000000;
}

double
timeval_to_num(struct timeval* tv)
{
  double d = tv->tv_sec;

  return d+(tv->tv_usec)/1000000.0;
}

enum time_tz {
  TZ_UTC = 0,
  TZ_NONE = 2000,               /* no time, no timezone */
  TZ_FAIL = 4000,               /* broken timezone */
};

static int
time_localoffset()
{
  static int localoffset = 1;

  if (localoffset == 1) {
    time_t now;
    struct tm gm;
    double d;

    now = time(NULL);
    gmtime_r(&now, &gm);
    d = difftime(now, mktime(&gm));
    localoffset = d / 60;
  }
  return localoffset;
}

static int
time_alloc(struct timeval* tv, int utc_offset, strm_value* ret)
{
  struct strm_time* t = malloc(sizeof(struct strm_time));

  if (!t) return STRM_NG;
  t->type = STRM_PTR_AUX;
  t->ns = time_ns;
  while (tv->tv_usec < 0) {
    tv->tv_sec--;
    tv->tv_usec += 1000000;
  }
  while (tv->tv_usec >= 1000000) {
    tv->tv_sec++;
    tv->tv_usec -= 1000000;
  }
  memcpy(&t->tv, tv, sizeof(struct timeval));
  t->utc_offset = utc_offset;
  *ret = strm_ptr_value(t);
  return STRM_OK;
}

static int
scan_digit(const char c)
{
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  else {
    return TZ_FAIL;
  }
}

static int
scan_num(const char** sp, const char* send)
{
  const char* s = *sp;
  int n = 0;

  while (s < send) {
    int i = scan_digit(*s);
    if (i>9) {
      if (s == *sp) return -1;
      *sp = s;
      return n;
    }
    s++;
    n = 10*n + i;
  }
  *sp = s;
  return n;
}

static int
parse_tz(const char* s, strm_int len)
{
  int h, m;
  char c;
  const char* send = s+len;

  switch (s[0]) {
  case 'Z':
    return 0;
  case '+':
  case '-':
    c = *s++;
    h = scan_num(&s, send);
    if (h < 0) return TZ_FAIL;
    if (*s == ':') {
      s++;
      m = scan_num(&s, send);
    }
    else if (h >= 100) {
      int i = h;
      h = i / 100;
      m = i % 100;
    }
    else {
      m = 0;
    }
    if (h > 12) return TZ_FAIL;
    if (m > 59) return TZ_FAIL;
    return (h*60+m) * (c == '-' ? -1 : 1);
  default:
    return TZ_FAIL;
  }
}

static int
time_now(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct timeval tv;
  int utc_offset;

  switch (argc) {
  case 0:
    utc_offset = time_localoffset(); 
   break;
  case 1:                       /* timezone */
    {
      strm_string str = strm_value_str(args[0]);
      utc_offset = parse_tz(strm_str_ptr(str), strm_str_len(str));
      if (utc_offset == TZ_FAIL) {
        strm_raise(strm, "wrong timezeone");
        return STRM_NG;
      }
    }
    break;
  default:
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  gettimeofday(&tv, NULL);
  return time_alloc(&tv, utc_offset, ret);
}

static struct strm_time*
get_time(strm_value val)
{
  struct strm_time* t = (struct strm_time*)strm_value_ptr(val, STRM_PTR_AUX);
  if (t->ns != time_ns) return NULL;
  return t;
}

static int
time_plus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_time* t1;
  struct timeval tv, tv2;

  if (argc != 2) {
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  t1 = get_time(args[0]);
  if (!strm_num_p(args[1])) {
    strm_raise(strm, "number required");
    return STRM_NG;
  }
  num_to_timeval(strm_value_flt(args[1]), &tv);
  timeradd(&t1->tv, &tv, &tv2);
  return time_alloc(&tv2, t1->utc_offset, ret);
}

static int
time_minus(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_time *t1;
  struct strm_time *t2;
  struct timeval tv;
  double d;

  if (argc != 2) {
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  if (strm_num_p(args[1])) {
    d = strm_value_flt(args[1]);
    args[1] = strm_flt_value(-d);
    return time_plus(strm, argc, args, ret);
  }
  t1 = get_time(args[0]);
  t2 = get_time(args[1]);
  timersub(&t1->tv, &t2->tv, &tv);
  d = timeval_to_num(&tv);
  *ret = strm_flt_value(d);
  return STRM_OK;
}

static void
get_tm(time_t t, int utc_offset, struct tm* tm)
{
  t += utc_offset * 60;
  gmtime_r(&t, tm);
}

static int
time_str(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_time *t;
  struct tm tm;
  int utc_offset;
  char buf[256];
  char *p;
  size_t n;

  switch (argc) {
  case 2:                       /* timezone */
    t = get_time(args[0]);
    {
      strm_string str = strm_value_str(args[1]);
      utc_offset = parse_tz(strm_str_ptr(str), strm_str_len(str));
      if (utc_offset == TZ_FAIL) {
        strm_raise(strm, "wrong timezeone");
        return STRM_NG;
      }
    }
    break;
  case 1:
    t = get_time(args[0]);
    utc_offset = t->utc_offset;
    break;
  default:
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  get_tm(t->tv.tv_sec, utc_offset, &tm);
  n = strftime(buf, sizeof(buf), "%Y.%m.%dT%H:%M:%S", &tm);
  p = buf+n;
  if (t->tv.tv_usec != 0) {
    char sbuf[20];
    double d = t->tv.tv_usec / 1000000.0;
    char *t;
    size_t len;

    snprintf(sbuf, sizeof(sbuf), "%.3f", d);
    t = sbuf+1;
    len = strlen(t);
    strncpy(p, t, len);
    p += len;
  }
  if (utc_offset == 0) {
    p[0] = 'Z';
    p[1] = '\0';
  }
  else {
    int off = utc_offset;
    char sign = off > 0 ? '+' : '-';

    if (off < 0) off = -off;
    snprintf(p, sizeof(buf)-(p-buf), "%c%02d:%02d", sign, off/60, off%60);
  }
  *ret = strm_str_new(buf, strlen(buf));
  return STRM_OK;
}

static int
time_num(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_time *t;

  if (argc != 1) {
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  t = get_time(args[0]);
  if (t->tv.tv_usec == 0) {
    *ret = strm_int_value(t->tv.tv_sec);
  }
  else {
    *ret = strm_flt_value(timeval_to_num(&t->tv));
  }
  return STRM_OK;
}

void
strm_time_init(strm_state* state)
{
  strm_var_def(state, "now", strm_cfunc_value(time_now));

  time_ns = strm_ns_new(NULL);
  strm_var_def(time_ns, "+", strm_cfunc_value(time_plus));
  strm_var_def(time_ns, "-", strm_cfunc_value(time_minus));
  strm_var_def(time_ns, "string", strm_cfunc_value(time_str));
  strm_var_def(time_ns, "number", strm_cfunc_value(time_num));
}
