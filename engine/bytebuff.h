#ifndef BASE_XLIB_XBYTEBUFFER_H_
#define BASE_XLIB_XBYTEBUFFER_H_
#include <stdint.h>
#include <ext/mt_allocator.h>
#include <string.h>
#include <vector>
#include <queue>

#define MAX_BUFSIZE 65536
#define PH_LEN 3
#define MAX_PACKSIZE (MAX_BUFSIZE-PH_LEN)
#define MIN_PACKSIZE 5  // for encrypt
#define MAX_CMD 255

class bytebuff
{
  public:
    bytebuff();
    ~bytebuff();

    uint32_t GetLeft() { return buffer_size_ - buffer_offset_; }
    void *GetBufOffset() { return &(byte_buffer_[buffer_offset_]); }
    void *GetBufOffset(uint32_t offset) { return &(byte_buffer_[offset]); }
    void *GetBufBegin() const { return &(byte_buffer_[0]); }
    void Copy(const bytebuff& buffer);

    uint32_t Put(const void *data, uint32_t len);
    uint32_t Put(uint32_t len);
    uint32_t Pop(uint32_t len);

    void Resize(uint32_t len);
    void Reset() { set_buffer_offset(0); }

    void set_buffer_offset(int buffer_offset) { buffer_offset_ = buffer_offset; }
    uint32_t buffer_offset() const { return buffer_offset_; }
    void set_buffer_size(int buffer_size) { buffer_size_ = buffer_size; }
    uint32_t buffer_size() const { return buffer_size_; }

  private:
    unsigned char *byte_buffer_;
    uint32_t buffer_size_;
    uint32_t buffer_offset_;

    __gnu_cxx::__mt_alloc<unsigned char> __mt_alloc;
};
#endif  // BASE_XLIB_XBYTEBUFFER_H_
