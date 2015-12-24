#include "socket_server.h"
#include <errno.h>

socket_server::socket_server()
    : listen_fd(-1)
    , epoll_fd(-1)
    , sequence_(0)
    , run(false)
{
}

socket_server::~socket_server()
{
    if (epoll_fd != -1)
        close(epoll_fd);
    if (listen_fd != -1)
        close(listen_fd);
}
void socket_server::start()
{
    run = true;
    thread_ = std::thread(&socket_server::run, this);
}

void socket_server::stop()
{
    run = false;
}

void socket_server::work()
{
    while(run)
    {
        event_poll(10);
    }
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
        return false;
    }

    rc  = ::listen(listen_fd, SOMAXCONN);
    if ( rc < 0)
    {
        return false;
    }

    sp_add(listen_fd, NULL);
    return true;
}

void socket_server::sp_add(int fd, void* ud)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = ud;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void socket_server::sp_write(int fd, void* ud)
{
    struct epoll_event ev;
    ev.events = EPOLLIN |  EPOLLOUT | EPOLLET;
    ev.data.ptr = ud;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void socket_server::sp_del(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, NULL);
}


void socket_server::event_poll(int timeout)
{
    memset(events, 0, MAX_SERVER_ENENT * sizeof(epoll_event));
    int nfds = epoll_wait(epoll_fd, events, MAX_SERVER_ENENT, timeout);

    for (int i = 0; i < nfds; ++i)
    {
        if ((events[i].data.fd == listen_fd))
        {
            accept_event();
        }
        else
        {
            socket_t* conn = (socket_t*)events[i].data.ptr;
            if (!conn) continue;

            if (events[i].events & EPOLLERR)
            {
                conn->close();
                continue;
            }
            else
            {
                if (events[i].events & EPOLLOUT)
                    out_event(conn);
                if (events[i].events & EPOLLIN)
                    in_event(conn);
            }
        }
    }
}


bool socket_server::accept_event()
{
    sockaddr_in caddr;
    int addrlen = sizeof(caddr);
    memset(&caddr, 0,  sizeof(caddr));
    int cfd = accept(listen_fd, (struct sockaddr*)&caddr, (socklen_t*)&addrlen);
    if (-1 == cfd)
    {
        fprintf(stderr, "accept error:(%d)%s", errno, strerror(errno));
        return false;
    }

    socket_t* sock = new socket_t(cfd, caddr);
    sp_write(cfd, sock);

    conn_map[sequence_++] = sock;
    return true;
}

bool socket_server::out_event(socket_t* conn)
{
    int ret = conn->sendCmd();
    if (-1 == ret)
    {
        conn->close();
        return false;
    }
    else if (ret > 0)
    {
        sp_write(conn->get_fd(), conn );
    }
    return true;
}

bool socket_server::in_event(socket_t* conn)
{
    if (!conn->readToBuf())
    {
        conn->close();
        return false;
    }

    unsigned char* cmd = NULL;
    unsigned short len;
    while (conn->getCmd(cmd, len))
    {
        conn->putCmdToQueue(cmd, len);
        conn->popCmd();
    }
    return true;
}
