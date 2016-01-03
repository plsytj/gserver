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
    uint32_t len;
    PacketHead()
    {
        len = 0;
    }
}__attribute__((packed));

struct Packet
{
    PacketHead ph;
    unsigned char data[0];

    uint32_t getDataSize() { return ph.len; }
    uint32_t getFullSize() { return sizeof(Packet) + ph.len; }
}__attribute__((packed));


class socket_t
{
    public:
        socket_t();
        ~socket_t();
    public:
        void init(int fd, const sockaddr&  addr);
        int get_fd() const {return fd_;}
        bool valid() {return fd_ != -1;}
        void shutdown(int how);
        bool connect(const char* host, const char * serv);
        void close();
        void set_nonblock();
        bool get_cmd(unsigned char*& cmd, uint16_t& len);
        bool pop_cmd();
        bool send_cmd(const void* data, uint16_t len);
        int send_cmd();
        void pre_send_cmd();
        bool send_buffer_empty();

        bool block_read();
        bool read_cmd();
        bool writeToBuf(void* data, uint32_t len);

    protected:
        int fd_;
        struct sockaddr addr_;
    public:
        const sockaddr& get_addr() const
        {
            return addr_;
        }
        
    private:
        mutex_t mutex;

        bytebuff read_buf;
        bytebuff cmd_write_buf;
        bytebuff tmp_write_buf;
        bytebuff send_buf;

        bytebuff encBuffer;

        uint32_t unCompCmdRealSize;//未解压的消息长度

        int id_;

};
