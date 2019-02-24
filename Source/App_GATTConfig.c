
#include "bcomdef.h"
#include "gatt.h"
#include "gattservapp.h"
#include "App_GATTConfig.h"
#include "Service_TempHumid.h"


//配置TempHumid服务
//appCBs: 应用层为此服务提供的回调函数
//如果有其他服务需要配置，请仿照此函数创建一个新函数。
extern void GATTConfig_SetTempHumidService(tempHumidServiceCBs_t* appCBs)
{
  TempHumid_AddService( GATT_ALL_SERVICES );  // 加载服务  

  // 登记回调
  VOID TempHumid_RegisterAppCBs( appCBs );  
}

//配置Timer服务
extern void GATTConfig_SetTimerService(timerServiceCBs_t* appCBs)
{
  Timer_AddService( GATT_ALL_SERVICES );  // 加载服务  

  // 登记回调
  VOID Timer_RegisterAppCBs( appCBs );   
}

//配置配对密码服务
extern void GATTConfig_SetPairPwdService(pairPwdServiceCBs_t* appCBs)
{
  PAIRPWD_AddService( GATT_ALL_SERVICES );  // 加载服务  

  // 登记回调
  VOID PAIRPWD_RegisterAppCBs( appCBs );   
}

//配置电池电量服务
extern void GATTConfig_SetBatteryService(batteryServiceCBs_t* appCBs)
{
  Battery_AddService( GATT_ALL_SERVICES );  // 加载服务  

  // 登记回调
  VOID Battery_RegisterAppCBs( appCBs );   
}