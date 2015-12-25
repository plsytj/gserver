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
        int event_poll(int timeout,poll_event * e, int max );
        void start();
        void stop();

        socket_t* handle_accept();
    public:
        /* event_poll ctl */
        void sp_add(int fd, void* ud);
        void sp_write(int fd, void* ud);
        void sp_del(int fd);
    private:
        void work();
    private:
        /*
        socket_t *  accept_event();
        bool    in_event(socket_t* conn);
        bool    out_event(socket_t* conn);
        */
    private:
        int     epoll_fd;
        int     listen_fd;
};

#endif
