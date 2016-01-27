#include "strm.h"
#include "khash.h"
#include <pthread.h>

KHASH_INIT(kvs, strm_string, strm_value, 1, kh_int64_hash_func, kh_int64_hash_equal);

typedef struct strm_kvs {
  STRM_AUX_HEADER;
  uint16_t flags;
  uint64_t serial;
  khash_t(kvs) *kv;
  union {
    pthread_mutex_t lock;
    struct strm_kvs* prev;
  } u;
} strm_kvs;

static strm_kvs*
get_kvs(int argc, strm_value* args)
{
  if (argc == 0) return NULL;
  return (strm_kvs*)strm_value_ptr(args[0], STRM_PTR_AUX);
}

static int
kvs_get(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;

  if (!k) return STRM_NG;
  pthread_mutex_lock(&k->u.lock);
  i = kh_get(kvs, k->kv, key);
  if (i == kh_end(k->kv)) {
    *ret = strm_nil_value();
  }
  else {
    *ret = kh_value(k->kv, i);
  }
  pthread_mutex_unlock(&k->u.lock);
  return STRM_OK;
}

static int
kvs_put(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;
  int st;
  
  if (!k) return STRM_NG;
  pthread_mutex_lock(&k->u.lock);
  i = kh_put(kvs, k->kv, key, &st);
  if (st < 0) {                 /* st<0: operation failed */
    pthread_mutex_unlock(&k->u.lock);
    return STRM_NG;
  }
  k->serial++;
  kh_value(k->kv, i) = args[2];
  pthread_mutex_unlock(&k->u.lock);
  return STRM_OK;
}

static int
kvs_update(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  strm_value val;
  khiter_t i;
  int st;
  
  if (!k) return STRM_NG;
  pthread_mutex_lock(&k->u.lock);
  i = kh_put(kvs, k->kv, key, &st);
  /* st<0: operation failed */
  /* st>0: key does not exist */
  if (st != 0) {
    pthread_mutex_unlock(&k->u.lock);
    return STRM_NG;
  }
  k->serial++;
  val = kh_value(k->kv, i);
  if (strm_funcall(task, args[1], 1, &val, &val) == STRM_NG) {
    pthread_mutex_unlock(&k->u.lock);
    return STRM_NG;
  }
  kh_value(k->kv, i) = val;
  pthread_mutex_unlock(&k->u.lock);
  return STRM_OK;
}

static int
kvs_close(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  if (!k) return STRM_NG;
  kh_destroy(kvs, k->kv);
  return STRM_OK;
}

static strm_state* kvs_ns;
static strm_state* txn_ns;

static int
kvs_new(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  struct strm_kvs *k = malloc(sizeof(struct strm_kvs));

  if (!k) return STRM_NG;
  k->ns = kvs_ns;
  k->type = STRM_PTR_AUX;
  k->kv = kh_init(kvs);
  k->flags = 0;
  k->serial = 0;
  pthread_mutex_init(&k->u.lock, NULL);
  *ret = strm_ptr_value(k);
  return STRM_OK;
}

static strm_kvs*
txn_new(strm_kvs* kvs)
{
  struct strm_kvs *k = malloc(sizeof(struct strm_kvs));

  if (!k) return NULL;
  k->ns = txn_ns;
  k->type = STRM_PTR_AUX;
  k->kv = kh_init(kvs);
  k->flags = 1;
  k->u.prev = kvs;
  pthread_mutex_lock(&kvs->u.lock);
  k->serial = kvs->serial;
  pthread_mutex_unlock(&kvs->u.lock);
  return k;
}

static int
kvs_txn(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* kvs = get_kvs(argc, args);
  strm_kvs* txn;
  strm_value val;
  khiter_t i, j;
  khash_t(kvs)* h;
  khash_t(kvs)* h0;
  int st;

  if (!kvs) return STRM_NG;
  txn = txn_new(kvs);
  val = strm_ptr_value(txn);
 retry:
  if (strm_funcall(task, args[1], 1, &val, ret) == STRM_NG) {
    return STRM_NG;
  }
  pthread_mutex_lock(&kvs->u.lock);
  if (kvs->serial != txn->serial) {
    pthread_mutex_unlock(&kvs->u.lock);
    goto retry;
  }
  h = txn->kv;
  h0 = kvs->kv;
  for (i = kh_begin(h); i != kh_end(h); i++) {
    if (kh_exist(h, i)) {
      j = kh_put(kvs, h0, kh_key(h, i), &st);
      if (st < 0) {
        pthread_mutex_unlock(&kvs->u.lock);
        return STRM_NG;
      }
      kh_value(h0, j) = kh_value(h, i);
    }
  }
  pthread_mutex_unlock(&kvs->u.lock);
  return STRM_OK;
}

static strm_kvs*
get_txn(int argc, strm_value* args)
{
  strm_kvs* k = get_kvs(argc, args);
  /* check if transaction */
  if (k && k->flags == 0) return NULL;
  return k;
}

static int
txn_get(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_txn(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;

  i = kh_get(kvs, k->kv, key);
  if (i == kh_end(k->kv)) {
    *ret = strm_nil_value();
  }
  else {
    *ret = kh_value(k->kv, i);
  }
  return STRM_OK;
}

static int
txn_put(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_txn(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;
  int st;
  
  i = kh_put(kvs, k->kv, key, &st);
  if (st < 0) {                 /* r<0: operation failed */
    return STRM_NG;
  }
  kh_value(k->kv, i) = args[2];
  return STRM_OK;
}

static int
txn_update(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_txn(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  strm_value val;
  khiter_t i;
  int st;
  
  i = kh_put(kvs, k->kv, key, &st);
  /* st<0: operation failed */
  /* st>0: key does not exist */
  if (st != 0) {
    return STRM_NG;
  }
  val = kh_value(k->kv, i);
  if (strm_funcall(task, args[2], 1, &val, &val) == STRM_NG) {
    return STRM_NG;
  }
  kh_value(k->kv, i) = val;
  return STRM_OK;
}

void
strm_kvs_init(strm_state* state)
{
  kvs_ns = strm_ns_new(NULL);
  strm_var_def(kvs_ns, "get", strm_cfunc_value(kvs_get));
  strm_var_def(kvs_ns, "put", strm_cfunc_value(kvs_put));
  strm_var_def(kvs_ns, "update", strm_cfunc_value(kvs_update));
  strm_var_def(kvs_ns, "txn", strm_cfunc_value(kvs_txn));
  strm_var_def(kvs_ns, "close", strm_cfunc_value(kvs_close));

  txn_ns = strm_ns_new(NULL);
  strm_var_def(txn_ns, "get", strm_cfunc_value(txn_get));
  strm_var_def(txn_ns, "put", strm_cfunc_value(txn_put));
  strm_var_def(txn_ns, "update", strm_cfunc_value(txn_update));

  strm_var_def(state, "kvs", strm_cfunc_value(kvs_new));
}
