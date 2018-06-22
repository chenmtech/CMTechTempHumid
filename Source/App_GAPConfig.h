/**
* 本模块主要用来配置与GAP有关的参数，包括广播参数，连接参数，GGS参数和绑定参数
*/
#ifndef APP_GAPCONFIG_H
#define APP_GAPCONFIG_H

// 配对密码操作
#define GAP_PARI_PASSWORD_READ    0x00    // 读
#define GAP_PARI_PASSWORD_WRITE   0x01    // 写


//设置与广播相关的参数，但是大多数广播数据和扫描响应数据还是静态配置
//advInt: 广播间隔时间，单位ms，范围20~10240ms
//servUUID：本设备包含的服务UUID
extern void GAPConfig_SetAdvParam(uint16 advInt, uint16 servUUID);


//使能或停止广播
extern void GAPConfig_EnableAdv(uint8 enable);


//设置与连接相关的参数
//连接间隔时间：范围7.5ms~4s之间1.25ms的整数倍。如果min和max不一样，芯片会选择一个靠近max的值
//slave latency: 从机的潜伏次数，范围0 - 499。这个数的连接间隔过后，下一个连接间隔，从机必须响应
//监控超时时间：范围100ms-32s。书上建议至少给从机6次侦听的机会
//minInt: 单位ms
//maxInt: 单位ms
//latency: 潜伏次数
//timeout: 监控超时时间，单位ms
//when: 连接建立后多少秒开始更新参数，单位s
extern void GAPConfig_SetConnParam(uint16 minInt, uint16 maxInt, uint16 latency, uint16 timeout, uint8 when);


//终止连接。从机没有权利发起连接，但可以主动终止连接
extern bStatus_t GAPConfig_TerminateConn();


//设置GGS参数
//devName: 设备名属性值
extern void GAPConfig_SetGGSParam(uint8* devName);


//设置配对与绑定相关参数
// pairmode : 配对模式，
//      GAPBOND_PAIRING_MODE_NO_PAIRING: 不允许配对
//      GAPBOND_PAIRING_MODE_WAIT_FOR_REQ: 等待对方请求或安全需要时请求
//      GAPBOND_PAIRING_MODE_INITIATE: 立刻启动配对
// isBonding: 是否绑定
extern void GAPConfig_SetPairBondingParam(uint8 pairMode, uint8 isBonding);

// 读配对密码
extern uint32 GapConfig_ReadPairPassword();

// 写配对密码
extern void GapConfig_WritePairPassword(uint32 password);

// 对配对密码进行读写操作
// flag: 0-》读，1-》写
// pPassword: 密码存储的地址
// len: 密码的字节长度
extern void GapConfig_SNV_Password(uint8 flag, uint8* pPassword, uint8 len);

#endif





