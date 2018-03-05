


#include "hal_adc.h"
#include "Dev_BlackHead.h"

static bool active = FALSE;

extern void BlackHead_Init()
{ 
  //P0SEL &= ~(1<<1);   //p0_1GPIO
  //P0DIR |= (1<<1);    //p0_1为输出
  //P0 &= ~(1<<1);      //p0_1为低电平
  
  //HalAdcSetReference( HAL_ADC_REF_AVDD );  //设置采用电源电压作为参考电压
  HalAdcSetReference( HAL_ADC_REF_DIFF );
             
  active = FALSE;
}

extern void BlackHead_Start()
{
  if(!active)
  {
    //P0_1 = 1;                   //p0_1为高电平   
    active = TRUE;
  } 
}
             
extern void BlackHead_Stop()
{
  if(active)
  {
    //P0_1 = 0;                   //p0_1为低电平   
    active = FALSE;
  }   
}
             
extern uint16 BlackHead_ReadTemperature()
{
  BlackHead_Start();
  
  uint16 sum = 0;
  for(int i = 0; i < 16; i++)
  {
    // ADC通道5，采用14位分辨率，实际ENOB为12位
    sum += HalAdcRead (HAL_ADC_CHN_A4A5, HAL_ADC_RESOLUTION_12);
  }
  //sum /= 8;
  sum /= 16;
  
  sum = (sum>>3);
    
  return sum;
}