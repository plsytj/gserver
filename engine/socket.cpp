#include "socket.h"
#include <errno.h>
#include <fcntl.h>

#define COMPRESS_LEVEL 6
#define MIN_COMPRESS_SIZE 48

socket_t::socket_t(int sockfd, const sockaddr_in& addr)
{
    fd_ = sockfd;
    addr_ = addr;

    _write_buffer.Resize(MAX_BUFSIZE * 2);
    read_buf.Resize(MAX_BUFSIZE * 2);
    encBuffer.Resize(MAX_BUFSIZE);
    cmd_write_buf.Resize(MAX_BUFSIZE * 2);
    tmp_write_buf.Resize(MAX_BUFSIZE * 2);

    unCompCmdRealSize = 0;
    set_nonblock();
}

socket_t::~socket_t()
{
    close();
}

bool socket_t::connect(const char* ip, int port)
{
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0)
    {
        fprintf(stderr, "[Socket],connect() %s:%d socket failed %p", ip, port, this);
        return false;
    }
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip);
    addr_.sin_port = htons(port);

    int ret = ::connect(fd_, (sockaddr*)&addr_, sizeof(sockaddr_in));
    if (0 != ret)
    {
        fprintf(stderr, "[Socket],connect() %s:%d failed with error %d", ip, port, ret);
        return false;
    }
    return true;
}

void socket_t::close()
{
    if (valid())
    {
        if( !::close(fd_) )
        {
            perror("close: ");
        }
        fd_ = -1;
    }
}

void socket_t::shutdown(int how)
{
    if (valid())
    {
        ::shutdown(fd_, how);
    }
}

void socket_t::set_nonblock()
{
    int flags = fcntl(fd_, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (-1 == fcntl(fd_, F_SETFL, flags))
    {
        perror("fcntl: ");
    }
}

bool socket_t::read_cmd()
{
    bool final_ret = true;
    while (1)
    {
        if (read_buf.GetLeft() < MAX_BUFSIZE)
        {
            read_buf.Resize(read_buf.buffer_size() + MAX_BUFSIZE);
        }

        int ret = ::recv(fd_, read_buf.GetBufOffset(), MAX_BUFSIZE, 0);
        if (ret < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK )
            {
                fprintf(stderr, "[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
                final_ret = false;
            }
            break;
        }
        else if (0 == ret) //peer shutdown
        {
            fprintf(stderr, "socket:%d peer down", fd_);
            final_ret = false;
            break;
        }
        else
        {
            read_buf.Put(ret);
        }
    }
    return final_ret;
}

int socket_t::send_cmd()
{
    if (!valid()) return -1;

    pre_send_cmd();
    int final_ret = 0;
    int all = _write_buffer.buffer_offset();
    while (all)
    {
        int realsend = std::min(all, MAX_BUFSIZE * 2);
        int ret = ::send(fd_, _write_buffer.GetBufBegin(), realsend, 0);
        if (ret > 0)
        {
            _write_buffer.Pop(ret);
            all = _write_buffer.buffer_offset();
        }
        else if (ret == 0)
        {
            final_ret = _write_buffer.buffer_offset();
            fprintf(stderr, "[SOCKET],发送异常,fd:%d,ret:%d,real:%d", fd_, ret, realsend);
            break;
        }
        else
        {
            if (errno != EWOULDBLOCK || errno != EAGAIN)
            {
                fprintf(stderr, "[SOCKET],发送错误,fd:%d,ret:%d,real:%d,errno:%u,%s", fd_, ret, realsend, errno, strerror(errno));
                final_ret = -1;
                _write_buffer.Pop(all);
            }
            break;
        }
    }
    return final_ret;
}


bool socket_t::send_cmd(const void* data, uint16_t len)
{
    scoped_lock_t l(mutex);
    if (!valid()) return false;

    PacketHead ph;
    ph.len = len;
    cmd_write_buf.Put(&ph, PH_LEN);
    cmd_write_buf.Put(data, len);
    return true;
}

bool socket_t::writeToBuf(void* data, uint32_t len)
{
    uint32_t real_size = 0;

    real_size = _write_buffer.Put(data, len);

    return (real_size == len);
}

bool socket_t::get_cmd(unsigned char*& cmd, uint16_t& len)
{
    cmd = 0;
    len = 0;

    uint32_t used = read_buf.buffer_offset();
    if (used < MIN_PACKSIZE) return false;

    Packet* p = (Packet*)read_buf.GetBufBegin();
    uint16_t real_size = 0;

    real_size = p->getFullSize();

    if (real_size > used) return false;

    unCompCmdRealSize = real_size;

    cmd = p->data;
    len = p->ph.len;
    return true;
}

bool socket_t::pop_cmd()
{
    if (unCompCmdRealSize >= MIN_PACKSIZE && unCompCmdRealSize <= read_buf.buffer_offset())
    {
        if (read_buf.Pop(unCompCmdRealSize))
        {
            unCompCmdRealSize = 0;
            return true;
        }
    }

    return false;
}

void socket_t::pre_send_cmd()
{
    do
    {
        scoped_lock_t l(mutex);
        tmp_write_buf.Reset();
        tmp_write_buf.Copy(cmd_write_buf);
        cmd_write_buf.Reset();
    }
    while (0);

    while (tmp_write_buf.buffer_offset() > 0)
    {
        PacketHead* oldPHead = (PacketHead*)tmp_write_buf.GetBufBegin();
        encBuffer.Reset();
        PacketHead ph;
        encBuffer.Put(&ph, PH_LEN);
        encBuffer.Put(tmp_write_buf.GetBufOffset(PH_LEN), oldPHead->len);
        PacketHead* p = (PacketHead*)encBuffer.GetBufBegin();

        uint16_t real_size = oldPHead->len;
        p->len = real_size;
        real_size += PH_LEN;

        if (!writeToBuf(encBuffer.GetBufBegin(), real_size))
        {
            fprintf(stderr, "[socket_t],fd:%d,push cmd error",  fd_);
        }
        tmp_write_buf.Pop(oldPHead->len + PH_LEN);
    }
}
