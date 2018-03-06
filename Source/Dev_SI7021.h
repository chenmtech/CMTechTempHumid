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

//����������Slave Address��SCLKƵ��
extern void SI7021_Start();

//��ʪ�ȣ����� (���ʪ��ֵ*1000)
extern long SI7021_MeasureHumidity();

//��ʪ��֮����¶ȣ����� �������¶�*1000��
extern long SI7021_ReadTemperature();

//���¶ȣ����� (�����¶�*1000)
extern long SI7021_MeasureTemperature();

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure();

//ֹͣ
extern void SI7021_Stop();
 
#endif

