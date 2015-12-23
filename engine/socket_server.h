#ifndef __SOCKET_SERVER_INCLUDE__
#define __SOCKET_SERVER_INCLUDE__

class socket_server
{
    public:
        socket_server();
        ~socket_server();
    public:
        bool listen(const char * addr, int port);
        void poll();
        bool accept(int cfd, struct sockaddr_in * addr);
    private:
        socket_t sock_;
        int epfd_;
        int listen_fd;
        int port_;
};

#endif
