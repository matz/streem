#include "strm.h"
#include "khash.h"

KHASH_MAP_INIT_INT64(ns, strm_state*);
static khash_t(ns) *nstbl;
static strm_state szero = {0};
                   
strm_string
strm_ns_name(strm_state* state)
{
  khiter_t k;

  if (!nstbl) return strm_str_null;
  for (k = kh_begin(nstbl); k != kh_end(nstbl); ++k) {
    if (kh_exist(nstbl, k)) {
      if (kh_value(nstbl, k) == state)
        return kh_key(nstbl, k);
    }
  }
  return strm_str_null;
}

strm_state*
strm_ns_get(strm_string name)
{
  khiter_t k;

  if (!nstbl) return NULL;
  k = kh_get(ns, nstbl, (intptr_t)name);
  if (k == kh_end(nstbl)) return NULL;
  return kh_value(nstbl, k);
}

strm_state*
strm_ns_new(strm_state* state)
{
  strm_state* s = malloc(sizeof(strm_state));
  if (!s) return NULL;
  *s = szero;
  s->prev = state;
  return s;
}

strm_state*
strm_ns_find(strm_state* state, strm_string name)
{
  strm_state* s = strm_ns_get(name);

  if (!s) {
    int r;
    khiter_t k;

    if (!nstbl) {
      nstbl = kh_init(ns);
    }
    s = malloc(sizeof(strm_state));
    if (!s) return NULL;
    k = kh_put(ns, nstbl, (intptr_t)name, &r);
    if (r < 0) {                /* r<0 operation failed */
      free(s);
      return NULL;
    }
    *s = szero;
    s->prev = state;
    kh_value(nstbl, k) = s;
  }
  return s;
}
