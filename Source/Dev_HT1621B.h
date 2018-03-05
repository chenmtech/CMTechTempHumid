#ifndef _HT1621B_H_
#define _HT1621B_H_

#include "comdef.h"
#include <iocc2541.h>




// 初始化HT IO端口
// 将VDD, CS, WR, DATA都设为GPIO输出低电平，以便降低功耗
extern void HT1621B_InitToPowerDown();

// 给LCD上电，通过命令初始化LCD所需设置
extern void HT1621B_TurnOnLCD(void);

// 关LCD，并回到低功耗状态
extern void HT1621B_TurnOffLCD(void);

// 清除屏幕
extern void HT1621B_ClearLCD(void);

// 在指定液晶位置写一个字节段码
extern void HT1621B_WR_ONEBYTE(uint8 location, uint8 segCode);

// 在指定液晶位置开始，写nWord长度的段码数据
extern void HT1621B_WR_LCD_MEM(uint8 location, uint8 * Seg_Buf, uint8 nWord);

// 从指定位置开始，显示一个uint16的十进制数据
extern void HT1621B_ShowUint16Data(uint8 location, uint16 data);

// 显示一个温度，带小数点，isPreTemp用于判断是否预测温度
extern void HT1621B_ShowTemperature(uint8 location, uint16 temp, bool isPreTemp);

// 显示"L"
extern void HT1621B_ShowL(uint8 location);

// 显示"H"
extern void HT1621B_ShowH(uint8 location);

// 开蜂鸣器
extern void HT1621B_ToneOn();

// 关蜂鸣器
extern void HT1621B_ToneOff();

// 
#endif