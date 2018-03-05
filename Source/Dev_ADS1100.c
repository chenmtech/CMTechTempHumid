
#include "Dev_ADS1100.h"
#include "hal_i2c.h"

/* Ative delay: 125 cycles ~1 msec */
#define ST_HAL_DELAY(n) st( { volatile uint32 i; for (i=0; i<(n); i++) { }; } )

#define ADS1100_I2C_ADDR 0x48   //AD0的7位地址,0b1001000

#define ADS1100_CONFIG  0x1C    //初始配置，0b0001 1100，即：single conversion模式，DR=8SPS, PGA=1

#define ADS1100_START_CMD  0x9C  //启动采样，0b1001 1100

// 电源管脚配置为P1.1
#define ADS1100_VDD_LOW  P1 &= ~(1<<1)
#define ADS1100_VDD_HIGH P1 |= (1<<1)



static uint8 buf[3] = {0};

static bool isADOn = false;


////////////////////////////////////////////////////////////////////////////


// 配置寄存器为初始状态
static void config();

// 启动单次转换
static void convert();




// 初始化
extern void ADS1100_Init()
{
  ADS1100_TurnOn();
  
  HalI2CInit(ADS1100_I2C_ADDR, i2cClock_123KHZ);
  config();
  convert();
  
  ADS1100_TurnOff();
}


// 打开ADS1100电源
extern void ADS1100_TurnOn()
{
  if(!isADOn)
  {
    // P1.0 电源管脚配置
    P1SEL &= ~(1<<1);   // 设置为GPIO
    ADS1100_VDD_HIGH;   // 设置为高电平
    P1DIR |= (1<<1);    // 设置为输出  
    
    ST_HAL_DELAY(3000);   // 延时等待电源稳定，不延时会出莫名其妙的问题
    
    isADOn = true;
  }
}

// 关闭ADS1100电源
extern void ADS1100_TurnOff()
{
  if(isADOn)
  {
    // 一定要把I2C的接口设为GPIO才能省电
    HalI2CWrapperEnable();
  
  // P1.0 电源管脚配置
    P1SEL &= ~(1<<1);   // 设置为GPIO
    ADS1100_VDD_LOW;    // 设置为低电平
    P1DIR |= (1<<1);    // 设置为输出  
    
    ST_HAL_DELAY(3000);   // 延时，等待电源稳定
  
    isADOn = false;  
  }
}

// 获取AD转换值
// 返回SUCCESS或者FAILURE
extern uint8 ADS1100_GetADValue(uint16 * pData)
{
  // 如果没有打开AD，则打开，但是本次读取为异常
  if(!isADOn)
  {
    ADS1100_TurnOn();
    HalI2CInit(ADS1100_I2C_ADDR, i2cClock_123KHZ);
    HalI2CRead(3, buf);     //把错误的读出来丢掉
    convert();
    *pData = 0;
    return FAILURE;   // 返回AD错误
  }
  
  HalI2CInit(ADS1100_I2C_ADDR, i2cClock_123KHZ);  
  // 只能读上一次的值
  HalI2CRead(3, buf);  
  // 启动本次采样
  convert();
  
  *pData = (((uint16)buf[0]) << 8) + ((uint16)buf[1]);
  
  return SUCCESS;
}



static void config()
{
  uint8 data = ADS1100_CONFIG;
  HalI2CWrite(1, &data);
}

static void convert()
{
  uint8 data = ADS1100_START_CMD;
  HalI2CWrite(1, &data);  
}
