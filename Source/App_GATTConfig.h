/**
* ���ģ�����ṩ����Ӧ�÷�������ù���
* ��ͬӦ�÷�����Ҫ������Ӧ�����ú���
*/
#ifndef APP_GATTCONFIG_H
#define APP_GATTCONFIG_H


#include "Service_TempHumid.h"

#include "Service_Timer.h"

//����TempHumid����
extern void GATTConfig_SetTempHumidService(tempHumidServiceCBs_t* appCBs);


//����Timer����
extern void GATTConfig_SetTimerService(timerServiceCBs_t* appCBs);





#endif