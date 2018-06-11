
#include "hal_queue.h"
#include "OSAL.h"

static uint8 buf[QUEUE_L*QUEUE_UNIT_L] = {0};
static uint16 length = 0;

//idx_send: 指向等待发送的数据索引
//idx_push: 指向要压入的数据索引
static uint16 idx_send, idx_push;

extern void Queue_Init()
{
  length = QUEUE_L*QUEUE_UNIT_L;
  idx_send = 0;
  idx_push = 0;   
}

extern void Queue_Push(uint8* pData)
{
  VOID osal_memcpy( &buf[idx_push], pData, QUEUE_UNIT_L );
  idx_push += QUEUE_UNIT_L;
  
  if(idx_push == length) idx_push = 0;
  if(idx_push == idx_send) 
  {
    idx_send += QUEUE_UNIT_L;
    if(idx_send == length) idx_send = 0;
  }  
}

extern void Queue_Pop(uint8* pData)
{
  //
  if(idx_send != idx_push)
  {
    VOID osal_memcpy(pData, &buf[idx_send], QUEUE_UNIT_L);
    idx_send += QUEUE_UNIT_L;
    if(idx_send == length) idx_send = 0;
  }else {
    pData = NULL;
  }
}