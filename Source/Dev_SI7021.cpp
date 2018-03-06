/*
* Dev_Si7021: ��ʪ�ȴ�����Si7021������
* Written by Chenm 2017-04-29

* ����ÿ�ε��ò���ʪ�Ȼ��¶ȵĺ�����������HalI2CEnable��HalI2CDisable������
* ����ֻ�ܶ�һ�Σ����˳�
* Written by Chenm 2017-05-10

* �������I2C����Ҫÿ�ζ�����֮ǰ��Ҫ����HalI2CInit��ʼ��
* ����ͻ������������
* Written by Chenm 2018-03-06
*/


#include "Dev_SI7021.h"

#define I2C_ADDR 0x40   //Si7021��I2C��ַ

// I2C����
#define I2CCMD_MEASURE_HUMIDITY	    0xE5      //��ʪ��

#define I2CCMD_READ_TEMPERATURE	    0xE0      //������ʪ��֮����¶�

#define I2CCMD_MEASURE_TEMPERATURE  0xE3      //���¶�

static uint8 cmd = 0;
static uint8 data[2] = {0};

//����������Slave Address��SCLKƵ��
extern void SI7021_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

extern long SI7021_MeasureHumidity()
{
  HalI2CEnable();
  
  //�ȷ�����ٶ�����
  cmd = I2CCMD_MEASURE_HUMIDITY;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  HalI2CDisable();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );  
  value = ( ((value*15625)>>13)-6000 );
  if(value < 0)
    value = 0;
  else if(value > 100000)
    value = 100000;
  return value;
}

//��ʪ��֮����¶ȣ����� ���¶�*1000��
extern long SI7021_ReadTemperature()
{
  HalI2CEnable();  
  
  cmd = I2CCMD_READ_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  HalI2CDisable();  
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*21965)>>13)-46850;  
  return value;
}

//���¶ȣ����� (�����¶�*1000)
extern long SI7021_MeasureTemperature()
{
  HalI2CEnable();   
  
  cmd = I2CCMD_MEASURE_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  HalI2CDisable();   
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*21965)>>13)-46850;  
  return value;  
}

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure()
{
  SI7021_HumiAndTemp rtn;
  rtn.Humidity = SI7021_MeasureHumidity();
  rtn.Temperature = SI7021_ReadTemperature();
  return rtn;
}

//ֹͣ
extern void SI7021_Stop()
{
  HalI2CDisable();
}