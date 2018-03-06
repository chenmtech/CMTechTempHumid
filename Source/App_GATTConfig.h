/**
* 这个模块是提供特殊应用服务的配置功能
* 不同应用服务需要创建相应的配置函数
*/
#ifndef APP_GATTCONFIG_H
#define APP_GATTCONFIG_H


#include "Service_TempHumid.h"

//配置Height服务
extern void GATTConfig_SetThermoService(tempHumidServiceCBs_t* appCBs);








#endif