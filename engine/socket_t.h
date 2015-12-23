#pragma once
#include "bytebuffer.h"
#include "xCmdQueue.h"
#include "xLog.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

inline void SAFE_CLOSE_SOCKET(int &fd, const char *name)
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
		socket_t(xNetProcessor *n);
		~socket_t();

		int get_fd() const {return _fd;}

		bool valid(){return _fd!=-1;}
		void shutdown(int how);
		bool connect(const char *ip, INT port);
		bool accept(int sockfd, const sockaddr_in &addr);
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

		inline void addEpoll(int ep)
		{
			_epfd = ep;
			epoll_event ev;
			bzero(&ev, sizeof(ev));
			ev.data.fd = _fd;
			ev.data.ptr = _np;
			ev.events = EPOLLIN|EPOLLOUT|EPOLLET;
			epoll_ctl(ep, EPOLL_CTL_ADD, _fd, &ev);
		}

		inline void addEpoll()
		{
			struct epoll_event ev;
			bzero(&ev, sizeof(ev));
			ev.data.fd = _fd;
			ev.events = EPOLLIN|EPOLLOUT|EPOLLET;
			ev.data.ptr = _np;
			epoll_ctl(_epfd, EPOLL_CTL_MOD, _fd, &ev);
		}

		inline void delEpoll()
		{
            XDBG("[Socket],%u,del epoll epfd:%u", _fd, _epfd);
			epoll_event ev;
			bzero(&ev, sizeof(ev));
			ev.data.fd = _fd;
			ev.data.ptr = _np;
			ev.events = EPOLLIN|EPOLLOUT|EPOLLET;
			epoll_ctl(_epfd, EPOLL_CTL_DEL, _fd, &ev);
			_epfd = 0;
		}

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
		INT _epfd;

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

		xNetProcessor *_np;
};
