/**
* 计时服务：提供一定精度的计时服务
* 计时精度不会很高，单位：分钟
*/

#ifndef SERVICE_TIMER_H
#define SERVICE_TIMER_H


// 特征标记位
#define TIMER_VALUE                   0


// 服务和特征的16位UUID
#define TIMER_SERV_UUID               0xAA70     // 定时服务UUID
#define TIMER_VALUE_UUID              0xAA71     // 定时特征值UUID


// 服务的bit field
#define TIMER_SERVICE               0x00000001


// 需要应用层提供的回调函数声明
// 当特征发生变化时，通知应用层
typedef NULL_OK void (*timerServiceCB_t)( uint8 paramID );

// 需要应用层提供的回调结构体声明
typedef struct
{
  timerServiceCB_t        pfnTimerServiceCB;
} timerServiceCBs_t;


// 加载本服务
extern bStatus_t Timer_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t Timer_RegisterAppCBs( timerServiceCBs_t *appCallbacks );

// 设置特征参数
extern bStatus_t Timer_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征参数
extern bStatus_t Timer_GetParameter( uint8 param, void *value );

#endif