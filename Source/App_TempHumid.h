/*
 * 这个模块用来实现体温计的应用功能
*/

#ifndef APP_TEMPHUMID_H
#define APP_TEMPHUMID_H

#include "comdef.h"

// 采集的三种数据类型
#define THERMOMETER_CONF_VALUETYPE_AD    0x01    //设置数据类型为AD值
#define THERMOMETER_CONF_VALUETYPE_R     0x02    //设置数据类型为电阻值
#define THERMOMETER_CONF_VALUETYPE_T     0x03    //设置数据类型为温度值


#define T_LOWLIMIT    3390    // 温度值下限
#define T_UPLIMIT     4410    // 温度值上限

#define R_LOWLIMIT    22332    // 电阻值下限
#define R_UPLIMIT     33528    // 电阻值上限

#define AD_LOWLIMIT   0        // AD下限
#define AD_UPLIMIT    32768    // AD上限


// 初始化
extern void Thermo_Init();

// 关硬件
extern void Thermo_HardwareOff();

// 开硬件
extern void Thermo_HardwareOn();

// 获取数据
extern uint16 Thermo_GetValue();

// 进行标定
extern void Thermo_DoCalibration();

// 获取数据类型
extern uint8 Thermo_GetValueType();

// 设置数据类型
extern void Thermo_SetValueType(uint8 type);

// 更新当前最大值
extern uint16 Thermo_UpdateMaxValue(uint16 value);

// 在LCD上显示一个值
extern void Thermo_ShowValueOnLCD(uint8 location, uint16 value);

// 开LCD
extern void Thermo_TurnOn_LCD();

// 关LCD
extern void Thermo_TurnOff_LCD();

// 开AD
extern void Thermo_TurnOn_AD();

// 关AD
extern void Thermo_TurnOff_AD();

// 开蜂鸣器
extern void Thermo_ToneOn();

// 关蜂鸣器
extern void Thermo_ToneOff();

// 设置预测温度值
extern void Thermo_SetPreTemp(uint16 temp);

// 设置是否显示预测温度值
extern void Thermo_SetShowPreTemp(bool isShow);


#endif