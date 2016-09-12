#include <signal.h>
#include "strm.h"

typedef void (*sighandler_t)(int);
struct sig_list {
  int sig;
  strm_sighandler_t func;
  void* arg;
  struct sig_list* next;
};

struct sig_list* sig_list;

static void
handler(int sig)
{
  struct sig_list* list;

  for (list=sig_list; list; list=list->next) {
    if (list->sig == sig) {
      (*list->func)(sig, list->arg);
    }
  }
}

static void
sigcall(int sig, void* f)
{
  (*(sighandler_t)f)(sig);
}

static int
add_sig(int sig, strm_sighandler_t func, void* arg)
{
  struct sig_list* node = malloc(sizeof(struct sig_list));

  if (node == NULL) return STRM_NG;
  node->next = sig_list;
  node->sig = sig;
  node->func = func;
  node->arg = arg;
  sig_list = node;
  return STRM_OK;
}

int
strm_signal(int sig, strm_sighandler_t func, void* arg)
{
  sighandler_t r = signal(sig, SIG_IGN);

  if (r == SIG_ERR) return STRM_NG;
  if (r && r != handler) {
    if (add_sig(sig, sigcall, (void*)r) == STRM_NG)
      return STRM_NG;
  }
  if (add_sig(sig, func, arg) == STRM_NG)
    return STRM_NG;
  r = signal(sig, handler);
  if (r == SIG_ERR) return STRM_NG;
  return STRM_OK;
}

int
strm_unsignal(int sig, strm_sighandler_t func)
{
  sighandler_t r = signal(sig, SIG_IGN);
  struct sig_list* list;
  struct sig_list* tmp = NULL;

  if (r == SIG_ERR) return STRM_NG;
  for (list=sig_list; list; list=list->next) {
    if (list->sig == sig) {
      if (tmp == NULL) {        /* head */
        sig_list = list->next;
      }
      else {
        tmp->next = list->next;
        tmp = list;
      }
    }
  }
  signal(sig, handler);
  return STRM_OK;
}
