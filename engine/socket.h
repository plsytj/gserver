#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "bytebuff.h"
#include "mutex.hpp"

// 包头标志
enum PACKET_FLAG_ENUM_TYPE
{
    PACKET_FLAG_COMPRESS = 1,  // 压缩
    PACKET_FLAG_ENCRYPT = 2,  // 加密
};

struct PacketHead
{
    //unsigned char flags;
    uint32_t len;
    PacketHead()
    {
        //flags = 0;
        len = 0;
    }
};

struct Packet
{
    PacketHead ph;
    unsigned char data[0];

    uint16_t getDataSize() { return ph.len; }
    uint16_t getFullSize() { return PH_LEN + ph.len; }
};


class socket_t
{
    public:
        socket_t(int fd, const sockaddr_in& addr);
        ~socket_t();
    public:
        int get_fd() const {return fd_;}
        bool valid() {return fd_ != -1;}
        void shutdown(int how);
        bool connect(const char* ip, int port);
        void close();

        void set_nonblock();

        bool get_cmd(unsigned char*& cmd, uint16_t& len);
        bool pop_cmd();
        bool send_cmd(const void* data, uint16_t len);
        int send_cmd();
        void pre_send_cmd();

        bool read_cmd();
        bool writeToBuf(void* data, uint32_t len);

        uint16_t sizeMod8(uint16_t len);
    protected:
        int fd_;
        sockaddr_in addr_;
    public:
        const sockaddr_in& get_addr() const
        {
            return addr_;
        }
        
    private:
        mutex_t mutex;

        bytebuff read_buf;
        bytebuff cmd_write_buf;
        bytebuff tmp_write_buf;
        bytebuff _write_buffer;

        bytebuff encBuffer;

        uint32_t unCompCmdRealSize;//未解压的消息长度

        int id_;

};
