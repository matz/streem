#include "strm.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>


static void
xorshift128init(uint32_t seed[4])
{
  struct timeval tv;
  uint32_t y;
  int i;

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

  y = 2463534242;
  gettimeofday(&tv, NULL);
  y ^= (uint32_t)tv.tv_usec;
  for (i=0; i<4; i++) {
    y = y ^ (y << 13); y = y ^ (y >> 17);
    y = y ^ (y << 5);
    seed[i] = y;
  }
  return;
}

static uint32_t
xorshift128(uint32_t seed[4])
{ 
  uint32_t x = seed[0];
  uint32_t y = seed[1];
  uint32_t z = seed[2];
  uint32_t w = seed[3]; 
  uint32_t t;
 
  t = x ^ (x << 11);
  x = y; y = z; z = w;
  w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
  seed[0] = x;
  seed[1] = y;
  seed[2] = z;
  seed[3] = w;
  return w;
}

struct rand_data {
  uint32_t seed[4];
  strm_int limit;
};

static int
gen_rand(strm_stream* strm, strm_value data)
{
  struct rand_data* d = strm->data;
  uint32_t r = xorshift128(d->seed);

  strm_emit(strm, strm_int_value(r % d->limit), gen_rand);
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
  *ret = strm_stream_value(strm_stream_new(strm_producer, gen_rand, NULL, (void*)d));
  return STRM_OK;
}

static int
rand_seed(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  uint32_t seed[4];

  strm_get_args(strm, argc, args, "");
  xorshift128init(seed);
  *ret = strm_str_new((const char*)seed, sizeof(seed));
  return STRM_OK;
}

struct sample_data {
  uint32_t seed[4];
  strm_int i;
  strm_int len;
  strm_value samples[0];
};

static int
iter_sample(strm_stream* strm, strm_value data)
{
  struct sample_data* d = strm->data;
  uint32_t r;

  if (d->i < d->len) {
    d->samples[d->i++] = data;
    return STRM_OK;
  }
  r = xorshift128(d->seed)%(d->i);
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
