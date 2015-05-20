#include "strm.h"
#include "node.h"
#include "khash.h"

KHASH_MAP_INIT_INT64(env, strm_value);
typedef khash_t(env) strm_env;
strm_env *globals;

static int
env_set(strm_env *env, strm_string* name, strm_value val)
{
  int r;
  khiter_t k;

  assert(env != globals || !strm_event_loop_started);
  if ((name->flags & STRM_STR_INTERNED) == 0) {
    name = strm_str_intern(name->ptr, name->len);
  }
  k = kh_put(env, env, (intptr_t)name, &r);
  if (r <= 0) return 1;         /* r=0  key is present in the hash table */
                                /* r=-1 operation failed */
  kh_value(env, k) = val;
  return 0;
}

int
env_get(strm_env *env, strm_string* name, strm_value *val)
{
  khiter_t k;

  if ((name->flags & STRM_STR_INTERNED) == 0) {
    name = strm_str_intern(name->ptr, name->len);
  }
  k = kh_get(env, env, (intptr_t)name);
  if (k == kh_end(env)) return 1;
  *val = kh_value(env, k);
  return 0;
}


int
strm_var_set(node_ctx *ctx, strm_string* name, strm_value val)
{
  strm_env *e;

  if (!ctx) {
    if (!globals) {
      globals = kh_init(env);
    }
    e = globals;
  }
  else {
    if (!ctx->env) {
      ctx->env = kh_init(env);
    }
    e = ctx->env;
  }
  return env_set(e, name, val);
}

int
strm_var_def(const char* name, strm_value val)
{
  return strm_var_set(NULL, strm_str_intern(name, strlen(name)), val);
}

int
strm_var_get(node_ctx *ctx, strm_string* name, strm_value *val)
{
  while (ctx) {
    if (ctx->env) {
      if (env_get((strm_env*)ctx->env, name, val) == 0)
        return 0;
    }
    ctx = ctx->prev;
  }
  if (!globals) return 1;
  return env_get(globals, name, val);
}
