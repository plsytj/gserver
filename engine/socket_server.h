#ifndef __SOCKET_SERVER_INCLUDE__
#define __SOCKET_SERVER_INCLUDE__
#include "socket.h"

class socket_server
{
    public:
        socket_server();
        ~socket_server();
    public:
        bool listen(const char* addr, int port);
        void poll(int timeout);
        void processmsg(const void* cmd, uint16_t len);
        /* epoll ctl */
        void sp_add(int fd, void* ud);
        void sp_write(int fd, void* ud, bool enable);
        void sp_del(int fd);

    private:
        bool accept_event();
    private:
        int         epfd_;
        int         listen_fd;
        int         port_;
    private:
        std::map<uint64_t, socket_t*> conn_map;
        uint64_t sequence_;
};

#endif
