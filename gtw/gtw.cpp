#include "gtw.h"

bool gtw::init(const char * addr, int port)
{
    server_.listen(addr, port);

    server_.start();
}

void run()
{
    while(server_.eve

}


