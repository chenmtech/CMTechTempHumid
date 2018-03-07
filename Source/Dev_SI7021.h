/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// ��ʪ�Ƚṹ�嶨��
typedef struct SI7021_HumiAndTemp {
  float humid;          // ʪ��ֵ����λ��%R.H.
  float temp;           // �¶�ֵ����λ�����϶�
} SI7021_HumiAndTemp;



//��ʪ��
extern float SI7021_MeasureHumidity();

//��ʪ��֮����¶�
extern float SI7021_ReadTemperature();

//���¶�
extern float SI7021_MeasureTemperature();

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure();

 
#endif

