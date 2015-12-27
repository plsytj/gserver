#include <stdio.h>
#include <thread>
#include "socket.h"
#include <stdint.h>

char host[256];
char serv[256];

int thread_fun(int id)
{
    socket_t sock;
    char msg[256] = {0};
    sprintf(msg, "thread %d send msg.", id);

    if( sock.connect(host, serv)) {
        printf("thread %d connected\n", id);
    }
    else {
        return -1;
    }

    for(;;) {
        sock.send_cmd(msg, strlen(msg)+1);
        if( sock.send_cmd() < 0)
        {
            break;
        }
        sock.read_cmd();
        unsigned char * cmd;
        uint16_t len;
        if( !sock.get_cmd(cmd, len))
        {
            sock.pop_cmd();
        }
        else
        {
            break;
        }
    }
    sock.close();
    printf("thread %d stop", id);
}

int main(int argc, char **argv)
{
    int thread_num;
    if(argc < 3)
    {
        printf("usage, host, port");
        return -1;
    }
    strncpy(host, argv[1], sizeof(host));
    strncpy(serv, argv[2], sizeof(serv));
    printf("enter thread num:\n");
    
    scanf("%d", &thread_num);

    std::thread t[thread_num];
    for(int i=0; i<thread_num; i++)
    {
        t[i] = std::thread(thread_fun, i);
    }
    for(int i=0; i<thread_num; i++)
    {
        t[i].join();
    }
    return 0;
}
