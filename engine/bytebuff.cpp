#include "bytebuff.h"

#define MAX_BUFSIZE 65536
bytebuff::bytebuff()
{
    buffer_offset_ = 0;
    buffer_size_ = 0;
    byte_buffer_ = NULL;

    Resize(MAX_BUFSIZE * 2);
}

bytebuff::~bytebuff(void)
{
    if (byte_buffer_)
    {
        __mt_alloc.deallocate((unsigned char*)byte_buffer_, buffer_size());
        byte_buffer_ = NULL;
        buffer_offset_ = 0;
        buffer_size_ = 0;
    }
}

void bytebuff::Resize(uint32_t len)
{
    if (len <= buffer_offset()) return;

    unsigned char* buf = __mt_alloc.allocate(len);

    if (byte_buffer_)
    {
        bcopy(&byte_buffer_[0], &buf[0], buffer_offset());
        __mt_alloc.deallocate((unsigned char*)byte_buffer_, buffer_size());
    }

    byte_buffer_ = buf;
    set_buffer_size(len);
}

uint32_t bytebuff::Put(const void* data, uint32_t len)
{
    if (len > MAX_PACKSIZE)
    {
        return 0;
    }

    while (GetLeft() < len)
    {
        Resize(buffer_size() + MAX_BUFSIZE * 10);
    }
    memcpy(GetBufOffset(), data, len);
    buffer_offset_ += len;
    return len;
}

uint32_t bytebuff::Put(uint32_t len)
{
    if ((buffer_offset() + len) > buffer_size())
    {
        return 0;
    }

    buffer_offset_ += len;

    if (GetLeft() < MAX_BUFSIZE)
    {
        Resize(buffer_size() + MAX_BUFSIZE * 10);
    }
    return len;
}

uint32_t bytebuff::Pop(uint32_t len)
{
    if (len < buffer_offset())
    {
        bcopy(&byte_buffer_[len], &byte_buffer_[0], buffer_offset() - len);
        buffer_offset_ -= len;
    }
    else if (len > buffer_offset())
    {
        buffer_offset_ = 0;
    }
    else
    {
        buffer_offset_ = 0;
    }
    return len;
}

void bytebuff::Copy(const bytebuff& buffer)
{
    if (buffer_size() < buffer.buffer_size())
    {
        Resize(buffer.buffer_size());
    }
    bcopy(buffer.GetBufBegin(), GetBufBegin(), buffer.buffer_offset());
    buffer_offset_ += buffer.buffer_offset();
}
