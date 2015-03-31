#include "strm.h"
#include "khash.h"

KHASH_MAP_INIT_INT64(env, strm_value);
typedef khash_t(env) strm_env;
strm_env *globals;

static void
env_set(strm_env *env, struct strm_string* name, strm_value val)
{
  int r;
  khiter_t k;

  assert(!strm_event_loop_started);
  if ((name->flags & STRM_STR_INTERNED) == 0) {
    name = strm_str_intern(name->ptr, name->len);
  }
  k = kh_put(env, env, (intptr_t)name, &r);
  kh_value(env, k) = val;
}

strm_value
env_get(strm_env *env, struct strm_string* name)
{
  khiter_t k;

  if ((name->flags & STRM_STR_INTERNED) == 0) {
    name = strm_str_intern(name->ptr, name->len);
  }
  k = kh_get(env, env, (intptr_t)name);
  return kh_value(env, k);
}

void
strm_var_setv(struct strm_string* name, strm_value val)
{
  if (!globals) {
    globals = kh_init(env);
  }
  env_set(globals, name, val);
}

void
strm_var_set(const char* name, strm_value val)
{
  strm_var_setv(strm_str_intern(name, strlen(name)), val);
}

strm_value
strm_var_getv(struct strm_string* name)
{
  if (!globals) {
    globals = kh_init(env);
  }
  return env_get(globals, name);
}

strm_value
strm_var_get(const char* name)
{
  return strm_var_getv(strm_str_intern(name, strlen(name)));
}
