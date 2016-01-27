#include "strm.h"
#include "khash.h"
#include <pthread.h>

KHASH_INIT(kvs, strm_string, strm_value, 1, kh_int64_hash_func, kh_int64_hash_equal);
KHASH_INIT(txn, strm_string, strm_value*, 1, kh_int64_hash_func, kh_int64_hash_equal);

typedef struct strm_kvs {
  STRM_AUX_HEADER;
  khash_t(kvs) *kv;
  pthread_mutex_t lock;
} strm_kvs;

typedef struct strm_txn {
  STRM_AUX_HEADER;
  khash_t(txn) *tv;
  strm_kvs* kvs;
} strm_txn;

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
  pthread_mutex_lock(&k->lock);
  i = kh_get(kvs, k->kv, key);
  if (i == kh_end(k->kv)) {
    *ret = strm_nil_value();
  }
  else {
    *ret = kh_value(k->kv, i);
  }
  pthread_mutex_unlock(&k->lock);
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
  pthread_mutex_lock(&k->lock);
  i = kh_put(kvs, k->kv, key, &st);
  if (st < 0) {                 /* st<0: operation failed */
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  kh_value(k->kv, i) = args[2];
  pthread_mutex_unlock(&k->lock);
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
  pthread_mutex_lock(&k->lock);
  i = kh_put(kvs, k->kv, key, &st);
  /* st<0: operation failed */
  /* st>0: key does not exist */
  if (st != 0) {
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  val = kh_value(k->kv, i);
  if (strm_funcall(task, args[1], 1, &val, &val) == STRM_NG) {
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  kh_value(k->kv, i) = val;
  pthread_mutex_unlock(&k->lock);
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
  pthread_mutex_init(&k->lock, NULL);
  *ret = strm_ptr_value(k);
  return STRM_OK;
}

static strm_txn*
txn_new(strm_kvs* kvs)
{
  struct strm_txn *k = malloc(sizeof(struct strm_kvs));

  if (!k) return NULL;
  k->ns = txn_ns;
  k->type = STRM_PTR_AUX;
  k->tv = kh_init(txn);
  k->kvs = kvs;
  return k;
}

static void
txn_free(strm_txn* txn)
{
  khash_t(txn)* tv;
  khiter_t i;

  tv = txn->tv;
  if (!tv) return;
  for (i = kh_begin(tv); i != kh_end(tv); i++) {
    if (kh_exist(tv, i)) {
      free(kh_value(tv, i));
    }
  }
  kh_destroy(txn, tv);
  txn->tv = NULL;
}

static int
kvs_txn(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* kvs = get_kvs(argc, args);
  strm_txn* txn;
  strm_value val;
  khiter_t i, j;
  khash_t(txn)* tv;             /* transaction values */
  khash_t(kvs)* kv;             /* target kvs */
  int st = 0;
  int result = 0;               /* 0: OK, 1: retry, 2: failure */

  if (!kvs) return STRM_NG;
  txn = txn_new(kvs);
  val = strm_ptr_value(txn);
 retry:
  if (strm_funcall(task, args[1], 1, &val, ret) == STRM_NG) {
    txn_free(txn);
    return STRM_NG;
  }
  pthread_mutex_lock(&kvs->lock);
  kv = kvs->kv;
  tv = txn->tv;
  for (i = kh_begin(tv); i != kh_end(tv); i++) {
    if (kh_exist(tv, i)) {
      strm_value key = kh_key(tv, i);
      strm_value *v = kh_value(tv, i);
      j = kh_put(kvs, kv, key, &st);
      if (st < 0) {
        result = 2;             /* failure */
        break;
      }
      if (st == 0) {
        if (v[0] != kh_value(kv, j)) {
          result = 1;           /* conflict */
          break;
        }
      }
      kh_value(kv, j) = v[1];
    }
  }
  pthread_mutex_unlock(&kvs->lock);
  switch (result) {
  case 1:                       /* conflict */
    goto retry;
  default:
  case 2:                       /* failure */
    txn_free(txn);
    return STRM_NG;
  case 0:
    txn_free(txn);
    return STRM_OK;
  }
}

static strm_txn*
get_txn(int argc, strm_value* args)
{
  if (argc == 0) return NULL;
  return (strm_txn*)strm_value_ptr(args[0], STRM_PTR_AUX);
}

static int
txn_get(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_kvs* k = t->kvs;
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;
  strm_value* v;
  int st;

  i = kh_get(txn, t->tv, key);
  if (i == kh_end(t->tv)) {     /* not in transaction */
    i = kh_get(kvs, k->kv, key);
    if (i == kh_end(k->kv)) {     /* not in database */
      *ret = strm_nil_value();
    }
    v = malloc(sizeof(strm_value)*2);
    v[0] = strm_nil_value();
    v[1] = kh_value(k->kv, i);
    i = kh_put(txn, t->tv, key, &st);
    if (st < 0) {
      free(v);
      return STRM_NG;
    }
    kh_value(t->tv, i) = v;
  }
  else {
    v = kh_value(t->tv, i);
    *ret = v[1];
  }
  return STRM_OK;
}

static int
txn_put(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_kvs* k = t->kvs;
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  strm_value *v;
  khiter_t i, j;
  int st;

  i = kh_put(txn, t->tv, key, &st);
  if (st < 0) {                 /* st<0: operation failed */
    return STRM_NG;
  }
  if (st == 0) {                /* st=0: key exists */
    v = kh_value(t->tv, i);
    v[1] = args[2];
  }
  else {
    strm_value val;

    j = kh_get(kvs, k->kv, key);
    if (j == kh_end(k->kv)) {     /* not in database */
      val = strm_nil_value();
    }
    else {
      val = kh_value(k->kv, j);
    }
    v = malloc(sizeof(strm_value)*2);
    v[0] = val;
    v[1] = args[2];
    kh_value(t->tv, i) = v;
  }
  return STRM_OK;
}

static int
txn_update(strm_task* task, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_kvs* k = t->kvs;
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
