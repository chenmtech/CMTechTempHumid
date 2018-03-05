


#include "hal_adc.h"
#include "Dev_LMT70.h"

static bool active = FALSE;

extern void Lmt70_Init()
{ 
  P0SEL &= ~(1<<1);   //p0_1GPIO
  P0DIR |= (1<<1);    //p0_1为输出
  P0 &= ~(1<<1);      //p0_1为低电平
  
  HalAdcSetReference( HAL_ADC_REF_125V );  //设置采用内部参考电压，对于CC2541为1.24V
             
  active = FALSE;
}

extern void Lmt70_Start()
{
  if(!active)
  {
    P0_1 = 1;                   //p0_1为高电平   
    active = TRUE;
  } 
}
             
extern void Lmt70_Stop()
{
  if(active)
  {
    P0_1 = 0;                   //p0_1为低电平   
    active = FALSE;
  }   
}
             
extern uint16 Lmt70_ReadTemperature()
{
  Lmt70_Start();
  
  // ADC通道5，采用14位分辨率，实际ENOB为12位
  uint16 sample = HalAdcRead (HAL_ADC_CHANNEL_5, HAL_ADC_RESOLUTION_14);
    
  return sample;
}