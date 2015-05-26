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
  node_ctx *ctx;
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
    if (sd->ctx->strm)
      strm_close(sd->ctx->strm);
    node_raise(sd->ctx, "socket error: listen");
    return;
  }

#ifdef _WIN32
  sock = _open_osfhandle(sock, _O_RDWR),
#endif
  strm_emit(strm, strm_ptr_value(strm_io_new(sock, STRM_IO_READ | STRM_IO_WRITE)), accept_cb);
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
exec_tcp_server(node_ctx* ctx, int argc, strm_value* args, strm_value *ret)
{
  struct sockaddr_in reader_addr; 
  int sock;
  int port;
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
  port = strm_value_int(args[0]);

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    node_raise(ctx, "socket error: socket");
    return 1;
  }

  memset((char *) &reader_addr, 0, sizeof(reader_addr));
  reader_addr.sin_family = PF_INET;
  reader_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  reader_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *)&reader_addr, sizeof(reader_addr)) < 0) {
    node_raise(ctx, "socket error: bind");
    return 1;
  }

  if (listen(sock, 5) < 0) {
    close(sock);
    node_raise(ctx, "socket error: listen");
    return 1;
  }

  sd = malloc(sizeof(struct socket_data));
  sd->fd = sock;
  sd->ctx = ctx;
  task = strm_alloc_stream(strm_task_prod, server_accept, server_close, (void*)sd);
  *ret = strm_task_value(task);
  return 0;
}

static int
exec_tcp_socket(node_ctx* ctx, int argc, strm_value* args, strm_value *ret)
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
    node_raise(ctx, gai_strerror(s));
    return 1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) continue;

    if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                    /* Success */
    close(sock);
  }

  if (rp == NULL) {
    node_raise(ctx, "socket error: connect");
    return 1;
  }
  freeaddrinfo(result);
  *ret = strm_ptr_value(strm_io_new(sock, STRM_IO_READ|STRM_IO_WRITE));
  return 0;
}

void
strm_socket_init(node_ctx* ctx)
{
  strm_var_def("tcp_server", strm_cfunc_value(exec_tcp_server));
  strm_var_def("tcp_socket", strm_cfunc_value(exec_tcp_socket));
}
