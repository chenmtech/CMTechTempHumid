
#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H

// 特征标记
#define TEMPHUMID_DATA    0       //体温值
#define TEMPHUMID_CONF    1       //测量控制点
#define TEMPHUMID_PERI    2       //测量周期




// 服务和特征的16位UUID
#define TEMPHUMID_SERV_UUID    0xAA60     // 温湿度服务UUID
#define TEMPHUMID_DATA_UUID    0xAA31
#define TEMPHUMID_CONF_UUID    0xAA32
#define TEMPHUMID_PERI_UUID    0xAA33


// 服务的bit field
#define TEMPHUMID_SERVICE               0x00000001

// 体温值的字节长度
#define TEMPHUMID_DATA_LEN     2

#define TEMPHUMID_MIN_PERIOD        1000    //最小采样周期为1000ms

#define TEMPHUMID_TIME_UNIT         1000    //采样周期时间单位为1000ms


typedef NULL_OK void (*tempHumidServiceCB_t)( uint8 paramID );


typedef struct
{
  tempHumidServiceCB_t        pfnTempHumidServiceCB;  // Called when characteristic value changes
} tempHumidServiceCBs_t;


// 加载本服务
extern bStatus_t TempHumid_AddService( uint32 services );

// 登记应用层回调
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// 设置特征值
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征值
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

#endif












