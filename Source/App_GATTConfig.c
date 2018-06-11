
#include "bcomdef.h"
#include "gatt.h"
#include "gattservapp.h"
#include "App_GATTConfig.h"
#include "Service_TempHumid.h"


//����TempHumid����
//appCBs: Ӧ�ò�Ϊ�˷����ṩ�Ļص�����
//���������������Ҫ���ã�����մ˺�������һ���º�����
extern void GATTConfig_SetTempHumidService(tempHumidServiceCBs_t* appCBs)
{
  TempHumid_AddService( GATT_ALL_SERVICES );  // ���ط���  

  // �Ǽǻص�
  VOID TempHumid_RegisterAppCBs( appCBs );  
}

//����Timer����
extern void GATTConfig_SetTimerService(timerServiceCBs_t* appCBs)
{
  Timer_AddService( GATT_ALL_SERVICES );  // ���ط���  

  // �Ǽǻص�
  VOID Timer_RegisterAppCBs( appCBs );   
}