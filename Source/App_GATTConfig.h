/**
* 这个模块是提供特殊应用服务的配置功能
* 不同应用服务需要创建相应的配置函数
*/
#ifndef APP_GATTCONFIG_H
#define APP_GATTCONFIG_H


#include "Service_TempHumid.h"

#include "Service_Timer.h"

#include "Service_PairPwd.h"

#include "Service_Battery.h"

//配置TempHumid服务
extern void GATTConfig_SetTempHumidService(tempHumidServiceCBs_t* appCBs);


//配置Timer服务
extern void GATTConfig_SetTimerService(timerServiceCBs_t* appCBs);

extern void GATTConfig_SetPairPwdService(pairPwdServiceCBs_t* appCBs);

//配置电池电量服务
extern void GATTConfig_SetBatteryService(batteryServiceCBs_t* appCBs);

#endif