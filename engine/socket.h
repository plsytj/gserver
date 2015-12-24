#pragma once
#include "bytebuffer.h"
#include "xCmdQueue.h"
#include "xLog.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// 包头标志
enum PACKET_FLAG_ENUM_TYPE
{
  PACKET_FLAG_COMPRESS = 1,  // 压缩
  PACKET_FLAG_ENCRYPT = 2,  // 加密
};

struct PacketHead
{
  BYTE flags;
  WORD len;
  PacketHead()
  {
    flags = len = 0;
  }
};


inline void SAFE_CLOSE_SOCKET(int fd, const char *name)
{
	XDBG("[Socket],%s,close %d", name, fd);

	int ret = 0;

	ret = shutdown(fd, SHUT_RDWR);

	ret = close(fd);
	if (0!=ret)
		XERR("[Socket]closesocket failed fd%d ret%d", fd, ret);

	fd = -1;
}

class socket_t
{
	public:
		socket_t(int fd, const socketaddr_in &addr);
		~socket_t();

		int get_fd() const {return _fd;}

		bool valid(){return _fd!=-1;}
		void shutdown(int how);
		bool connect(const char *ip, int port);
		void close();

		bool setNonBlock();
		bool setSockOpt();
		void setComp(bool flag) { compFlag = flag; }

		bool getCmd(BYTE *&cmd, uint16_t &len);
		bool popCmd();

		//数据放进缓冲区，等待发送
		bool sendCmd(const void *data, uint16_t len);
		//返回发送后缓冲区剩余字节数
		int sendCmd();
		bool sendFlashPolicy();

		bool readToBuf();
		bool writeToBuf(void *data, uint32_t len);

	protected:
		uint16_t sizeMod8(uint16_t len);//resize by 8 bytes, for encrypt

		void compressAll();
		uint16_t compress(void *data, uint64_t len);
		uint16_t uncompress(void *dest, uint64_t destLen, void *src, uint64_t srcLen);

		void encrypt(void *data, uint16_t len);
		void decrypt(void *data, uint16_t len);
		bool needEnc(){return encFlag;}
		bool needDec(){return encFlag;}

		int _fd;
		sockaddr_in _addr;
		int _epfd;

	public:
		in_addr& getIP()
		{
		  return _addr.sin_addr;
		}
		uint16_t getPort()
		{
		  return _addr.sin_port;
		}

		//发送缓存
	private:
		bytebuffer _write_buffer;
		xRWLock _write_critical;

		bytebuffer _read_buffer;
		bytebuffer _cmd_write_buffer;
		bytebuffer _tmp_cmd_write_buffer;
		xRWLock _cmd_write_critical;

		bool encFlag;	//加密、解密标志
		bool compFlag;	//压缩标志

		bytebuffer encBuffer;

		uint32_t unCompCmdRealSize;//未解压的消息长度
		bytebuffer tmpWriteBuffer;	//临时存储压缩、加密数据
		bytebuffer tmpDecBuffer;	//存放解密后的数据
		bytebuffer cmdBuffer;		//存放解压后的数据

		xRWLock _send_critical;

};
