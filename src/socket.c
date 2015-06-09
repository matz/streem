#ifndef _WIN32
# include <sys/fcntl.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <stdio.h>
# define closesocket(fd) close(fd)
#else
# include <ws2tcpip.h>
# include <mswsock.h>
# include <fcntl.h>
#endif

#include "strm.h"
#include "node.h"

struct socket_data {
  int fd;
  strm_state *strm;
};

static void
accept_cb(strm_task *strm, strm_value data)
{
  struct socket_data *sd = strm->data;
  struct sockaddr_in writer_addr;
  socklen_t writer_len;
  int sock;

  writer_len = sizeof(writer_addr);
  sock = accept(sd->fd, (struct sockaddr *)&writer_addr, &writer_len);
  if (sock < 0) {
    closesocket(sock);
    if (sd->strm->strm)
      strm_close(sd->strm->strm);
    node_raise(sd->strm, "socket error: listen");
    return;
  }

#ifdef _WIN32
  sock = _open_osfhandle(sock, 0);
#endif
  strm_emit(strm, strm_ptr_value(strm_io_new(sock, STRM_IO_READ|STRM_IO_WRITE|STRM_IO_FLUSH)), accept_cb);
}

static void
server_accept(strm_task *strm, strm_value data)
{
  struct socket_data *sd = strm->data;

  strm_io_start_read(strm, sd->fd, accept_cb);
}

static void
server_close(strm_task *strm, strm_value d)
{
  struct socket_data *sd = strm->data;

  closesocket(sd->fd);
}

static int
tcp_server(strm_state* strm, int argc, strm_value* args, strm_value *ret)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sock, s;
  const char *service;
  char buf[12];
  struct socket_data *sd;
  strm_task *task;

#ifdef _WIN32
  int sockopt = SO_SYNCHRONOUS_NONALERT;
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 0), &wsa);
  setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char *)&sockopt, sizeof(sockopt));
#endif

  if (argc != 1) {
    return 1;
  }
  if (strm_int_p(args[0])) {
    sprintf(buf, "%d", (int)strm_value_int(args[0]));
    service = buf;
  }
  else {
    volatile strm_string* str = strm_value_str(args[0]);
    service = str->ptr;
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;/* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(NULL, service, &hints, &result);
  if (s != 0) {
    node_raise(strm, gai_strerror(s));
    return 1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) continue;

    if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
      break;                    /* Success */
    closesocket(sock);
  }

  if (rp == NULL) {
    node_raise(strm, "socket error: bind");
    return 1;
  }
  freeaddrinfo(result);

  if (listen(sock, 5) < 0) {
    closesocket(sock);
    node_raise(strm, "socket error: listen");
    return 1;
  }

#ifdef _WIN32
  sock = _open_osfhandle(sock, 0);
#endif
  sd = malloc(sizeof(struct socket_data));
  sd->fd = sock;
  sd->strm = strm;
  task = strm_alloc_stream(strm_task_prod, server_accept, server_close, (void*)sd);
  *ret = strm_task_value(task);
  return 0;
}

static int
tcp_socket(strm_state* strm, int argc, strm_value* args, strm_value *ret)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sock, s;
  const char *service;
  char buf[12];
  strm_string* host;

#ifdef _WIN32
  int sockopt = SO_SYNCHRONOUS_NONALERT;
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 0), &wsa);
  setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char *)&sockopt, sizeof(sockopt));
#endif

  if (argc != 2) {
    return 1;
  }
  host = strm_value_str(args[0]);
  if (strm_int_p(args[1])) {
    sprintf(buf, "%d", (int)strm_value_int(args[1]));
    service = buf;
  }
  else {
    volatile strm_string* str = strm_value_str(args[1]);
    service = str->ptr;
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */
  s = getaddrinfo(host->ptr, service, &hints, &result);

  if (s != 0) {
    node_raise(strm, gai_strerror(s));
    return 1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) continue;

    if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                    /* Success */
    closesocket(sock);
  }

  if (rp == NULL) {
    node_raise(strm, "socket error: connect");
    return 1;
  }
  freeaddrinfo(result);
#ifdef _WIN32
  sock = _open_osfhandle(sock, 0);
#endif
  *ret = strm_ptr_value(strm_io_new(sock, STRM_IO_READ|STRM_IO_WRITE|STRM_IO_FLUSH));
  return 0;
}

void
strm_socket_init(strm_state* strm)
{
  strm_var_def("tcp_server", strm_cfunc_value(tcp_server));
  strm_var_def("tcp_socket", strm_cfunc_value(tcp_socket));
}
