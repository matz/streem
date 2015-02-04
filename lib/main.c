#include "strm.h"

static void
map_recv(strm_stream *strm, strm_value data)
{
  strm_map_func func = strm->data;
  strm_value d;

  d = (*func)(strm, data);
  strm_emit(strm, d, NULL);
}

strm_stream*
strm_funcmap(strm_map_func func)
{
  return strm_alloc_stream(strm_task_filt, map_recv, NULL, func);
}

#include <ctype.h>

static strm_value
str_toupper(strm_stream *strm, strm_value p)
{
  struct strm_string *str = strm_value_str(p);
  const char *s, *send;
  char *t, *buf;

  s = str->ptr;
  send = s + str->len;
  buf = malloc(str->len);
  t = buf;
  while (s < send) {
    *t = toupper(*s);
    t++;
    s++;
  }
  return strm_str_value(buf, str->len);
}

struct seq_seeder {
  int n;
  int end;
};

static void
seq_seed(strm_stream *strm, strm_value data)
{
  struct seq_seeder *s = strm->data;
  struct strm_string *str;

  if (s->n > s->end)
    return;
  str = strm_str_new(0, 16);
  str->len = sprintf((char*)str->ptr, "%d", s->n);
  strm_emit(strm, strm_ptr_value(str), seq_seed);
  s->n++;
}

strm_stream*
strm_seq(int start, int end)
{
  struct seq_seeder *s = malloc(sizeof(struct seq_seeder));
  s->n = start;
  s->end = end;
  return strm_alloc_stream(strm_task_prod, seq_seed, NULL, (void*)s);
}

int
main(int argc, char **argv)
{
  strm_stream *strm_stdin = strm_readio(0 /* stdin*/);
  strm_stream *strm_map = strm_funcmap(str_toupper);
  strm_stream *strm_stdout = strm_writeio(1 /* stdout */);

  strm_connect(strm_stdin, strm_map);
  strm_connect(strm_map, strm_stdout);
  strm_loop();

  return 0;
}
