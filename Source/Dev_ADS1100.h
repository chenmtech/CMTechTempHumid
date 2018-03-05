

#ifndef DEV_ADS1100_H
#define DEV_ADS1100_H

#include "comdef.h"
#include <iocc2541.h>


// 打开ADS1100电源
extern void ADS1100_TurnOn();

// 关闭ADS1100电源
extern void ADS1100_TurnOff();

// 初始化
extern void ADS1100_Init();

// 获取AD转换值
// 返回SUCCESS或者FAILURE
extern uint8 ADS1100_GetADValue(uint16 * pData);





#endif