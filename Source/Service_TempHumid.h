/**
* 环境温湿度服务：提供实时温湿度测量，并保存过去24小时历史测量数据
*/

#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H


// 特征标记位
#define TEMPHUMID_DATA          0       //实时温湿度数据
#define TEMPHUMID_CTRL          1       //实时测量控制
#define TEMPHUMID_PERI          2       //实时测量周期
#define TEMPHUMID_HISTORYTIME   3       //历史测量数据的时间点
#define TEMPHUMID_HISTORYDATA   4       //历史测量数据


// 服务和特征的16位UUID
#define TEMPHUMID_SERV_UUID               0xAA60     // 温湿度服务UUID
#define TEMPHUMID_DATA_UUID               0xAA61     // 实时温湿度数据UUID
#define TEMPHUMID_CTRL_UUID               0xAA62     // 实时测量控制UUID
#define TEMPHUMID_PERI_UUID               0xAA63     // 实时测量周期UUID

/*历史数据的测量周期依赖计时服务中的定时周期，由定时周期控制*/
#define TEMPHUMID_HISTORY_TIME_UUID       0xAA64      // 历史测量数据的时间UUID
#define TEMPHUMID_HISTORY_DATA_UUID       0xAA65      // 历史测量数据UUID


// 服务的bit field
#define TEMPHUMID_SERVICE               0x00000001


// 温湿度值的字节长度
#define TEMPHUMID_DATA_LEN             8       // 两个float的长度

#define TEMPHUMID_PERIOD_MIN_V         5       //最小采样周期为500ms

#define TEMPHUMID_PERIOD_UNIT          100     //采样周期的单位：100ms


// 需要应用层提供的回调函数声明
// 当特征发生变化时，通知应用层
typedef NULL_OK void (*tempHumidServiceCB_t)( uint8 paramID );

// 需要应用层提供的回调结构体声明
typedef struct
{
  tempHumidServiceCB_t        pfnTempHumidServiceCB;
} tempHumidServiceCBs_t;


// 加载本服务
extern bStatus_t TempHumid_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// 设置特征参数
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征参数
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

#endif












