

#ifndef APP_DATAPROCESSOR_H
#define APP_DATAPROCESSOR_H

#include "comdef.h"


// 初始化
extern void DP_Init(uint8 TaskID);

// 处理温度数据
extern void DP_Process(uint16 data);

// 预测温度操作
extern uint16 getPrecastTemp();

// 测温稳定提示操作
extern void notifyTempStable();






#endif













