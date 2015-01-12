#include "strm.h"

static void
map_recv(strm_stream *strm, void *data)
{
  strm_map_func func = strm->data;
  void *d;

  d = (*func)(strm, data);
  strm_emit(strm, d, NULL);
}

strm_stream*
strm_funcmap(void *(*func)(strm_stream *, void*))
{
  return strm_alloc_stream(strm_task_filt, map_recv, NULL, func);
}

#include <ctype.h>

static 
void*
str_toupper(strm_stream *strm, void *p)
{
  const char *s = p;
  char *t, *buf;

  buf = malloc(strlen(s)+1);
  t = buf;
  while (*s) {
    *t = toupper(*s);
    t++;
    s++;
  }
  *t = '\0';
  return (void*)buf;
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
