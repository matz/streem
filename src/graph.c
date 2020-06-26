/* stream graph generator inspired by stag */
/* https://github.com/seenaburns/stag */
#include "strm.h"
#include "atomic.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

/* signal handling */
static int refcnt = 0;
static int winch = FALSE;
static int interrupt = FALSE;

static void
sigupdate(int sig, void* arg)
{
  int* var = (int*)arg;
  *var = TRUE;
}

static int
get_winsize(int* row, int* col)
{
  struct winsize w;
  int n;

  n = ioctl(1, TIOCGWINSZ, &w);
  if (n < 0 || w.ws_col == 0) {
    return STRM_NG;
  }
  *row = w.ws_row;
  *col = w.ws_col;
  return STRM_OK;
}

static void
move_cursor(int row, int col)
{
  printf("\x1b[%d;%dH", row, col);
}

static void
clear()
{
  printf("\x1b[2J");
}

static void
erase_cursor()
{
  printf("\x1b[?25l");
}

static void
show_cursor()
{
  printf("\x1b[?25h");
}

struct bar_data {
  const char *title;
  strm_int tlen;
  strm_int col, row;
  strm_int dlen, llen;
  strm_int offset;
  strm_int max;
  double* data;
};

static void
show_title(struct bar_data* d)
{
  int start;

  erase_cursor();
  clear();
  if (d->tlen == 0) return;
  start = (d->col - d->tlen) / 2;
  move_cursor(1, start);
  fwrite(d->title, d->tlen, 1, stdout);
}

static void
show_yaxis(struct bar_data* d)
{
  move_cursor(1,2);
  printf("\x1b[0m");
  for (int i=0; i<d->llen; i++) {
    move_cursor(i+2, d->dlen+1);
    if (i == 0) {
      printf("├ %d   ", d->max);
    }
    else if (i == d->llen-1) {
      printf("├ 0");
    }
    else {
      printf("|");
    }
  }
}

static void
show_bar(struct bar_data* d, int i, int n)
{
  double f = d->data[i] / d->max * d->llen;

  for (int line=0; line<d->llen; line++) {
    move_cursor(d->llen+1-line, n);
    if (line < f) {
      printf("\x1b[7m ");
    }
    else if (line == 0) {
      printf("\x1b[0m_");
    }
    else {
      printf("\x1b[0m ");
    }
  }
}

static void
show_graph(struct bar_data* d)
{
  int n = 1;
  
  show_yaxis(d);
  for (int i=d->offset; i<d->dlen; i++) {
    show_bar(d, i, n++);
  }
  for (int i=0; i<d->offset; i++) {
    show_bar(d, i, n++);
  }
}

static int
init_bar(struct bar_data* d)
{
  if (get_winsize(&d->row, &d->col))
    return STRM_NG;
  d->max = 1;
  d->offset = 0;
  d->dlen = d->col-6;
  d->llen = d->row-3;
  d->data = malloc((d->dlen)*sizeof(double));
  for (int i=0;i<d->dlen;i++) {
    d->data[i] = 0;
  }
  show_title(d);
  return STRM_OK;
}

static int
iter_bar(strm_stream* strm, strm_value data)
{
  struct bar_data* d = strm->data;
  double f, max = 1.0;

  if (interrupt) {
    interrupt = FALSE;
    strm_unsignal(SIGINT, sigupdate);
    move_cursor(d->row-1, 1);
    show_cursor();
    exit(1);
  }
  if (!strm_number_p(data)) {
    strm_raise(strm, "invalid data");
    return STRM_NG;
  }

  if (winch) {
    winch = FALSE;
    free(d->data);
    if (init_bar(d) == STRM_NG) {
      strm_stream_close(strm);
      return STRM_NG;
    }
  }
  f = strm_value_float(data);
  if (f < 0) f = 0;
  d->data[d->offset++] = f;
  max = 1.0;
  for (int i=0; i<d->dlen; i++) {
    f = d->data[i];
    if (f > max) max = f;
  }
  d->max = max;
  if (d->offset == d->dlen) {
    d->offset = 0;
  }
  show_graph(d);
  return STRM_OK;
}

static int
fin_bar(strm_stream* strm, strm_value data)
{
  struct bar_data* d = strm->data;

  move_cursor(d->row, 1);
  if (d->title) free((void*)d->title);
  free(d->data);
  free(d);
  show_cursor();
  strm_atomic_inc(refcnt);
  if (refcnt <= 0) {
    strm_unsignal(SIGWINCH, sigupdate);
    strm_unsignal(SIGINT, sigupdate);
  }
  return STRM_OK;
}

static int
exec_bgraph(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
  struct bar_data* d;
  char* title = NULL;
  strm_int tlen = 0;

  strm_get_args(strm, argc, args, "|s", &title, &tlen);
  d = malloc(sizeof(struct bar_data));
  if (!d) return STRM_NG;
  d->title = malloc(tlen);
  memcpy((void*)d->title, title, tlen);
  d->tlen = tlen;
  if (refcnt == 0) {
    strm_atomic_inc(refcnt);
    strm_signal(SIGWINCH, sigupdate, &winch);
    strm_signal(SIGINT, sigupdate, &interrupt);
  }
  if (init_bar(d) == STRM_NG) return STRM_NG;
  *ret = strm_stream_value(strm_stream_new(strm_consumer, iter_bar,
                                           fin_bar, (void*)d));
  return STRM_OK;
}
#endif

void
strm_graph_init(strm_state* state)
{
#ifndef _WIN32
  strm_var_def(state, "graph_bar", strm_cfunc_value(exec_bgraph));
#endif
}
