#pragma once
#include "bytebuff.h"
#include "xCmdQueue.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

// 包头标志
enum PACKET_FLAG_ENUM_TYPE
{
  PACKET_FLAG_COMPRESS = 1,  // 压缩
  PACKET_FLAG_ENCRYPT = 2,  // 加密
};

struct PacketHead
{
  unsigned char flags;
  uint16_t len;
  PacketHead()
  {
    flags = len = 0;
  }
};

struct Packet
{
  PacketHead ph;
  unsigned char data[0];

  uint16_t getDataSize() { return ph.len; }
  uint16_t getFullSize() { return PH_LEN+ph.len; }
};


class socket_t
{
	public:
		socket_t(int fd, const sockaddr_in &addr);
		~socket_t();

		int get_fd() const {return _fd;}

		bool valid(){return _fd!=-1;}
		void shutdown(int how);
		bool connect(const char *ip, int port);
		void close();

		bool setNonBlock();
		bool setSockOpt();
		void setComp(bool flag) { compFlag = flag; }

		bool getCmd(unsigned char *&cmd, uint16_t &len);
		bool popCmd();

		//数据放进缓冲区，等待发送
		bool sendCmd(const void *data, uint16_t len);
		//返回发送后缓冲区剩余字节数
		int sendCmd();

		bool readToBuf();
		bool writeToBuf(void *data, uint32_t len);

        uint16_t sizeMod8(uint16_t len);
	protected:
		int _fd;
		sockaddr_in _addr;
	public:
		in_addr& getIP()
		{
		  return _addr.sin_addr;
		}
		uint16_t getPort()
		{
		  return _addr.sin_port;
		}

    public:
		void putCmdToQueue(unsigned char *cmd, uint16_t len)
		{
			queue.PutCmd(cmd, len);
		}
		bool getCmdFromQueue(unsigned char *&cmd, uint16_t &len)
		{
			return queue.GetCmd(cmd, len);
		}
		bool popCmdFromQueue()
		{
			return queue.PopCmd();
		}
		void popAllCmdFromQueue()
		{
			queue.PopAllCmd();
		}
        void compressAll();
	private:
		xCmdQueue queue;

		//发送缓存
	private:
		bytebuff _write_buffer;
		xRWLock _write_critical;

		bytebuff _read_buffer;
		bytebuff _cmd_write_buffer;
		bytebuff _tmp_cmd_write_buffer;
		xRWLock _cmd_write_critical;

		bool encFlag;	//加密、解密标志
		bool compFlag;	//压缩标志

		bytebuff encBuffer;

		uint32_t unCompCmdRealSize;//未解压的消息长度
		bytebuff tmpWriteBuffer;	//临时存储压缩、加密数据
		bytebuff tmpDecBuffer;	//存放解密后的数据
		bytebuff cmdBuffer;		//存放解压后的数据

		xRWLock _send_critical;

};
