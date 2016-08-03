#include "strm.h"
#include "khash.h"
#include <pthread.h>

KHASH_INIT(kvs, strm_string, strm_value, 1, kh_int64_hash_func, kh_int64_hash_equal);
KHASH_INIT(txn, strm_string, strm_value, 1, kh_int64_hash_func, kh_int64_hash_equal);

typedef struct strm_kvs {
  STRM_AUX_HEADER;
  khash_t(kvs) *kv;
  uint64_t serial;
  pthread_mutex_t lock;
} strm_kvs;

typedef struct strm_txn {
  STRM_AUX_HEADER;
  khash_t(txn) *tv;
  strm_kvs* kvs;
  uint64_t serial;
} strm_txn;

static strm_kvs*
get_kvs(int argc, strm_value* args)
{
  if (argc == 0) return NULL;
  return (strm_kvs*)strm_value_ptr(args[0], STRM_PTR_AUX);
}

/* db.get(key): return a value corresponding to the key (or nil) */
static int
kvs_get(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;

  if (!k) {
    strm_raise(strm, "no kvs given");
    return STRM_NG;
  }
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

static uint64_t
kvs_serial(strm_kvs* kvs)
{
  uint64_t serial;

  pthread_mutex_lock(&kvs->lock);
  serial = kvs->serial;
  pthread_mutex_unlock(&kvs->lock);
  return serial;
}

/* db.put(key,val): save the value associated with the key */
static int
kvs_put(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;
  int st;
  
  if (!k) {
    strm_raise(strm, "no kvs given");
    return STRM_NG;
  }
  pthread_mutex_lock(&k->lock);
  i = kh_put(kvs, k->kv, key, &st);
  if (st < 0) {                 /* st<0: operation failed */
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  k->serial++;
  kh_value(k->kv, i) = args[2];
  pthread_mutex_unlock(&k->lock);
  return STRM_OK;
}

/* db.update(key){x->new_x}: update value for key with function */
/* raises error when value does not exist */
static int
kvs_update(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  strm_value old, val;
  khiter_t i;
  int st;

  if (!k) {
    strm_raise(strm, "no kvs given");
    return STRM_NG;
  }
  /* get the old value */
  pthread_mutex_lock(&k->lock);
  i = kh_get(kvs, k->kv, key);
  if (i == kh_end(k->kv)) {
    /* no previous value */
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  old = kh_value(k->kv, i);
  pthread_mutex_unlock(&k->lock);

  /* call function */
  if (strm_funcall(strm, args[2], 1, &old, &val) == STRM_NG) {
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  
  pthread_mutex_lock(&k->lock);
  i = kh_put(kvs, k->kv, key, &st);
  /* st<0: operation failed */
  /* st>0: key does not exist */
  if (st != 0 || kh_value(k->kv, i) != old) {
    pthread_mutex_unlock(&k->lock);
    return STRM_NG;
  }
  k->serial++;
  kh_value(k->kv, i) = val;
  pthread_mutex_unlock(&k->lock);
  *ret = val;
  return STRM_OK;
}

/* db.close(): close db and free memory */
static int
kvs_close(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* k = get_kvs(argc, args);
  if (!k) {
    strm_raise(strm, "no kvs given");
    return STRM_NG;
  }
  kh_destroy(kvs, k->kv);
  return STRM_OK;
}

static strm_state* ns_kvs;
static strm_state* ns_txn;

/* kvs(): return a new instance of key-value store */
static int
kvs_new(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct strm_kvs *k = malloc(sizeof(struct strm_kvs));

  if (!k) return STRM_NG;
  k->ns = ns_kvs;
  k->type = STRM_PTR_AUX;
  k->kv = kh_init(kvs);
  k->serial = 1;
  pthread_mutex_init(&k->lock, NULL);
  *ret = strm_ptr_value(k);
  return STRM_OK;
}

static strm_txn*
txn_new(strm_kvs* kvs)
{
  struct strm_txn *t = malloc(sizeof(struct strm_txn));

  if (!t) return NULL;
  t->ns = ns_txn;
  t->type = STRM_PTR_AUX;
  t->tv = kh_init(txn);
  t->kvs = kvs;
  t->serial = kvs_serial(kvs);
  return t;
}

static void
txn_free(strm_txn* txn)
{
  kh_destroy(txn, txn->tv);
  txn->tv = NULL;
}

#define MAXTRY 10

/* db.txn{txn->...}: run transaction on db */
/* txn works like db (but no close/txn), updated at once */
/* at the end of the function */
static int
kvs_txn(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_kvs* kvs = get_kvs(argc, args);
  strm_txn* txn;
  strm_value val;
  khiter_t i, j;
  khash_t(txn)* tv;             /* transaction values */
  khash_t(kvs)* kv;             /* target kvs */
  int st = 0;
  int result = 0;               /* 0: OK, 1: retry, 2: failure */
  int tries = 0;

  if (!kvs) {
    strm_raise(strm, "no kvs given");
    return STRM_NG;
  }
  txn = txn_new(kvs);
  val = strm_ptr_value(txn);
 retry:
  if (strm_funcall(strm, args[1], 1, &val, ret) == STRM_NG) {
    if (txn->serial == 0) {
      tries++;
      if (tries > MAXTRY) {
        strm_raise(strm, "too many transaction retries");
        goto fail;
      }
      txn->serial = kvs_serial(kvs);
      goto retry;
    }
    txn_free(txn);
    return STRM_NG;
  }
  pthread_mutex_lock(&kvs->lock);
  if (kvs->serial != txn->serial) {
    pthread_mutex_unlock(&kvs->lock);
    goto retry;
  }
  kv = kvs->kv;
  tv = txn->tv;
  for (i = kh_begin(tv); i != kh_end(tv); i++) {
    if (kh_exist(tv, i)) {
      strm_value key = kh_key(tv, i);
      strm_value v = kh_value(tv, i);
      j = kh_put(kvs, kv, key, &st);
      if (st < 0) {
        pthread_mutex_unlock(&kvs->lock);
        goto fail;
      }
      kh_value(kv, j) = v;
    }
  }
  if (result == 0) {
    kvs->serial++;
  }
  pthread_mutex_unlock(&kvs->lock);
  switch (result) {
  case 1:                       /* conflict */
    goto retry;
  default:
  case 2:                       /* failure */
  case 0:
    txn_free(txn);
    return STRM_OK;
  }
 fail:
  txn_free(txn);
  return STRM_NG;
}

static strm_txn*
get_txn(int argc, strm_value* args)
{
  strm_txn* txn;

  if (argc == 0) return NULL;
  txn = (strm_txn*)strm_value_ptr(args[0], STRM_PTR_AUX);
  if (!txn->tv) {
    return NULL;
  }
  return txn;
}

static int
void_txn(strm_stream* strm)
{
  strm_raise(strm, "invalid transaction");
  return STRM_NG;
}

static int
txn_retry(strm_txn* txn)
{
  txn->serial = 0;              /* retry mark */
  return STRM_NG;
}

/* txn.get(key): return a value corresponding to the key (or nil) */
static int
txn_get(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_kvs* k;
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;

  if (!t) return void_txn(strm);
  k = t->kvs;
  if (t->serial != kvs_serial(k)) {
    return txn_retry(t);
  }
  i = kh_get(txn, t->tv, key);
  if (i == kh_end(t->tv)) {     /* not in transaction */
    pthread_mutex_lock(&k->lock);
    i = kh_get(kvs, k->kv, key);
    if (i == kh_end(k->kv)) {     /* not in database */
      *ret = strm_nil_value();
    }
    else {
      *ret = kh_value(k->kv, i);
    }
    pthread_mutex_unlock(&k->lock);
  }
  else {
    *ret = kh_value(t->tv, i);
  }
  return STRM_OK;
}

/* txn.put(key,val): save the value associated with the key */
static int
txn_put(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  khiter_t i;
  int st;

  if (!t) return void_txn(strm);
  i = kh_put(txn, t->tv, key, &st);
  if (st < 0) {                 /* st<0: operation failed */
    return STRM_NG;
  }
  kh_value(t->tv, i) = args[2];
  return STRM_OK;
}

/* txn.update(key){x->new_x}: update value for key with function */
static int
txn_update(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  strm_txn* t = get_txn(argc, args);
  strm_kvs* k;
  strm_string key = strm_str_intern_str(strm_to_str(args[1]));
  strm_value val;
  khiter_t i;
  int st;

  if (!t) return void_txn(strm);
  k = t->kvs;
  if (t->serial != kvs_serial(k)) {
    return txn_retry(t);
  }
  i = kh_put(txn, t->tv, key, &st);
  /* st<0: operation failed */
  if (st < 0) {
    return STRM_NG;
  }
  /* st=0: key exists */
  if (st == 0) {
    val = kh_value(t->tv, i);
  }
  /* st>0: key does not exist */
  else {
    pthread_mutex_lock(&k->lock);
    i = kh_get(kvs, k->kv, key);
    if (i == kh_end(k->kv)) {     /* not in database */
      pthread_mutex_unlock(&k->lock);
      return STRM_NG;
    }
    else {
      val = kh_value(k->kv, i);
    }
    pthread_mutex_unlock(&k->lock);
  }
  if (strm_funcall(strm, args[2], 1, &val, &val) == STRM_NG) {
    return STRM_NG;
  }
  kh_value(t->tv, i) = val;
  *ret = val;
  return STRM_OK;
}

static strm_value
to_str(strm_stream* strm, strm_value val, char* type)
{
  char buf[256];
  int n;

  n = sprintf(buf, "<%s:%p>", type, (void*)strm_value_vptr(val));
  return strm_str_new(buf, n);
}

static int
kvs_str(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  if (argc != 1) return STRM_NG;
  *ret = to_str(strm, args[0], "kvs");
  return STRM_OK;
}

static int
txn_str(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  if (argc != 1) return STRM_NG;
  *ret = to_str(strm, args[0], "txn");
  return STRM_OK;
}

void
strm_kvs_init(strm_state* state)
{
  ns_kvs = strm_ns_new(NULL, "kvs");
  strm_var_def(ns_kvs, "get", strm_cfunc_value(kvs_get));
  strm_var_def(ns_kvs, "put", strm_cfunc_value(kvs_put));
  strm_var_def(ns_kvs, "update", strm_cfunc_value(kvs_update));
  strm_var_def(ns_kvs, "txn", strm_cfunc_value(kvs_txn));
  strm_var_def(ns_kvs, "close", strm_cfunc_value(kvs_close));
  strm_var_def(ns_kvs, "string", strm_cfunc_value(kvs_str));

  ns_txn = strm_ns_new(NULL, "kvs_txn");
  strm_var_def(ns_txn, "get", strm_cfunc_value(txn_get));
  strm_var_def(ns_txn, "put", strm_cfunc_value(txn_put));
  strm_var_def(ns_txn, "update", strm_cfunc_value(txn_update));
  strm_var_def(ns_kvs, "string", strm_cfunc_value(txn_str));

  strm_var_def(state, "kvs", strm_cfunc_value(kvs_new));
}
