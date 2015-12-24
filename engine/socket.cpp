#include "zlib.h"


#include "socket.h"
#include <errno.h>
#include <fcntl.h>

#define COMPRESS_LEVEL 6
#define MIN_COMPRESS_SIZE 48

unsigned char fixedkey[8] = { 95,27,5,20,131,4,8,88 };

socket_t::socket_t(int sockfd, const sockaddr_in &addr)
{
	_fd = sockfd;
    _addr = addr;

	_write_buffer.Resize(MAX_BUFSIZE*2);
	_read_buffer.Resize(MAX_BUFSIZE*2);
	encBuffer.Resize(MAX_BUFSIZE);
	tmpWriteBuffer.Resize(MAX_BUFSIZE);
	tmpDecBuffer.Resize(MAX_BUFSIZE);
	cmdBuffer.Resize(MAX_BUFSIZE);
	_cmd_write_buffer.Resize(MAX_BUFSIZE*2);
	_tmp_cmd_write_buffer.Resize(MAX_BUFSIZE*2);

	unCompCmdRealSize = 0;
	encFlag = false;
	compFlag = false;
    setNonBlock();

}

socket_t::~socket_t()
{
	close();
}


bool socket_t::connect(const char *ip, int port)
{
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd<0)
	{
		fprintf(stderr, "[Socket],connect() %s:%d socket failed %p", ip, port, this);
		return false;
	}
	bzero(&_addr, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(ip);
	_addr.sin_port = htons(port);

	int ret = ::connect(_fd, (sockaddr *)&_addr, sizeof(sockaddr_in));
	if (0!=ret)
	{
		fprintf(stderr, "[Socket],connect() %s:%d failed with error %d", ip, port, ret);
		return false;
	}

	return setNonBlock();
}

void socket_t::close()
{
	if (valid())
	{
        ::close(_fd);
		_fd = -1;
	}
}

void socket_t::shutdown(int how)
{
	if (valid())
	{
		::shutdown(_fd, how);
	}
}

bool socket_t::setNonBlock()
{
	int flags = fcntl(_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if(-1 == fcntl(_fd, F_SETFL, flags))
	{
        return false;
	}
    return true;
}

uint16_t socket_t::sizeMod8(uint16_t len)
{
	return len;
	return (len+7)/8*8;
}

bool socket_t::readToBuf()
{
	bool final_ret = true;
	while (1)
	{
		if (_read_buffer.GetLeft()<MAX_BUFSIZE)
		{
			_read_buffer.Resize(_read_buffer.buffer_size()+MAX_BUFSIZE);
		}

		int ret = ::recv(_fd, _read_buffer.GetBufOffset(), MAX_BUFSIZE, 0);
		if (ret<0)
		{
			if (errno!=EAGAIN && errno!=EWOULDBLOCK )
			{
				fprintf(stderr, "[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
				final_ret = false;
			}
			break;
		}
		else if (0==ret)//peer shutdown
		{
			final_ret = false;
#ifdef _LX_DEBUG
		//	XERR("[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
#endif
			break;
		}
		else
		{
			_read_buffer.Put(ret);
		}
	}

	return final_ret;
}

int socket_t::sendCmd()
{
	if (!valid()) return -1;

	compressAll();

	int final_ret = 0;
	int all = _write_buffer.buffer_offset();
	while (all)
	{
		int realsend = std::min(all, MAX_BUFSIZE*2);
		int ret = ::send(_fd, _write_buffer.GetBufBegin(), realsend, 0);
		if (ret>0)
		{
			_write_buffer.Pop(ret);
			all = _write_buffer.buffer_offset();
		}
		else if (ret==0)
		{
			final_ret = _write_buffer.buffer_offset();
			fprintf(stderr,"[SOCKET],发送异常,fd:%d,ret:%d,real:%d", _fd, ret, realsend);
			break;
		}
		else
		{
			if(errno!=EWOULDBLOCK)
			{
				fprintf(stderr, "[SOCKET],发送错误,fd:%d,ret:%d,real:%d,errno:%u,%s", _fd, ret, realsend, errno, strerror(errno));
				final_ret = -1;
				_write_buffer.Pop(all);
			}
			fprintf(stderr, "[SOCKET],发送异常,fd:%d,ret:%d,real:%d,errno:%u,%s", _fd, ret, realsend, errno, strerror(errno));
			break;
		}
	}

	return final_ret;
}


bool socket_t::sendCmd(const void *data, uint16_t len)
{
  if (!valid()) return false;

  PacketHead ph;
  ph.len = len;
  ScopeWriteLock swl(_cmd_write_critical);
  _cmd_write_buffer.Put(&ph,PH_LEN);
  _cmd_write_buffer.Put(data,len);
  return true;
}

bool socket_t::writeToBuf(void *data, uint32_t len)
{
	uint32_t real_size = 0;

	real_size = _write_buffer.Put(data, len);

	return (real_size==len);
}

bool socket_t::getCmd(unsigned char *&cmd, uint16_t &len)
{
	cmd = 0;
	len = 0;

	uint32_t used = _read_buffer.buffer_offset();
	if (used<MIN_PACKSIZE) return false;

	Packet *p = (Packet *)_read_buffer.GetBufBegin();
	uint16_t real_size = 0;

	
	real_size = p->getFullSize();
	real_size = sizeMod8(real_size);

	if (real_size>used) return false;

	cmd = p->data;
	len = p->ph.len;
	return true;
}

bool socket_t::popCmd()
{
	if (unCompCmdRealSize >= MIN_PACKSIZE && unCompCmdRealSize <= _read_buffer.buffer_offset())
	{
		if (_read_buffer.Pop(unCompCmdRealSize))
		{
			unCompCmdRealSize = 0;
			return true;
		}
	}

	return false;
}

void socket_t::compressAll()
{
    do
	{
		_tmp_cmd_write_buffer.Reset();
		ScopeWriteLock swl(_cmd_write_critical);
		_tmp_cmd_write_buffer.Copy(_cmd_write_buffer);
		_cmd_write_buffer.Reset();
	}while(0);

	while(_tmp_cmd_write_buffer.buffer_offset()>0)
	{
		PacketHead *oldPHead = (PacketHead *)_tmp_cmd_write_buffer.GetBufBegin();
		encBuffer.Reset();
		PacketHead ph;
		encBuffer.Put(&ph, PH_LEN);
		encBuffer.Put(_tmp_cmd_write_buffer.GetBufOffset(PH_LEN), oldPHead->len);
		PacketHead *p = (PacketHead *)encBuffer.GetBufBegin();

		uint16_t real_size = oldPHead->len;
		/*if (compFlag && oldPHead->len >= MIN_COMPRESS_SIZE)
		{
			real_size = compress(encBuffer.GetBufOffset(PH_LEN), oldPHead->len);
			if (real_size > 0)
				p->flags |= PACKET_FLAG_COMPRESS;
			else
				real_size = oldPHead->len;
		}
        */
		p->len = real_size;
		real_size += PH_LEN;
		real_size = sizeMod8(real_size);
		//if (needEnc())
        if(0)
		{
		//	p->flags |= PACKET_FLAG_ENCRYPT;
		//	encrypt(encBuffer.GetBufBegin(), real_size);
		}

		if (!writeToBuf(encBuffer.GetBufBegin(), real_size))
		{
			fprintf(stderr, "[socket_t],fd:%d,push cmd error",  _fd);
		}
		_tmp_cmd_write_buffer.Pop(oldPHead->len+PH_LEN);
	}
}
