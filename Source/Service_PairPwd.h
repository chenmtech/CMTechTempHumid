/**
* 配对密码操作服务：提供对配对密码进行操作的服务
*/

#ifndef SERVICE_PAIRPWD_H
#define SERVICE_PAIRPWD_H


// 特征标记位
#define PAIRPWD_VALUE                   0


// 服务和特征的16位UUID
#define PAIRPWD_SERV_UUID               0xAA80     // 配对密码服务UUID
#define PAIRPWD_VALUE_UUID              0xAA81     // 配对密码特征值UUID


// 服务的bit field
#define PAIRPWD_SERVICE               0x00000001


// 需要应用层提供的回调函数声明
// 当特征发生变化时，通知应用层
typedef NULL_OK void (*pairPwdServiceCB_t)( uint8 paramID );

// 需要应用层提供的回调结构体声明
typedef struct
{
  pairPwdServiceCB_t        pfnPairPwdServiceCB;
} pairPwdServiceCBs_t;


// 加载本服务
extern bStatus_t PAIRPWD_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t PAIRPWD_RegisterAppCBs( pairPwdServiceCBs_t *appCallbacks );


#endif