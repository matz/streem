#include "strm.h"

struct seq_seeder {
  long n;
  long end;
  long inc;
};

static int
seq_seed(strm_task* task, strm_value data)
{
  struct seq_seeder *s = task->data;

  if (s->n > s->end) {
    strm_task_close(task);
    return STRM_OK;
  }
  strm_emit(task, strm_int_value(s->n), seq_seed);
  s->n += s->inc;
  return STRM_OK;
}

static int
exec_seq(strm_state* state, int argc, strm_value* args, strm_value* ret)
{
  long start, end, inc=1;
  struct seq_seeder *s;

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
  s = malloc(sizeof(struct seq_seeder));
  s->n = start;
  s->inc = inc;
  s->end = end;
  *ret = strm_task_value(strm_task_new(strm_task_prod, seq_seed, NULL, (void*)s));
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
  strm_var_def(state, "each", strm_cfunc_value(exec_each));
  strm_var_def(state, "map", strm_cfunc_value(exec_map));
  strm_var_def(state, "filter", strm_cfunc_value(exec_filter));
  strm_var_def(state, "count", strm_cfunc_value(exec_count));
}
