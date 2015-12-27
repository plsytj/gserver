#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "gtw.h"


gtw * server = NULL;

void stop_server(int ){
    if(server)
        server->stop();
}

int main(int argc, char* argv[]) {

    if(argc < 2) {
        printf("error args:port");
        return -1;
    }

    server = new gtw();

    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

    server->init("0.0.0.0", argv[1]);
    server->run();

    return 0;
}
