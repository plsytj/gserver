#include <stdio.h>
#include "gtw.h"


gtw * server = NULL;

void stop_server(){
    if(server)
        server->stop();
}

int main(int argc, char* argv[]) {

    if(argc < 2) {
        printf("error args:port");
        return -1;
    }

    int port = atoi(argv[1]);

    server = new gtw();

    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

    server->init(port);
    server->run();

    return 0;
}