#include "gtw.h"

gtw::gtw()
:sequence(0)
,run_flag(false)
{
}
gtw::~gtw()
{
}
bool gtw::init(const char * addr, int port)
{
    if( !ss_.listen(addr, port))
    {
        fprintf(stderr, "error listen on %s:%d\n", addr, port);
        return false;
    }
}

void gtw::run() {
    run_flag = true;
    while(run_flag) {
        poll_event e[256];
        memset(e, 0, sizeof(poll_event)*256);
        int num = ss_.event_poll(10, e, 256);

        for(int i=0; i<num; ++i) {

            if(e[i].accept)
                accept_event();

            socket_t * s = e[i].sock;
            if(s == NULL) continue;
            int error;
            socklen_t len = sizeof(error);  
            int code = getsockopt(s->get_fd(), SOL_SOCKET, SO_ERROR, &error, &len);  
            if(code < 0)
                printf("error..%d\n", error);

            if(e[i].read)
                in_event(s);
            if(e[i].write)
                out_event(s);
        }
    }
    printf("gtw stop.");
}

void gtw::stop()
{
    run_flag = false;
}

void gtw::accept_event()
{
    socket_t * sock = ss_.handle_accept();
    if(sock == NULL) return;
    const sockaddr_in & addr = sock->get_addr();

    char buf[256];
    inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));

    printf("new client:%s:%d\n", buf, addr.sin_port);
    conn_map[sequence++] = sock;
}

bool gtw::in_event(socket_t* conn)
{
    if(conn == NULL) return false;
    unsigned char* cmd = NULL;
    unsigned short len;

	if( !conn->read_cmd())
    {
        printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~peer down\n");
        ss_.poll_del(conn->get_fd());
        conn->close();
        return false;
   	}

    while (conn->get_cmd(cmd, len))
    {
		conn->pop_cmd();
        do_cmd(conn, cmd, len);
    }
    return true;
}

bool gtw::out_event(socket_t* conn)
{
    if(conn == NULL) return false;
    int ret = conn->send_cmd();
    if (-1 == ret )
    {
        ss_.poll_del(conn->get_fd());
        conn->close();
        return false;
    }
    else if (ret > 0)
    {
        //ss_.poll_write(conn->get_fd(), conn);
    }
    return true;
}

void gtw::do_cmd(socket_t * conn, void * data, int len)
{
    printf("recv client:(%d)%s\n", len, (char*)data);
    conn->send_cmd(data, len);
}
