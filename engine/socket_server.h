#ifndef __SOCKET_SERVER_INCLUDE__
#define __SOCKET_SERVER_INCLUDE__

#include <thread>
#include <map>
#include "socket.h"

#define MAX_SERVER_ENENT 256

struct poll_event
{
    socket_t * sock;
    bool accept;
    bool read;
    bool write;
};

union sockaddr_all {
    struct sockaddr s;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
};

class socket_server
{
    public:
        socket_server();
        ~socket_server();
    public:
        bool open(const char * host, const char * serv, socklen_t *addrlenp);
        socket_t* handle_accept();
    public:
        /* event_poll ctl */
        int event_poll(int timeout,poll_event * e, int max );
        void poll_add(int fd, void* ud);
        void poll_write(int fd, void* ud, bool enable);
        void poll_del(int fd);
    private:
        int     epoll_fd;
        int     listen_fd;
};

#endif
