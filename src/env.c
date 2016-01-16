#include "strm.h"
#include "khash.h"

KHASH_MAP_INIT_INT64(env, strm_value);
typedef khash_t(env) strm_env;
strm_env *globals;

static int
env_set(strm_env *env, strm_string name, strm_value val)
{
  int r;
  khiter_t k;

  assert(env != globals || !strm_event_loop_started);
  if (!strm_str_intern_p(name)) {
    name = strm_str_intern_str(name);
  }
  k = kh_put(env, env, (intptr_t)name, &r);
  if (r <= 0) return STRM_NG;   /* r=0  key is present in the hash table */
                                /* r=-1 operation failed */
  kh_value(env, k) = val;
  return STRM_OK;
}

int
env_get(strm_env* env, strm_string name, strm_value* val)
{
  khiter_t k;

  if (!strm_str_intern_p(name)) {
    name = strm_str_intern_str(name);
  }
  k = kh_get(env, env, (intptr_t)name);
  if (k == kh_end(env)) return STRM_NG;
  *val = kh_value(env, k);
  return STRM_OK;
}


int
strm_var_set(strm_state* state, strm_string name, strm_value val)
{
  strm_env *e;

  if (!state) {
    if (!globals) {
      globals = kh_init(env);
    }
    e = globals;
  }
  else {
    if (!state->env) {
      state->env = kh_init(env);
    }
    e = state->env;
  }
  return env_set(e, name, val);
}

int
strm_var_def(strm_state* state, const char* name, strm_value val)
{
  return strm_var_set(state, strm_str_intern(name, strlen(name)), val);
}

int
strm_var_get(strm_state* state, strm_string name, strm_value* val)
{
  while (state) {
    if (state->env) {
      if (env_get((strm_env*)state->env, name, val) == 0)
        return STRM_OK;
    }
    state = state->prev;
  }
  if (!globals) return STRM_NG;
  return env_get(globals, name, val);
}

int
strm_env_copy(strm_state* s1, strm_state* s2)
{
  strm_env *e1 = s1->env;
  strm_env *e2 = s2->env;
  khiter_t k, kk;
  int r;

  if (!e1) {
    e1 = s1->env = kh_init(env);
  }
  if (!e2) {
    e2 = s1->env = kh_init(env);
  }
  for (k = kh_begin(e2); k != kh_end(e2); k++) {
    if (kh_exist(e2, k)) {
      kk = kh_put(env, e1, kh_key(e2, k), &r);
      if (r <= 0) return STRM_NG; /* r=0  key is present in the hash table */
                                  /* r=-1 operation failed */
      kh_value(e1, kk) = kh_value(e2, k);
    }
  }
  return STRM_OK;
}
