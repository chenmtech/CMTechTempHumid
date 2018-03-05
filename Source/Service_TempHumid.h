
#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H

// 特征标记
#define THERMOMETER_DATA    0       //体温值
#define THERMOMETER_CONF    1       //测量控制点
#define THERMOMETER_PERI    2       //测量周期




// 服务和特征的16位UUID
#define THERMOMETER_SERV_UUID    0xAA30
#define THERMOMETER_DATA_UUID    0xAA31
#define THERMOMETER_CONF_UUID    0xAA32
#define THERMOMETER_PERI_UUID    0xAA33


// 服务的bit field
#define THERMOMETER_SERVICE               0x00000001

// 体温值的字节长度
#define THERMOMETER_DATA_LEN     2

#define THERMOMETER_MIN_PERIOD        1000    //最小采样周期为1000ms

#define THERMOMETER_TIME_UNIT         1000    //采样周期时间单位为1000ms


typedef NULL_OK void (*thermometerServiceCB_t)( uint8 paramID );


typedef struct
{
  thermometerServiceCB_t        pfnThermometerServiceCB;  // Called when characteristic value changes
} thermometerServiceCBs_t;


// 加载本服务
extern bStatus_t Thermometer_AddService( uint32 services );

// 登记应用层回调
extern bStatus_t Thermometer_RegisterAppCBs( thermometerServiceCBs_t *appCallbacks );

// 设置特征值
extern bStatus_t Thermometer_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征值
extern bStatus_t Thermometer_GetParameter( uint8 param, void *value );

#endif












