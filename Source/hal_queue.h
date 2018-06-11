
#ifndef HAL_QUEUE_H
#define HAL_QUEUE_H

#include "bcomdef.h"

#define QUEUE_L         48
#define QUEUE_UNIT_L    10

extern void Queue_Init();

extern void Queue_Push(uint8* pData);

extern void Queue_Pop(uint8* pData);






#endif