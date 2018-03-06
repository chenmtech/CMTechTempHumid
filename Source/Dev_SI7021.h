/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// ��ʪ�Ƚṹ�嶨��
typedef struct SI7021_HumiAndTemp {
  long Humidity;          // ʪ��ֵ����λ��0.001%R.H.
  long Temperature;       // �¶�ֵ����λ��0.001���϶�
} SI7021_HumiAndTemp;



//��ʪ�ȣ����� (���ʪ��ֵ*1000)
extern long SI7021_MeasureHumidity();

//��ʪ��֮����¶ȣ����� �������¶�*1000��
extern long SI7021_ReadTemperature();

//���¶ȣ����� (�����¶�*1000)
extern long SI7021_MeasureTemperature();

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure();

 
#endif

