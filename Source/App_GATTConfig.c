
#include "bcomdef.h"
#include "gatt.h"
#include "gattservapp.h"
#include "App_GATTConfig.h"
#include "Service_TempHumid.h"


//配置Height服务
//appCBs: 应用层为此服务提供的回调函数
//如果有其他服务需要配置，请仿照此函数创建一个新函数。
extern void GATTConfig_SetThermoService(tempHumidServiceCBs_t* appCBs)
{
  TempHumid_AddService( GATT_ALL_SERVICES );  // 加载服务  

  // 登记回调
  VOID TempHumid_RegisterAppCBs( appCBs );  
}
