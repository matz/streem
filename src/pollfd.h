#ifdef __linux__
# include <sys/epoll.h>
#else

#include <stdint.h>

/* for FD_SETSIZE */
#ifdef _WIN32
# include <ws2tcpip.h>
#else
# include <sys/select.h>
#endif
 
#define EPOLL_CTL_ADD (1)
#define EPOLL_CTL_DEL (2)
#define EPOLL_CTL_MOD (3)
#define EPOLLIN  (0x001)
#define EPOLLOUT (0x004)
#define EPOLLERR (0x008)
#define EPOLLONESHOT (1u << 30)
 
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
