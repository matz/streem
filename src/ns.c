#include "strm.h"
#include "khash.h"

KHASH_MAP_INIT_INT64(ns, strm_state*);
static khash_t(ns) *nstbl;

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
strm_ns_new(strm_state* state, strm_string name)
{

  strm_state *s = strm_ns_get(name);

  if (!s) {
    s = malloc(sizeof(strm_state));
    if (!s) return NULL;
    else {
      int r;
      khiter_t k;

      if (!nstbl) {
        nstbl = kh_init(ns);
      }
      k = kh_put(ns, nstbl, (intptr_t)name, &r);
      if (r <= 0) return NULL;  /* r=0  key is present in the hash table */
                                /* r=-1 operation failed */
      kh_value(nstbl, k) = s;
    }
    memset(s, 0, sizeof(strm_state));
    s->prev = state;
  }
  return s;
}
