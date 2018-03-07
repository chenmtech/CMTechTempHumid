/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// 温湿度结构体定义
typedef struct SI7021_HumiAndTemp {
  float humid;          // 湿度值，单位：%R.H.
  float temp;           // 温度值，单位：摄氏度
} SI7021_HumiAndTemp;



//测湿度
extern float SI7021_MeasureHumidity();

//测湿度之后读温度
extern float SI7021_ReadTemperature();

//测温度
extern float SI7021_MeasureTemperature();

//同时测湿度和温度
extern SI7021_HumiAndTemp SI7021_Measure();

 
#endif

