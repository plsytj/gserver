#ifndef BASE_XLIB_XCMDQUEUE_H_
#define BASE_XLIB_XCMDQUEUE_H_
#include <stdint.h>
#include <ext/mt_allocator.h>
#include <vector>
#include <queue>
#include <utility>
#include "xMutex.h"

struct xCmd
{
  unsigned short len;
  unsigned char data[0];
  xCmd() { len = 0; }
};
//  }__attribute__((packed));

const int XCMD_SIZE = sizeof(xCmd);

class xCmdQueue
{
  public:
    xCmdQueue();
    ~xCmdQueue();

  public:
    bool empty() { return cmd_queue_.empty(); }
    void PutCmd(unsigned char *cmd, unsigned short len);
    bool GetCmd(unsigned char *&cmd, uint16_t &len);
    bool PopCmd();
    void PopAllCmd();

    bool is_valid() { return is_valid_; }

  protected:
    std::queue<xCmd*> cmd_queue_;
    xRWLock cmd_queue_lock_;

    bool is_valid_;

    __gnu_cxx::__mt_alloc<unsigned char> __mt_alloc;
};
#endif  // BASE_XLIB_XCMDQUEUE_H_
