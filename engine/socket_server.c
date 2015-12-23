#include "socket_server.h"

#define MAX_SERVER_ENENT 256
socket_server::socket_server()
    : listen_fd(-1)
    , epfd(-1)
{
}

socket_server::~socket_server()
{
    if (epfd != -1)
        close(epfd);
    if (listen_fd != -1)
        close(listen_fd);
}

bool socket_server::listen(const char* addr, int port)
{
    sockaddr_in localaddr;
    unsigned long uladdr = INADDR_ANY;

    epfd = epoll_create(256);
    if ( epfd < 0)
    {
        return false;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_fd == -1)
    {
        return false;
    }

    if ( addr && *addr != 0)
        uladdr = inet_addr(szAddress);

    if ( uladdr == INADDR_NONE )
        uladdr = INADDR_ANY;

    localaddr.sin_family		= AF_INET;
    localaddr.sin_addr.s_addr	= uladdr;
    localaddr.sin_port			= htons(port);

    rc = bind(listen_fd, (struct sockaddr*)&localaddr, sizeof(sockaddr_in));
    if (rc < 0)
    {
        return false;
    }

    rc  = listen(listen_fd, SOMAXCONN);
    if ( rc < 0)
    {
        return false;
    }
    port_ = port;
    struct epoll_event ev;
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN | EPOLLERR;
    epoll_ctl(listen_epfd, EPOLL_CTL_ADD, listen_sock, &ev);
    return true;
}

void socket_server::poll(int epfd, int sock, epoll_event events[])
{
    if (!epfd) return;

    bzero(events, MAX_SERVER_EVENT * sizeof(epoll_event));
    int nfds = epoll_wait(epfd, events, MAX_SERVER_EVENT, EPOLL_WAIT_TIMEOUT);
    for (int i = 0; i < nfds; ++i)
    {
        if ((events[i].data.fd == sock))
        {
            sockaddr_in caddr;
            int addrlen = sizeof(caddr);
            bzero(&caddr, sizeof(caddr));
            int cfd = ::accept(sock, (struct sockaddr*)&caddr, (socklen_t*)&addrlen);
            if (-1 == cfd){
                sleep(1);
            }
            else{
                accept(cfd, caddr, epfd);
            }
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
                {
                    int ret = conn->realSendCmd();
                    if (-1 == ret)
                    {
                        conn->close();
                        continue;
                    }
                    else if (ret > 0)
                    {
                        conn->addEpoll();
                    }
                }
                if (events[i].events & EPOLLIN)
                {
                    if (!conn->readCmdFromSocket())
                    {
                        conn->close();
                        continue;
                    }

                    unsigned char* cmd = NULL;
                    unsigned short cmdLen;
                    while (conn->getCmdFromSocketBuf(cmd, cmdLen))
                    {

                    }
                }
            }
        }
    }
}

