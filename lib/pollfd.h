#ifndef _WIN32
#include <sys/epoll.h>
#else
 
#include <ws2tcpip.h>
#include <stdint.h>
 
#define EPOLL_CTL_ADD 0
#define EPOLL_CTL_DEL 1
#define EPOLL_CTL_MOD 2
#define EPOLLIN 1
#define EPOLLOUT 2
#define EPOLLERR 4
#define EPOLLONESHOT 8
 
typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
 
struct epoll_event {
    uint32_t events;
    epoll_data_t data;
    int fds[FD_SETSIZE];
};
 
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
#endif
