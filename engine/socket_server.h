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

class socket_server
{
    public:
        socket_server();
        ~socket_server();
    public:
        bool listen(const char* addr, int port);
        socket_t* handle_accept();
    public:
        /* event_poll ctl */
        int event_poll(int timeout,poll_event * e, int max );
        void poll_add(int fd, void* ud);
        void poll_write(int fd, void* ud);
        void poll_del(int fd);
    private:
        int     epoll_fd;
        int     listen_fd;
};

#endif
