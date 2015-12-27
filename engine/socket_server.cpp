#include "socket_server.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "wrapsocket.h"

socket_server::socket_server()
    : listen_fd(-1)
    , epoll_fd(-1)
{
}

socket_server::~socket_server()
{
    if (epoll_fd != -1)
        close(epoll_fd);
    if (listen_fd != -1)
        close(listen_fd);
}
    


bool socket_server::open(const char * host, const char * serv, socklen_t *addrlenp)
{
    epoll_fd = epoll_create(256);
    if ( epoll_fd < 0)
    {
        return false;
    }
    
    listen_fd = tcp_listen(host, serv, addrlenp);
    if(listen_fd < 0)
    {
        return false;
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR;
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    return true;
}

void socket_server::poll_add(int fd, void* ud)
{
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.ptr = ud;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void socket_server::poll_write(int fd, void* ud)
{
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.ptr = ud;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void socket_server::poll_del(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, NULL);
}

int socket_server::event_poll(int timeout, poll_event * e, int max)
{
    epoll_event events[max];
    memset(events, 0, sizeof(epoll_event)*max);
    int nfds = epoll_wait(epoll_fd, events, max, timeout);
    for (int i = 0; i < nfds; ++i)
    {
        if ((events[i].data.fd == listen_fd))
        {
            e[i].sock = NULL;
            e[i].accept = true;
        }
        else
        {
            e[i].sock = (socket_t*)events[i].data.ptr;
            unsigned int flag = events[i].events;

            e[i].read = (flag & EPOLLIN) != 0;
            e[i].write = (flag & EPOLLOUT) != 0;
        }
    }
    return nfds;
}


socket_t* socket_server::handle_accept()
{
    union sockaddr_all caddr;
    socklen_t addrlen = sizeof(caddr);
    memset(&caddr, 0,  sizeof(caddr));
    int cfd = accept(listen_fd, (struct sockaddr*)&caddr.s, &addrlen);
    if (cfd < 0)
    {
        fprintf(stderr, "accept error:(%d)%s", errno, strerror(errno));
        return NULL; 
    }
    void * sin_addr = (caddr.s.sa_family == AF_INET) ? (void*)&caddr.v4.sin_addr : (void *)&caddr.v6.sin6_addr;
    int sin_port = ntohs((caddr.s.sa_family == AF_INET) ? caddr.v4.sin_port : caddr.v6.sin6_port);
    char tmp[INET6_ADDRSTRLEN];
    if (inet_ntop(caddr.s.sa_family, sin_addr, tmp, sizeof(tmp))) {
        printf("connection from %s:%d\n", tmp, sin_port);
    }    
    else {
        perror("inet_ntop: ");
    }
    socket_t* sock = new socket_t();
    sock->init(cfd, caddr.s);
    sock->set_nonblock();

    poll_add(cfd, sock);

    return sock;
}

