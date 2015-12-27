#ifndef __GTW_INCLUDE__
#define __GTW_INCLUDE__
#include "socket_server.h"

class gtw
{
    public:
        gtw();
        ~gtw();
    public:
        bool    init(const char * host, const char* service);
        void    run();
        void    stop();
        bool    in_event(socket_t* conn);
        bool    out_event(socket_t* conn);
        void    accept_event();
        void    do_cmd(socket_t * conn, void * data, int len);
    private:
        socket_server ss_;
        std::map<uint64_t, socket_t*> conn_map;
        uint64_t sequence;
        bool run_flag;
};

#endif
