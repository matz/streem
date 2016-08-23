#include "strm.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

static void
xorshift128init(uint64_t seed[2])
{
  struct timeval tv;
  uint64_t x;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
  int fd = open("/dev/urandom",
# ifdef O_NONBLOCK
                           O_NONBLOCK|
# endif
# ifdef O_NOCTTY
                           O_NOCTTY|
# endif
                           O_RDONLY, 0);
  if (fd > 0) {
    struct stat statbuf;
    ssize_t ret = 0;
    const size_t size = sizeof(uint64_t)*2;

    if (fstat(fd, &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
      ret = read(fd, seed, size);
    }
    close(fd);
    if ((size_t)ret == size) return;
  }
#endif

  x = 88172645463325252ULL;
  gettimeofday(&tv, NULL);
  x ^= (uint64_t)tv.tv_usec;
  x = x ^ (x << 13);
  x = x ^ (x >> 7);
  x = x ^ (x << 17);
  seed[0] = x;
  x = x ^ (x << 13);
  x = x ^ (x >> 7);
  x = x ^ (x << 17);
  seed[1] = x;
}

static uint64_t
xorshift128plus(uint64_t seed[2])
{
  uint64_t x = seed[0];
  const uint64_t y = seed[1];
  seed[0] = y;
  x ^= x << 23; /* a */
  seed[1] = x ^ y ^ (x >> 17) ^ (y >> 26); /* b, c */
  return seed[1] + y;
}

struct rand_data {
  uint64_t seed[2];
  strm_int limit;
};

static int
gen_rand(strm_stream* strm, strm_value data)
{
  struct rand_data* d = strm->data;
  uint64_t r = xorshift128plus(d->seed);

  strm_emit(strm, strm_int_value(r % d->limit), gen_rand);
  return STRM_OK;
}

static int
fin_rand(strm_stream* strm, strm_value data)
{
  free(strm->data);
  return STRM_OK;
}

static int
exec_rand(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_int n;
  struct rand_data* d;
  const char* s;
  strm_int len;

  strm_get_args(strm, argc, args, "i|s", &n, &s, &len);
  d = malloc(sizeof(struct rand_data));
  if (!d) return STRM_NG;
  d->limit = n;
  if (argc == 2) {
    if (len != sizeof(d->seed)) {
      strm_raise(strm, "seed size differ");
      return STRM_NG;
    }
    memcpy(d->seed, s, len);
  }
  else {
    xorshift128init(d->seed);
  }
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_rand, fin_rand, (void*)d));
  return STRM_OK;
}

static int
rand_seed(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  uint64_t seed[2];

  strm_get_args(strm, argc, args, "");
  xorshift128init(seed);
  *ret = strm_str_new((const char*)seed, sizeof(seed));
  return STRM_OK;
}

struct sample_data {
  uint64_t seed[2];
  strm_int i;
  strm_int len;
  strm_value samples[0];
};

static int
iter_sample(strm_stream* strm, strm_value data)
{
  struct sample_data* d = strm->data;
  uint64_t r;

  if (d->i < d->len) {
    d->samples[d->i++] = data;
    return STRM_OK;
  }
  r = xorshift128plus(d->seed)%(d->i);
  if (r < d->len) {
    d->samples[r] = data;
  }
  d->i++;
  return STRM_OK;
}

static int
finish_sample(strm_stream* strm, strm_value data)
{
  struct sample_data* d = strm->data;
  strm_int i, len=d->len;

  for (i=0; i<len; i++) {
    strm_emit(strm, d->samples[i], NULL);
  }
  free(d);
  return STRM_OK;
}

static int
exec_sample(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct sample_data* d;
  strm_int len;

  strm_get_args(strm, argc, args, "i", &len);
  d = malloc(sizeof(struct sample_data)+sizeof(strm_value)*len);
  if (!d) return STRM_NG;
  d->len = len;
  d->i = 0;
  xorshift128init(d->seed);
  *ret = strm_stream_value(strm_stream_new(strm_filter, iter_sample,
                                           finish_sample, (void*)d));
  return STRM_OK;
}

void
strm_rand_init(strm_state* state)
{
  strm_var_def(state, "rand_seed", strm_cfunc_value(rand_seed));
  strm_var_def(state, "rand", strm_cfunc_value(exec_rand));
  strm_var_def(state, "sample", strm_cfunc_value(exec_sample));
}
