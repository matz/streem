#include "strm.h"
#include "queue.h"

const char* strm_p(strm_value val);

struct recv_data {
  strm_stream* strm;
  strm_callback func;
};

struct chan_data {
  struct strm_queue* dq;        /* data queue */
  struct strm_queue* rq;        /* receiver queue */
};

int
strm_chan_finish_p(strm_stream* chan)
{
  struct chan_data* c;

  if (chan->mode == strm_consumer) return 0;
  c = chan->data;
  if (c->dq->head == c->dq->tail) return 1;
  return 0;
}

static int
chan_get(strm_stream* strm, strm_value data)
{
  struct chan_data* d = strm->data;
  struct recv_data* r = strm_queue_get(d->rq);

  if (r) {
    (*r->func)(r->strm, data);
    free(r);
  }
  else {
    strm_value* v = malloc(sizeof(strm_value));
    *v = data;
    strm_queue_add(d->dq, v);
  }
  return STRM_OK;
}

void
strm_chan_receive(strm_stream* chan, strm_stream* strm, strm_callback func)
{
  struct chan_data* d;
  strm_value* v;

  assert(chan->start_func == chan_get);
  d = chan->data;
  v = strm_queue_get(d->dq);
  if (v) {
    (*func)(strm, *v);
    free(v);
  }
  else {
    struct recv_data* r = malloc(sizeof(struct recv_data));
    r->strm = strm;
    r->func = func;
    strm_queue_add(d->rq, r);
  }
}

static int
chan_close(strm_stream* strm, strm_value data)
{
  struct chan_data* d = strm->data;
  struct recv_data* r;

  for (;;) {
    r = strm_queue_get(d->rq);
    if (!r) break;
    (*r->func)(r->strm, strm_nil_value());
    free(r);
  }
  return STRM_OK;
}

strm_stream*
strm_chan_new()
{
  struct chan_data* d = malloc(sizeof(struct chan_data));

  assert(d != NULL);
  d->dq = strm_queue_new();
  d->rq = strm_queue_new();
  return strm_stream_new(strm_consumer, chan_get, chan_close, d);
}

struct zip_data {
  strm_array a;
  strm_int i;
  strm_int len;
  strm_stream* chan[0];
};

static int zip_start(strm_stream* strm, strm_value data);

static int
zip_iter(strm_stream* strm, strm_value data)
{
  struct zip_data* z = strm->data;

  strm_ary_ptr(z->a)[z->i++] = data;
  if (z->i < z->len) {
    strm_chan_receive(z->chan[z->i], strm, zip_iter);
  }
  else {
    strm_int i;
    strm_int done = 0;

    for (i=0; i<z->len; i++){
      if (strm_chan_finish_p(z->chan[i]))
        done = 1;
    }
    if (done) {
      for (i=0; i<z->len; i++){
        strm_stream_close(z->chan[i]);
      }
      strm_stream_close(strm);
    }
    else {
      strm_emit(strm, z->a, zip_start);
    }
  }
  return STRM_OK;
}

static int
zip_start(strm_stream* strm, strm_value data)
{
  struct zip_data* z = strm->data;

  if (z) {
    z->i = 0;
    z->a = strm_ary_new(NULL, z->len);
    strm_chan_receive(z->chan[0], strm, zip_iter);
  }
  return STRM_OK;
}

static int
zip_close(strm_stream* strm, strm_value data)
{
  strm->data = NULL;
  return STRM_OK;
}

static int
exec_zip(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct zip_data* z = malloc(sizeof(struct zip_data)+sizeof(strm_stream*)*argc);
  strm_int i;
  strm_stream* s;

  z->i = 0;
  z->len = argc;
  for (i=0; i<argc; i++) {
    s = strm_chan_new();
    strm_stream_connect(strm_value_stream(args[i]), s);
    z->chan[i] = s;
  }
  *ret = strm_stream_value(strm_stream_new(strm_producer, zip_start, zip_close, z));
  return STRM_OK;
}

void
strm_chan_init(strm_state* state)
{
  strm_var_def(state, "zip", strm_cfunc_value(exec_zip));
}
