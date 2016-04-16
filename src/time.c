#define _XOPEN_SOURCE 700
#include "strm.h"
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
char* strptime(const char *buf, char *fmt, struct tm *tm);
#endif

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

int
strm_time_p(strm_value val)
{
  if (strm_value_tag(val) != STRM_TAG_PTR)
    return FALSE;
  else {
    struct strm_misc {
      STRM_AUX_HEADER;
    }* p = strm_value_vptr(val);

    if (!p) return FALSE;
    if (p->type != STRM_PTR_AUX)
      return FALSE;
    if (p->ns != time_ns)
      return FALSE;
  }
  return TRUE;
}

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
  TZ_NONE = 50000,              /* no time, no timezone */
  TZ_FAIL = 60000,              /* broken timezone */
};

static int
time_localoffset(int force)
{
  static int localoffset = 1;

  if (force || localoffset == 1) {
    time_t now;
    struct tm gm;

    now = time(NULL);
    gmtime_r(&now, &gm);
    localoffset = difftime(now, mktime(&gm));
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
  t->tv = *tv;
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
    return (h*60+m) * (c == '-' ? -1 : 1) * 60;
  default:
    return TZ_FAIL;
  }
}

int
strm_time_parse_time(const char* p, strm_int len, long* sec, long* usec, int* offset)
{
  const char* s = p;
  const char* t;
  const char* t2;
  const char* tend;
  struct tm tm = {0};
  int localoffset = time_localoffset(1);
  time_t tt;

  if (s[len] != '\0') {
    char* pp = malloc(len+1);
    memcpy(pp, p, len);
    pp[len] = '\0';
    s = (const char*)pp;
  }
  tend = s + len;
  *usec = 0;
  t = strptime(s, "%Y.%m.%d", &tm);   /* Streem time literal */
  if (t == NULL) {
    t = strptime(s, "%Y-%m-%d", &tm); /* ISO8601 */
  }
  if (t == NULL) {
    t = strptime(s, "%Y/%m/%d", &tm);
  }
  if (t == NULL) {
    t = strptime(s, "%b %d %Y", &tm);
    /* check year and hour confusion (Apr 14 20:00 is not year 20 AD) */
    if (t && t[0] == ':') {
      t = NULL;
    }
  }
  if (t == NULL) {
    struct tm tm2;
    tt = time(NULL);
    localtime_r(&tt, &tm2);
    tm.tm_year = tm2.tm_year;
    t = strptime(s, "%b %d", &tm);
  }
  if (t == NULL) goto bad;
  if (t == tend) {
    tt = mktime(&tm);
    tt += localoffset;
    *sec = tt;
    *usec = 0;
    *offset = TZ_NONE;
    goto good;
  }

  switch (*t++) {
  case 'T':
    break;
  case ' ':
    while (*t == ' ')
      t++;
    break;
  default:
    goto bad;
  }

  t2 = strptime(t, "%H:%M:%S", &tm);
  if (t2 == NULL) {
    t2 = strptime(t, "%H:%M", &tm);
    if (t2 == NULL)
      goto bad;
  }
  t = t2;
  tt = mktime(&tm);
  if (t[0] == '.') {          /* frac */
    t++;
    long frac = scan_num(&t, tend);
    if (frac < 0) goto bad;
    if (frac > 0) {
      double d = ceil(log10((double)frac));
      d = ((double)frac / pow(10.0, d));
      *usec = d * 1000000;
    }
  }
  if (t[0] == 'Z') {          /* UTC zone */
    tt += localoffset;
    *offset = 0;
  }
  else if (t == tend) {
    *offset = localoffset;
  }
  else {
    int n;

    switch (t[0]) {           /* offset zone */
    case '+':
    case '-':
      n = parse_tz(t, (strm_int)(tend-t));
      if (n == TZ_FAIL) goto bad;
      tt += localoffset;
      tt -= n;
      *offset = n;
      break;
    default:
      goto bad;
    }
  }
  *sec = tt;
 good:
  if (s != p) free((char*)s);
  return 0;
 bad:
  if (s != p) free((char*)s);
  return -1;
}

strm_value
strm_time_new(long sec, long usec, int offset)
{
  struct timeval tv;
  strm_value v;

  tv.tv_sec = sec;
  tv.tv_usec = usec;
  if (time_alloc(&tv, offset, &v) == STRM_NG) {
    return strm_nil_value();
  }
  return v;
}

static int
time_time(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct timeval tv = {0};
  struct tm tm = {0};
  time_t t;
  int utc_offset = 0;

  switch (argc) {
  case 1:                       /* string */
    {
      strm_string str = strm_value_str(args[0]);
      long sec, usec;

      if (strm_time_parse_time(strm_str_ptr(str), strm_str_len(str),
                               &sec, &usec, &utc_offset) < 0) {
        strm_raise(strm, "bad time format");
        return STRM_NG;
      }
      tv.tv_sec = sec;
      tv.tv_usec = usec;
      return time_alloc(&tv, utc_offset, ret);
    }
    break;
  case 3:                       /* date (YYYY,MM,DD) */
    tm.tm_year = strm_value_int(args[0]);
    tm.tm_mon = strm_value_int(args[1])-1;
    tm.tm_mday = strm_value_int(args[2]);
    tv.tv_sec = mktime(&tm);
    tv.tv_sec += time_localoffset(1);
    utc_offset = TZ_NONE;
    return time_alloc(&tv, utc_offset, ret);
  case 8:                       /* date (YYYY,MM,DD,hh,mm,ss,usec,zone) */
    {
      strm_string str = strm_value_str(args[7]);
      utc_offset = parse_tz(strm_str_ptr(str), strm_str_len(str));
      if (utc_offset == TZ_FAIL) {
        strm_raise(strm, "wrong timezeone");
        return STRM_NG;
      }
    }
  case 7:                       /* date (YYYY,MM,DD,hh,mm,ss,usec) */
    tv.tv_usec = strm_value_int(args[6]);
  case 6:                       /* date (YYYY,MM,DD,hh,mm,ss) */
    tm.tm_sec = strm_value_int(args[5]);
  case 5:                       /* date (YYYY,MM,DD,hh,mm) */
    tm.tm_min = strm_value_int(args[4]);
  case 4:                       /* date (YYYY,MM,DD,hh) */
    tm.tm_year = strm_value_int(args[0]);
    tm.tm_mon = strm_value_int(args[1]);
    tm.tm_mday = strm_value_int(args[2]);
    tm.tm_hour = strm_value_int(args[3]);
    t = mktime(&tm);
    tv.tv_sec = t;
    if (argc == 8) {
      tv.tv_sec += time_localoffset(1);
      tv.tv_sec -= utc_offset;
    }
    else {
      utc_offset = time_localoffset(0);
    }
    return time_alloc(&tv, utc_offset, ret);
  default:
    strm_raise(strm, "wrong # of arguments");
    return STRM_NG;
  }
  gettimeofday(&tv, NULL);
  return time_alloc(&tv, utc_offset, ret);
}

static int
time_now(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct timeval tv;
  int utc_offset;

  switch (argc) {
  case 0:
    utc_offset = time_localoffset(0);
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
  t += utc_offset;
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
  if (utc_offset == TZ_NONE) {
    get_tm(t->tv.tv_sec, 0, &tm);
    if (tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec == 0) {
      n = strftime(buf, sizeof(buf), "%Y.%m.%d", &tm);
      *ret = strm_str_new(buf, n);
      return STRM_OK;
    }
    utc_offset = 0;
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
    int off = utc_offset / 60;
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

#define time_func(attr, value) \
static int \
attr(strm_stream* strm, int argc, strm_value* args, strm_value* ret)\
{\
  struct strm_time *t;\
  struct tm tm;\
\
  t = get_time(args[0]);\
  get_tm(t->tv.tv_sec, t->utc_offset, &tm);\
  *ret = strm_int_value(tm.value);\
  return STRM_OK;\
}

time_func(time_year, tm_year+1900);
time_func(time_month, tm_mon+1);
time_func(time_day, tm_mday);
time_func(time_hour, tm_hour);
time_func(time_min, tm_min);
time_func(time_sec, tm_sec);

static int
time_nanosec(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_time *t;

  t = get_time(args[0]);
  *ret = strm_int_value(t->tv.tv_usec*1000);
  return STRM_OK;
}

void
strm_time_init(strm_state* state)
{
  strm_var_def(state, "now", strm_cfunc_value(time_now));
  strm_var_def(state, "time", strm_cfunc_value(time_time));

  time_ns = strm_ns_new(NULL);
  strm_var_def(time_ns, "+", strm_cfunc_value(time_plus));
  strm_var_def(time_ns, "-", strm_cfunc_value(time_minus));
  strm_var_def(time_ns, "string", strm_cfunc_value(time_str));
  strm_var_def(time_ns, "number", strm_cfunc_value(time_num));
  strm_var_def(time_ns, "year", strm_cfunc_value(time_year));
  strm_var_def(time_ns, "month", strm_cfunc_value(time_month));
  strm_var_def(time_ns, "day", strm_cfunc_value(time_day));
  strm_var_def(time_ns, "hour", strm_cfunc_value(time_hour));
  strm_var_def(time_ns, "minute", strm_cfunc_value(time_min));
  strm_var_def(time_ns, "second", strm_cfunc_value(time_sec));
  strm_var_def(time_ns, "nanosecond", strm_cfunc_value(time_nanosec));
}
