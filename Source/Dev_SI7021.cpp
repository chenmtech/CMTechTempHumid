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
#include "osal.h"

/*
* ����
*/
#define I2C_ADDR 0x40   //Si7021��I2C��ַ

// I2C����
#define I2CCMD_MEASURE_HUMIDITY	    0xE5      //��ʪ��

#define I2CCMD_READ_TEMPERATURE	    0xE0      //������ʪ��֮����¶�

#define I2CCMD_MEASURE_TEMPERATURE  0xE3      //���¶�

/*
* �ֲ�����
*/
// ���͵�����ֵ
static uint8 cmd = 0;

// ��ȡ��2�ֽ�����
static uint8 data[2] = {0};

/*
* �ֲ�����
*/

//����������Slave Address��SCLKƵ��
static void SI7021_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

//ֹͣSI7021
static void SI7021_Stop()
{
  HalI2CDisable();
}

/*
* ��������
*/

//����ʪ��ֵ
extern float SI7021_MeasureHumidity()
{
  SI7021_Start();
  
  //�ȷ�����ٶ�����
  cmd = I2CCMD_MEASURE_HUMIDITY;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );  
  value = ( ((value*15625)>>13)-6000 );
  if(value < 0)
    value = 0;
  else if(value > 100000)
    value = 100000;
  return (float)value/1000.0;
}

//��ʪ��֮����¶�
extern float SI7021_ReadTemperature()
{
  SI7021_Start();
  
  cmd = I2CCMD_READ_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*21965)>>13)-46850;  
  return (float)value/1000.0;
}

//���¶�
extern float SI7021_MeasureTemperature()
{
  SI7021_Start();
  
  cmd = I2CCMD_MEASURE_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*21965)>>13)-46850;  
  return (float)value/1000.0;  
}

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure()
{
  SI7021_HumiAndTemp rtn;
  rtn.humid = SI7021_MeasureHumidity();
  rtn.temp = SI7021_ReadTemperature();
  return rtn;
}

// ͬʱ��ʪ�Ⱥ��¶ȣ�����������
extern void SI7021_MeasureData(uint8* pData)
{
  float humid = SI7021_MeasureHumidity();
  float temp = SI7021_ReadTemperature();
  uint8* pt = osal_memcpy(pData, (uint8*)&humid, sizeof(float));
  osal_memcpy(pt, (uint8*)&temp, sizeof(float));
}