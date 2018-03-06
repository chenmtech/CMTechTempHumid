/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// 温湿度结构体定义
typedef struct SI7021_HumiAndTemp {
  long Humidity;          // 湿度值，单位：0.001%R.H.
  long Temperature;       // 温度值，单位：0.001摄氏度
} SI7021_HumiAndTemp;



//测湿度：返回 (相对湿度值*1000)
extern long SI7021_MeasureHumidity();

//测湿度之后读温度：返回 （摄氏温度*1000）
extern long SI7021_ReadTemperature();

//测温度：返回 (摄氏温度*1000)
extern long SI7021_MeasureTemperature();

//同时测湿度和温度
extern SI7021_HumiAndTemp SI7021_Measure();

 
#endif

