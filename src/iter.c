#include "strm.h"

struct seq_data {
  strm_int n;
  strm_int end;
  strm_int inc;
};

static int
gen_seq(strm_task* task, strm_value data)
{
  struct seq_data *s = task->data;

  if (s->n > s->end) {
    strm_task_close(task);
    return STRM_OK;
  }
  strm_emit(task, strm_int_value(s->n), gen_seq);
  s->n += s->inc;
  return STRM_OK;
}

static int
exec_seq(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  strm_int start, end, inc=1;
  struct seq_data *s;

  switch (argc) {
  case 1:
    start = 1;
    end = strm_value_int(args[0]);
    break;
  case 2:
    start = strm_value_int(args[0]);
    end = strm_value_int(args[1]);
    break;
  case 3:
    start = strm_value_int(args[0]);
    inc = strm_value_int(args[1]);
    end = strm_value_int(args[2]);
    break;
  default:
    strm_raise(state, "wrong number of arguments");
    return STRM_NG;
  }
  s = malloc(sizeof(struct seq_data));
  s->n = start;
  s->inc = inc;
  s->end = end;
  *ret = strm_task_value(strm_task_new(strm_task_prod, gen_seq, NULL, (void*)s));
  return STRM_OK;
}

#include <sys/time.h>

static uint64_t x; /* the state must be seeded with a nonzero value. */

static uint64_t
xorshift64star(void)
{
  x ^= x >> 12; /* a */
  x ^= x << 25; /* b */
  x ^= x >> 27; /* c */
  return x * UINT64_C(2685821657736338717);
}

static int
rand_iter(strm_task* task, strm_value data)
{
  strm_int n = (strm_int)(intptr_t)task->data;
  uint64_t r = xorshift64star();
  
  strm_emit(task, strm_int_value(r % n), rand_iter);
  return STRM_OK;
}

static int
exec_rand(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  struct timeval tv;
  strm_int n;

  if (argc != 1) {
    strm_raise(state, "wrong number of arguments");
    return STRM_NG;
  }
  n = strm_value_int(args[0]);
  gettimeofday(&tv, NULL);
  x ^= (uint32_t)tv.tv_usec;
  x ^= x >> 11; x ^= x << 17; x ^= x >> 4;
  x *= 2685821657736338717LL;
  *ret = strm_task_value(strm_task_new(strm_task_prod, rand_iter, NULL,
                                       (void*)(intptr_t)n));
  return STRM_OK;
}

struct map_data {
  strm_value func;
};

static int
each(strm_task* task, strm_value data)
{
  struct map_data *m = task->data;
  strm_value val;

  if (strm_funcall(NULL, m->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  return STRM_OK;
}

static int
exec_each(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_task_value(strm_task_new(strm_task_cons, each, NULL, (void*)m));
  return STRM_OK;
}

static int
map(strm_task* task, strm_value data)
{
  struct map_data *m = task->data;
  strm_value val;

  if (strm_funcall(NULL, m->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  strm_emit(task, val, NULL);
  return STRM_OK;
}

static int
exec_map(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_task_value(strm_task_new(strm_task_filt, map, NULL, (void*)m));
  return STRM_OK;
}

static int
filter(strm_task* task, strm_value data)
{
  struct map_data *m = task->data;
  strm_value val;

  if (strm_funcall(NULL, m->func, 1, &data, &val) == STRM_NG) {
    return STRM_NG;
  }
  if (strm_value_bool(val)) {
    strm_emit(task, data, NULL);
  }
  return STRM_OK;
}

static int
exec_filter(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  struct map_data* m = malloc(sizeof(struct map_data));

  m->func = args[0];
  *ret = strm_task_value(strm_task_new(strm_task_filt, filter, NULL, (void*)m));
  return STRM_OK;
}

struct count_data {
  long count;
};

static int
count(strm_task* task, strm_value data)
{
  struct count_data *s = task->data;

  s->count++;
  return STRM_OK;
}

static int
count_finish(strm_task* task, strm_value data)
{
  struct count_data *s = task->data;

  strm_emit(task, strm_int_value(s->count), NULL);
  return STRM_OK;
}

static int
exec_count(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  struct count_data* c = malloc(sizeof(struct count_data));
  c->count = 0;
  *ret = strm_task_value(strm_task_new(strm_task_filt, count, count_finish, (void*)c));
  return STRM_OK;
}

void
strm_iter_init(strm_state* state)
{
  strm_var_def(state, "seq", strm_cfunc_value(exec_seq));
  strm_var_def(state, "rand", strm_cfunc_value(exec_rand));
  strm_var_def(state, "each", strm_cfunc_value(exec_each));
  strm_var_def(state, "map", strm_cfunc_value(exec_map));
  strm_var_def(state, "filter", strm_cfunc_value(exec_filter));
  strm_var_def(state, "count", strm_cfunc_value(exec_count));
}
