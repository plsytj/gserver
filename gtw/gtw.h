#ifndef __GTW_INCLUDE__
#define __GTW_INCLUDE__
#include "socket_server.h"

class gtw
{
    public:
        gtw();
        ~gtw();
    public:
        bool init(const char * addr, int port);
        void run();
        void stop();
    private:
        socket_server server_;
};

#endif
