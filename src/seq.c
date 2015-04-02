#include "strm.h"
#include "node.h"

struct seq_seeder {
  long n;
  long end;
};

static void
seq_seed(strm_stream *strm, strm_value data)
{
  struct seq_seeder *s = strm->data;

  if (s->n > s->end) {
    strm_close(strm);
    return;
  }
  strm_emit(strm, strm_int_value(s->n), seq_seed);
  s->n++;
}

static strm_stream*
strm_seq(long start, long end)
{
  struct seq_seeder *s = malloc(sizeof(struct seq_seeder));
  s->n = start;
  s->end = end;
  return strm_alloc_stream(strm_task_prod, seq_seed, NULL, (void*)s);
}

static int
exec_seq(node_ctx* ctx, int argc, strm_value* args, strm_value* ret)
{
  long start, end;

  switch (argc) {
  case 1:
    start = 1;
    end = strm_value_int(args[0]);
    break;
  case 2:
    start = strm_value_int(args[0]);
    end = strm_value_int(args[1]);
    break;
  default:
    node_raise(ctx, "wrong number of arguments");
    return 1;
  }
  *ret = strm_task_value(strm_seq(start, end));
  return 0;
}

void
strm_seq_init(node_ctx* ctx)
{
  strm_var_def("seq", strm_cfunc_value(exec_seq));
}
