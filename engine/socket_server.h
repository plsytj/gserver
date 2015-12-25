#ifndef __SOCKET_SERVER_INCLUDE__
#define __SOCKET_SERVER_INCLUDE__

#include <thread>
#include <map>
#include "socket.h"

#define MAX_SERVER_ENENT 256

struct poll_event
{
    socket_t * sock;
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
        void event_poll(int timeout,poll_event * e, int max );
        void processmsg(const void* cmd, uint16_t len);
        void start();
        void stop();

    public:
        /* event_poll ctl */
        void sp_add(int fd, void* ud);
        void sp_write(int fd, void* ud);
        void sp_del(int fd);
    private:
        void work();
    private:
        bool    accept_event();
        bool    in_event(socket_t* conn);
        bool    out_event(socket_t* conn);
    private:
        int     epoll_fd;
        int     listen_fd;
        epoll_event events[MAX_SERVER_ENENT];
    private:
        std::map<uint64_t, socket_t*> conn_map;
        uint64_t sequence_;
        //std::thread thread_;
};

#endif
