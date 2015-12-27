#ifndef __WRAP_SOCKET_INCLUDE__
#include <sys/types.h>
#include <sys/socket.h>


int tcp_connect(const char *host, const char *serv, sockaddr * addr);
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);

#endif

