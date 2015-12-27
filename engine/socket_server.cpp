#include "socket_server.h"
#include <errno.h>

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

bool socket_server::listen(const char* addr, int port)
{
    sockaddr_in localaddr;
    unsigned long uladdr;
    bool rc;

    epoll_fd = epoll_create(256);
    if ( epoll_fd < 0)
    {
        return false;
    }
    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_fd == -1)
    {
        perror("socket: ");
        return false;
    }

    if ( addr && *addr != 0) 
        uladdr = inet_addr(addr);
    else
        uladdr = INADDR_ANY;

    localaddr.sin_family		= AF_INET;
    localaddr.sin_addr.s_addr	= uladdr;
    localaddr.sin_port			= htons(port);

    rc = bind(listen_fd, (struct sockaddr*)&localaddr, sizeof(sockaddr_in));
    if (rc < 0)
    {
        perror("bind: ");
        return false;
    }

    rc  = ::listen(listen_fd, SOMAXCONN);
    if ( rc < 0)
    {
        perror("listen: ");
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
    sockaddr_in caddr;
    int addrlen = sizeof(caddr);
    memset(&caddr, 0,  sizeof(caddr));
    int cfd = accept(listen_fd, (struct sockaddr*)&caddr, (socklen_t*)&addrlen);
    if (-1 == cfd)
    {
        fprintf(stderr, "accept error:(%d)%s", errno, strerror(errno));
        return NULL; 
    }

    socket_t* sock = new socket_t(cfd, caddr);
    poll_add(cfd, sock);

    return sock;
}

