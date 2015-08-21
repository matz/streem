#ifndef __linux__
#include "pollfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <unistd.h>

#if defined(__APPLE__) || defined(__FreeBSD__)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int
epoll_create(int size)
{
  return kqueue();
}

int
epoll_ctl(int epfd, int op, int fd, struct epoll_event* event)
{
  struct kevent kev = {0};
  struct timespec tv = {0};

  kev.ident = fd;
  kev.filter = EVFILT_READ;
  if (event) {
    if (event->events & EPOLLONESHOT) {
      kev.flags = EV_ONESHOT;
    }
    kev.udata = event->data.ptr;
  }
  switch (op) {
  case EPOLL_CTL_ADD:
  case EPOLL_CTL_MOD:
    kev.flags |= EV_ADD;
    break;
  case EPOLL_CTL_DEL:
    kev.flags |= EV_DELETE;
    break;
  }
  return kevent(epfd, &kev, 1, &kev, 1, &tv);
}

#define MAX_KEVENTS 10

int
epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
  struct kevent kev[MAX_KEVENTS] = {{0}};
  struct timespec tv, *tp;
  int i, n;

  if (maxevents > MAX_KEVENTS)
    maxevents = MAX_KEVENTS;
  if (timeout < 0) tp = NULL;
  else {
    tv.tv_sec = timeout/1000;
    tv.tv_nsec = (timeout%1000)*1000000;
    tp = &tv;
  }
  n = kevent(epfd, NULL, 0, kev, maxevents, tp);
  if (n >= 0) {
    for (i=0; i<n; i++) {
      events[i].data.ptr = kev[i].udata;
    }
  }
  return n;
}

#else

#ifndef _WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/time.h>
#endif

#define MAX_EPOLL 1

static struct epoll_event *_epoll_events = NULL;
static int _num_epoll_events = 0;

int
epoll_create(int size)
{
  int i, num = _num_epoll_events++;
#ifdef _WIN32
  WSADATA wsa;
  if (num == 0) WSAStartup(MAKEWORD(2, 0), &wsa);
#endif
  _epoll_events = realloc(_epoll_events, sizeof(struct epoll_event) * _num_epoll_events);
  memset(&_epoll_events[num], 0, sizeof(struct epoll_event));
  for (i = 0; i < FD_SETSIZE; i++) _epoll_events[num].fds[i] = -1;
  return num;
}

int
epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  struct epoll_event* ee = &_epoll_events[epfd];
  int i;
  switch (op) {
  case EPOLL_CTL_ADD:
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] == fd) return 0;
      if (ee->fds[i] == -1) {
        ee->events = event->events;
        ee->data = event->data;
        ee->fds[i] = fd;
        return 0;
      }
    }
    break;
  case EPOLL_CTL_DEL:
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] == fd) {
        ee->fds[i] = -1;
        return 0;
      }
    }
    break;
  case EPOLL_CTL_MOD:
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] == fd) {
        ee->events = event->events;
        ee->data = event->data;
        return 0;
      }
    }
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] == -1) {
        ee->events = event->events;
        ee->data = event->data;
        ee->fds[i] = fd;
        return 0;
      }
    }
    break;
  }
  return 1;
}

#ifdef _WIN32
int
epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
  struct epoll_event* ee;
  DWORD ct;
  FD_SET fdset[3];
  WSANETWORKEVENTS wev;
  ee = &_epoll_events[epfd];
  struct timeval tv, *tp;
  if (!ee) return -1;
  if (timeout < 0) tp = NULL;
  else {
    tv.tv_sec = timeout/1000;
    tv.tv_usec = (timeout%1000)*1000000;
    tp = &tv;
  }
  ct = GetTickCount();
  while (GetTickCount() - ct < timeout) {
    int i, e = 0;
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] < 0) continue;
      if (WSAEnumNetworkEvents((SOCKET) ee->fds[i], NULL, &wev) == 0) {
        FD_ZERO(&fdset[0]);
        FD_ZERO(&fdset[1]);
        FD_ZERO(&fdset[2]);
        if (ee->events & EPOLLIN) FD_SET(ee->fds[i], &fdset[0]);
        if (ee->events & EPOLLOUT) FD_SET(ee->fds[i], &fdset[1]);
        if (ee->events & EPOLLERR) FD_SET(ee->fds[i], &fdset[2]);
        if (select(1, &fdset[0], &fdset[1], &fdset[2], tp) > 0 &&
            (FD_ISSET(ee->fds[i], &fdset[0]) ||
             FD_ISSET(ee->fds[i], &fdset[1]) ||
             FD_ISSET(ee->fds[i], &fdset[2]))) {
          memcpy(&events[e++], ee, sizeof(struct epoll_event));
          if (ee->events & EPOLLONESHOT)
            ee->fds[i] = -1;
        }
      } else {
        HANDLE h = (HANDLE) _get_osfhandle(ee->fds[i]);
        if (h != INVALID_HANDLE_VALUE && WaitForSingleObject(h, timeout < 0 ? INFINITE : timeout) == WAIT_OBJECT_0) {
          memcpy(&events[e++], ee, sizeof(struct epoll_event));
          if (ee->events & EPOLLONESHOT)
            ee->fds[i] = -1;
        }
      }
      if (e >= maxevents) break;
    }
    if (e > 0) return e;
    if (timeout == -1) break;
  }
  return 0;
}
#else

unsigned long current_time()
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0)
    return 0;
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

int
epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
  struct epoll_event* ee;
  unsigned long ct;
  fd_set fdset[3];
  struct timeval tv, *tp;
  ee = &_epoll_events[epfd];
  if (!ee) return -1;
  if (timeout < 0) tp = NULL;
  else {
    tv.tv_sec = timeout/1000;
    tv.tv_usec = (timeout%1000)*1000000;
    tp = &tv;
  }
  ct = current_time();
  while (current_time() - ct < timeout) {
    int i, e = 0;
    for (i = 0; i < FD_SETSIZE; i++) {
      if (ee->fds[i] < 0) continue;
      FD_ZERO(&fdset[0]);
      FD_ZERO(&fdset[1]);
      FD_ZERO(&fdset[2]);
      if (ee->events & EPOLLIN) FD_SET(ee->fds[i], &fdset[0]);
      if (ee->events & EPOLLOUT) FD_SET(ee->fds[i], &fdset[1]);
      if (ee->events & EPOLLERR) FD_SET(ee->fds[i], &fdset[2]);
      if (select(1, &fdset[0], &fdset[1], &fdset[2], tp) > 0 &&
          (FD_ISSET(ee->fds[i], &fdset[0]) ||
           FD_ISSET(ee->fds[i], &fdset[1]) ||
           FD_ISSET(ee->fds[i], &fdset[2]))) {
        memcpy(&events[e++], ee, sizeof(struct epoll_event));
        if (ee->events & EPOLLONESHOT)
          ee->fds[i] = -1;
      }
      if (e >= maxevents) break;
    }
    if (e > 0) return e;
    if (timeout == -1) break;
  }
  return 0;
}
#endif /* _WIN32 */
#endif /* __APPLE__ || __FreeBSD__ */
#endif /* not __linux__ */
