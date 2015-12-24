#include "xCmdQueue.h"

xCmdQueue::xCmdQueue()
{
  is_valid_ = true;
}

xCmdQueue::~xCmdQueue()
{
  is_valid_ = false;
  PopAllCmd();
}

void xCmdQueue::PopAllCmd()
{
  ScopeWriteLock swl(cmd_queue_lock_);
  while (!cmd_queue_.empty())
  {
    xCmd *p = cmd_queue_.front();
    cmd_queue_.pop();
    if (p)
    {
      __mt_alloc.deallocate((unsigned char *)p, p->len+XCMD_SIZE);
    }
  }
}

void xCmdQueue::PutCmd(unsigned char *cmd, unsigned short len)
{
  if (!cmd || !is_valid()) return;
  unsigned char *ch = __mt_alloc.allocate(len+XCMD_SIZE);
  ((xCmd *)ch)->len = len;
  bcopy(cmd, ((xCmd *)ch)->data, (uint32_t)len);
  ScopeWriteLock swl(cmd_queue_lock_);
  cmd_queue_.push((xCmd *)ch);
}

bool xCmdQueue::GetCmd(unsigned char *&cmd, uint16_t &len)
{
  cmd = 0;
  len = 0;
  ScopeWriteLock swl(cmd_queue_lock_);
  if (!cmd_queue_.empty())
  {
    xCmd *p = cmd_queue_.front();
    if (p)
    {
      len = p->len;
      cmd = p->data;
    }
    return true;
  }
  return false;
}

bool xCmdQueue::PopCmd()
{
  ScopeWriteLock swl(cmd_queue_lock_);
  if (!cmd_queue_.empty())
  {
    xCmd *p = cmd_queue_.front();
    cmd_queue_.pop();
    if (p)
    {
      __mt_alloc.deallocate((unsigned char *)p, p->len+XCMD_SIZE);
    }
    return true;
  }
  return false;
}
