#include "zlib/zlib.h"
extern "C"
{
#include "des/d3des.h"
}

#include "socket_t.h"
#include "xNetProcessor.h"
#include <errno.h>
#include <fcntl.h>
#include "xServer.h"

//压缩等级: 0 ~ 9, 数字越大，压缩率越高，耗时越长
#define COMPRESS_LEVEL 6
#define MIN_COMPRESS_SIZE 48

BYTE fixedkey[8] = { 95,27,5,20,131,4,8,88 };

socket_t::socket_t(xNetProcessor *n)
:_np(n)
{
	_fd = 0;

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

	//setSockOpt();
}

socket_t::~socket_t()
{
	close();
}

bool socket_t::accept(int sockfd, const sockaddr_in &addr)
{
	_fd = sockfd;
	_addr = addr;

	XLOG("[Socket],open,%s,%d,%s,%u", _np->name(), _fd, inet_ntoa(getIP()), getPort());

	return setNonBlock();
}

bool socket_t::connect(const char *ip, INT port)
{
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd<0)
	{
		XERR("[Socket],connect() %s:%lld socket failed %p", ip, port, this);
		return false;
	}
	bzero(&_addr, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(ip);
	_addr.sin_port = htons(port);

	int ret = ::connect(_fd, (sockaddr *)&_addr, sizeof(sockaddr_in));
	if (0!=ret)
	{
		XERR("[Socket],connect() %s:%lld failed with error %d %p", ip, port, ret, this);
		return false;
	}

	XLOG("[Socket],connect,%s:%lld,%u", ip, port, _fd);
	return setNonBlock();
}

void socket_t::close()
{
	if (valid())
	{
		SAFE_CLOSE_SOCKET(_fd, _np->name());
		_fd = -1;
	}
}

void socket_t::shutdown(int how)
{
	if (valid())
	{
		XDBG("[Socket],shutdown %s, %d", _np->name(), _fd);
		::shutdown(_fd, how);
	}
}

bool socket_t::setNonBlock()
{
	int flags = fcntl(_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if(-1 == fcntl(_fd, F_SETFL, flags))
	{
		XERR("[Socket],setNonBlock failed %s, %d", _np->name(), _fd);
        return false;
	}
    return true;
}

bool socket_t::setSockOpt()
{
	int nRecvBuf = MAX_BUFSIZE*2;
	setsockopt(_fd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	int nSendBuf = MAX_BUFSIZE*2;
	setsockopt(_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
    return true;
}

WORD socket_t::sizeMod8(WORD len)
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
			if ((errno!=EAGAIN)&&(errno!=EWOULDBLOCK))
			{
#ifdef _LX_DEBUG
				XERR("[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
#endif
				final_ret = false;
			}
			else
			{
#ifdef _LX_DEBUG
				//	XERR("[SOCKET],接收成功");
#endif
			}
			//XERR("[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
			break;
		}
		else if (0==ret)//peer shutdown
		{
			final_ret = false;
#ifdef _LX_DEBUG
			XERR("[SOCKET]接收错误,errno:%u,%s", errno, strerror(errno));
#endif
			break;
		}
		else
		{
			_read_buffer.Put(ret);
#ifdef _WUWENJUAN_DEBUG
      //XLOG("[SOCKET],fd:%u,recv:%u",_fd,ret);
#endif
		}
	}

	return final_ret;
}

int socket_t::sendCmd()
{
	if (!valid()) return -1;

	compressAll();
//	ScopeWriteLock swl(_write_critical);

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
#ifdef _WUWENJUAN_DEBUG
      //XLOG("[SOCKET],fd:%d,send:%u",_fd,ret);
#endif
		}
		else if (ret==0)
		{
			final_ret = _write_buffer.buffer_offset();
			XERR("[SOCKET],发送异常,fd:%d,ret:%d,real:%d", _fd, ret, realsend);
			break;
		}
		else
		{
			if(errno!=EWOULDBLOCK)
			{
				XERR("[SOCKET],发送错误,fd:%d,ret:%d,real:%d,errno:%u,%s", _fd, ret, realsend, errno, strerror(errno));
				final_ret = -1;
				_write_buffer.Pop(all);
			}
			XERR("[SOCKET],发送异常,fd:%d,ret:%d,real:%d,errno:%u,%s", _fd, ret, realsend, errno, strerror(errno));
			break;
		}
	}

	return final_ret;
}

bool socket_t::sendFlashPolicy()
{
	char policy[]="<?xml version=\"1.0\"?> <cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /> </cross-domain-policy>";
	int ret = 0;
	{
//		ScopeWriteLock swl(_write_critical);
		ret = ::send(_fd,(const char*)policy,sizeof(policy),0);
	}
	if(ret!=(int)sizeof(policy))
	{
		XERR("[SOCKET],fd:%d,send flashPolicy fail,errno:%u,%s,ret:%d",_fd,errno, strerror(errno),ret);
		return false;
	}
	else
	{
		shutdown(SHUT_RDWR);
		return true;
	}
}

bool socket_t::sendCmd(const void *data, WORD len)
{
  if (!valid()) return false;

  PacketHead ph;
  ph.len = len;
  ScopeWriteLock swl(_cmd_write_critical);
  _cmd_write_buffer.Put(&ph,PH_LEN);
  _cmd_write_buffer.Put(data,len);
  //XDBG("[SOCKET],fd:%d,send len:%u",_fd,len);
  return true;
}

bool socket_t::writeToBuf(void *data, uint32_t len)
{
	uint32_t real_size = 0;

//	ScopeWriteLock swl(_write_critical);
	real_size = _write_buffer.Put(data, len);

	return (real_size==len);
}

WORD socket_t::compress(void *data, QWORD len)
{
	QWORD newLen = tmpWriteBuffer.buffer_size();
	if (Z_OK == ::compress2((Bytef *)tmpWriteBuffer.GetBufBegin(), (uLongf *)&newLen, (Bytef *)data, (uLong)len, COMPRESS_LEVEL))
	{
		memcpy(data, tmpWriteBuffer.GetBufBegin(), newLen);
		return newLen;
	}
	XLOG("[socket_t],%s,fd:%d,压缩错误", _np->name(), _fd);
	return 0;
}

WORD socket_t::uncompress(void *dest, QWORD destLen, void *src, QWORD srcLen)
{
	QWORD newLen = destLen;
	if (Z_OK == ::uncompress((Bytef *)dest, (uLongf *)&newLen, (Bytef *)src, (uLong)srcLen))
	{
		return newLen;
	}

	return 0;
}

void socket_t::encrypt(void *data, WORD len)
{
	deskey(fixedkey, EN0);
	for (WORD i = 0; i < len; i += 8)
		des((BYTE *)data + i, (BYTE *)tmpWriteBuffer.GetBufOffset(i));
	memcpy(data, tmpWriteBuffer.GetBufBegin(), len);
}

void socket_t::decrypt(void *data, WORD len)
{
	deskey(fixedkey, DE1);
	for (WORD i = 0; i < len; i += 8)
		des((BYTE *)data + i, (BYTE *)tmpDecBuffer.GetBufOffset(i));
	memcpy(data, tmpDecBuffer.GetBufBegin(), len);
}

bool socket_t::getCmd(BYTE *&cmd, WORD &len)
{
	//return cmdQueue.getCmd(cmd, len);

	cmd = 0;
	len = 0;

	uint32_t used = _read_buffer.buffer_offset();
	if (used<MIN_PACKSIZE) return false;

	//check flashPolicy
	//<policy-file-request/>
	if(*((BYTE*)_read_buffer.GetBufBegin())=='<')
	{
		if(_read_buffer.buffer_offset()>=22)
		{
			sendFlashPolicy();
			_read_buffer.Pop(22);
		}
		return false;
	}
	Packet *p = (Packet *)_read_buffer.GetBufBegin();
	WORD real_size = 0;

	if (needDec())
	{
		BYTE copy[MIN_PACKSIZE];
		memcpy(copy, (BYTE *)_read_buffer.GetBufBegin(), MIN_PACKSIZE);
		decrypt(copy, MIN_PACKSIZE);
		real_size = ((Packet *)copy)->getFullSize();
	}
	else
		real_size = p->getFullSize();
	real_size = sizeMod8(real_size);

	if (real_size>used) return false;

	if (needDec())
		decrypt(p, real_size);

	unCompCmdRealSize = real_size;

	if (p->ph.flags & PACKET_FLAG_COMPRESS)
	{
		len = uncompress(cmdBuffer.GetBufBegin(), cmdBuffer.buffer_size(), p->data, p->ph.len);
		if (len > 0)
			cmd = (BYTE *)cmdBuffer.GetBufBegin();
		else
			XLOG("[socket_t],%s,fd:%d,解压错误,消息长度:%u", _np->name(), _fd, p->ph.len);
	}
	else
	{
		cmd = p->data;
		len = p->ph.len;
	}
  //XLOG("[SOCKET],fd:%u,recv cmd len:%u",_fd,len);
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
	{
		_tmp_cmd_write_buffer.Reset();
		ScopeWriteLock swl(_cmd_write_critical);
		_tmp_cmd_write_buffer.Copy(_cmd_write_buffer);
		_cmd_write_buffer.Reset();
	}

	while(_tmp_cmd_write_buffer.buffer_offset()>0)
	{
		PacketHead *oldPHead = (PacketHead *)_tmp_cmd_write_buffer.GetBufBegin();
		encBuffer.Reset();
		PacketHead ph;
		encBuffer.Put(&ph, PH_LEN);
		encBuffer.Put(_tmp_cmd_write_buffer.GetBufOffset(PH_LEN), oldPHead->len);
		PacketHead *p = (PacketHead *)encBuffer.GetBufBegin();

		WORD real_size = oldPHead->len;
		if (compFlag && oldPHead->len >= MIN_COMPRESS_SIZE)
		{
			real_size = compress(encBuffer.GetBufOffset(PH_LEN), oldPHead->len);
			if (real_size > 0)
				p->flags |= PACKET_FLAG_COMPRESS;
			else
				real_size = oldPHead->len;
		}
		p->len = real_size;
		real_size += PH_LEN;
		real_size = sizeMod8(real_size);
		if (needEnc())
		{
			p->flags |= PACKET_FLAG_ENCRYPT;
			encrypt(encBuffer.GetBufBegin(), real_size);
		}

		if (!writeToBuf(encBuffer.GetBufBegin(), real_size))
		{
			XLOG("[socket_t],%s,fd:%d,push cmd error", _np->name(), _fd);
		}
		_tmp_cmd_write_buffer.Pop(oldPHead->len+PH_LEN);
	}
}
