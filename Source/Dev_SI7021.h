/*
  Copyright 2017 CentiMeter Technology Company <chenm91@163.com>
*/
#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

typedef struct SI7021_HumiAndTemp {
  long Humidity;
  long Temperature;
} SI7021_HumiAndTemp;

//启动：设置Slave Address和SCLK频率
extern void SI7021_Start();

//测湿度：返回 (相对湿度值*1000)
extern long SI7021_MeasureHumidity();

//测湿度之后读温度：返回 （摄氏温度*1000）
extern long SI7021_ReadTemperature();

//测温度：返回 (摄氏温度*1000)
extern long SI7021_MeasureTemperature();

//同时测湿度和温度
extern SI7021_HumiAndTemp SI7021_Measure();

//停止
extern void SI7021_Stop();
 
#endif

