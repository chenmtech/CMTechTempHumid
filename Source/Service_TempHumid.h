/**
* 自定义环境温湿度服务：提供实时温湿度测量数据
*/

#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H


// 温湿度服务参数
#define TEMPHUMID_DATA                   0       //实时温湿度数据
#define TEMPHUMID_DATA_CHAR_CFG          1       //实时测量控制
#define TEMPHUMID_INTERVAL               2       //实时测量间隔
#define TEMPHUMID_IRANGE                 3       //实时测量间隔范围

// 温湿度数据属性在属性表中的数据
#define TEMPHUMID_DATA_POS               2

// 服务16位UUID
#define TEMPHUMID_SERV_UUID               0xAA60     // 温湿度服务UUID
#define TEMPHUMID_DATA_UUID               0xAA61     // 实时温湿度数据UUID
#define TEMPHUMID_INTERVAL_UUID           0x2A21     // 实时测量间隔UUID
#define TEMPHUMID_IRANGE_UUID             0x2906     // 实时测量间隔范围UUID



// 服务的bit field
#define TEMPHUMID_SERVICE               0x00000001


// 温湿度数据的字节长度
#define TEMPHUMID_DATA_LEN             4       


// 回调事件
#define TEMPHUMID_DATA_IND_ENABLED          1
#define TEMPHUMID_DATA_IND_DISABLED         2
#define TEMPHUMID_INTERVAL_SET              3

/**
 * Thermometer Interval Range 
 */
typedef struct
{
  uint16 low;         
  uint16 high; 
} tempHumidIRange_t;


// 当某个事件发生时调用的回调函数声明
typedef void (*tempHumidServiceCB_t)( uint8 event );


// 由应用层提供的回调结构体声明
typedef struct
{
  tempHumidServiceCB_t pfnTempHumidServiceCB;
} tempHumidServiceCBs_t;


// 加载本服务
extern bStatus_t TempHumid_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// 设置特征参数
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征参数
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

extern bStatus_t TempHumid_TempHumidIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId );

#endif












