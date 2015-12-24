#ifndef __GTW_INCLUDE__
#define __GTW_INCLUDE__

class gtw
{
    public:
        gtw();
        ~gtw();
    public:
        bool init(int port);
        void run();
        void stop();
    private:
        socket_server server_;
};

#endif
