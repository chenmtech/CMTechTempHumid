
#include "Dev_HT1621B.h"

//数字0～9的笔段码
//不带小数点{0,1,2,3,4,5,6,7,8,9}
static uint8 Digital[10] = {0xbe,0X06,0X7c,0X5e,0Xc6,0Xda,0Xfa,0X0e,0Xfe,0Xde};
//带小数点{0.,1.,2.,3.,4.,5.,6.,7.,8.,9.}
static uint8 Digital_Dot[10] = {0Xbf,0X07,0X7d,0X5f,0Xc7,0Xdb,0Xfb,0X0f,0Xff,0Xdf};

#define LCODE 0xb0      //"L"的段码
#define HCODE 0xe6      //"H"的段码

// 电源管脚配置为P1.0
#define HT1621_VDD_LOW  P1 &= ~(1<<0)
#define HT1621_VDD_HIGH P1 |= (1<<0)

// CS配置为P0_4
#define HT1621_CS_LOW   P0 &= ~(1 << 4)
#define HT1621_CS_HIGH  P0 |= (1 << 4)
// WR配置为P0_5
#define HT1621_WR_LOW   P0 &= ~(1 << 5)
#define HT1621_WR_HIGH  P0 |= (1 << 5)
// DAT配置为P0_6
#define HT1621_DATA_LOW   P0 &= ~(1 << 6)
#define HT1621_DATA_HIGH  P0 |= (1 << 6)


//commond ID
#define	RD_CMD_ID		0XC0
#define WR_CMD_ID		0XA0
#define	CMD_MODE_ID		0X80
//commond type
#define	SYS_DIS_CMD		0X00
#define	SYS_EN_CMD		0X01
#define	LCD_OFF_CMD		0X02
#define	LCD_ON_CMD		0X03
#define	TMR_DIS_CMD		0X04
#define	WDT_DIS_CMD		0X05
#define	TMR_EN_CMD		0X06
#define	WDT_EN_CMD		0X07
#define	TONE_OFF_CMD	        0X08
#define	TONE_ON_CMD		0X09
#define	CLR_TMR_CMD		0X0C
#define	CLR_WDT_CMD		0X0E
#define	XT_32K_CMD		0X14
#define	RC_256K_CMD		0X18
#define	EXT_256K_CMD	0X1C
#define	BIAS_Half_CMD	0X28	//Attention:4MUX  LCD，1/2偏压，4个公共口
#define	BIAS_TRI_CMD	0X29	//Attention:4MUX  LCD，1/3偏压，4个公共口
#define	TONE_4K_CMD		0X40
#define	TONE_2K_CMD		0X60
#define	IRQ_DIS_CMD		0X80
#define	IRQ_EN_CMD		0X88
#define	NORMOAL_CMD		0XE3


/* Ative delay: 125 cycles ~1 msec，这个不太准 */
#define ST_HAL_DELAY(n) st( { volatile uint32 i; for (i=0; i<(n); i++) { }; } )

// LCD是否打开
static bool isLCDOn = false;

// 写指定长度的bit位
static void HT1621B_WR_nbit_HL(uint8 dat, uint8 nbit);


/**************************************************************************************************
函数名称:static void WR_HT1621B_nbit_HL(uint8 dat, uint8 nbit)
函数功能: 向HT1621B写入指定字节的前几位，MSB.....LSB, 高位在前, 低位在后
入口参数: 无
返回参数: 无
**************************************************************************************************/
static void HT1621B_WR_nbit_HL(uint8 dat, uint8 nbit)
{
  uint8 i;
  for(i=0; i<nbit; i++)
  {
    HT1621_WR_LOW;
    
    if((dat & 0x80) == 0)
      HT1621_DATA_LOW;
    else
      HT1621_DATA_HIGH;

    HT1621_WR_HIGH;
    
    dat<<=1;		
  }  	
}




///////////////////////////////////////////// 外部函数

// 初始化HT IO端口
// 将VDD, CS, WR, DATA都设为GPIO输出低电平，以便降低功耗
extern void HT1621B_InitToPowerDown()
{
  // P1.0 电源管脚配置
  P1SEL &= ~(1<<0);   // 设置为GPIO
  HT1621_VDD_LOW;    // 初始为低电平
  P1DIR |= (1<<0);    // 设置为输出
  
  //P0.4 CS管脚配置
  P0SEL &= ~(1<<4); //设置为GPIO
  HT1621_CS_LOW;   //初始为低电平
  P0DIR |= (1<<4);  //设置为输出  
  
  //P0.5 WR管脚配置
  P0SEL &= ~(1<<5); //设置为GPIO
  HT1621_WR_LOW;   //初始为低电平
  P0DIR |= (1<<5);  //设置为输出  
  
  //P0.6 DATA管脚配置
  P0SEL &= ~(1<<6);   //设置为GPIO
  HT1621_DATA_LOW;   //初始为低电平
  P0DIR |= (1<<6);    //设置为输出    
  
  isLCDOn = false;
}


/**************************************************************************************************
函数名称: void HT1621B_Init_LCD (void)
函数功能: 给液晶上电, 驱动频率256Hz,系统开, LCD开, 1/3偏压, 4MUX
入口参数: 无
返回参数: 无
**************************************************************************************************/
extern void HT1621B_TurnOnLCD (void)
{ 
  if(isLCDOn) return;
  
  // 上电
  HT1621_VDD_HIGH;
  ST_HAL_DELAY(125);  // 延时  
  
  // 需要CS有一个高电平脉冲进行通信初始化
  HT1621_CS_HIGH;
  ST_HAL_DELAY(250);  // 延时
  
  HT1621_CS_LOW;
  //
  HT1621B_WR_nbit_HL(CMD_MODE_ID, 3);	// 写入命令模式100
  HT1621B_WR_nbit_HL(SYS_EN_CMD, 9);   	//
  HT1621B_WR_nbit_HL(LCD_ON_CMD, 9);   	//
  HT1621B_WR_nbit_HL(RC_256K_CMD, 9);   //
  HT1621B_WR_nbit_HL(BIAS_TRI_CMD, 9); 	//
  HT1621B_WR_nbit_HL(TONE_4K_CMD, 9); 	//
  //
  HT1621_CS_HIGH;
  
  ST_HAL_DELAY(125);  // 延时  
  isLCDOn = true;
}



// 关LCD，并回到低功耗状态
extern void HT1621B_TurnOffLCD(void)
{
  if(isLCDOn)
  {
    HT1621_CS_LOW;
    
    HT1621B_WR_nbit_HL(CMD_MODE_ID, 3);
    HT1621B_WR_nbit_HL(LCD_OFF_CMD, 9); 
    HT1621B_WR_nbit_HL(SYS_DIS_CMD, 9);
    
    HT1621_CS_HIGH;  
    
    ST_HAL_DELAY(125);  // 延时
    
    HT1621B_InitToPowerDown();   // 回到低功耗状态
    
    isLCDOn = false;
  }
}

/**************************************************************************************************
函数名称:void LCD_CLR(void)
函数功能: 清零显示屏
入口参数: 无
返回参数: 无
**************************************************************************************************/
extern void HT1621B_ClearLCD(void)
{
  uint8 i;
  HT1621_CS_LOW;
  HT1621B_WR_nbit_HL(WR_CMD_ID,3);	//
  HT1621B_WR_nbit_HL(0x00,6);		//6位开始地址, 此处为"000000"
  //采用连续写入方式写入16字节0x00
  for(i=0;i<16;i++)
  {
    HT1621B_WR_nbit_HL(0x00,8);
  }
  HT1621_CS_HIGH;
}


// 在指定液晶位置写一个字节段码
extern void HT1621B_WR_ONEBYTE(uint8 location, uint8 segCode)
{
  HT1621_CS_LOW;
  //
  HT1621B_WR_nbit_HL(WR_CMD_ID, 3);			// 写命令
  HT1621B_WR_nbit_HL(location<<2, 6);	 		//因HT1621B只需要6bit的地址，所以地址要左移2位
  HT1621B_WR_nbit_HL(segCode, 8);
  //
  HT1621_CS_HIGH;	  
}


/**************************************************************************************************
函数名称: extern void WR_LCD_MEM(uint8 location, uint8 * Seg_Buf, uint8 nWord)
函数功能: 写H1621B缓存
入口参数: 无
返回参数: 
**************************************************************************************************/
extern void HT1621B_WR_LCD_MEM(uint8 location, uint8 * Seg_Buf, uint8 nWord)
{
  int i;
  HT1621_CS_LOW;
  //
  HT1621B_WR_nbit_HL(WR_CMD_ID, 3);			// 写命令
  HT1621B_WR_nbit_HL(location<<2, 6);	 		//因HT1621B只需要6bit的地址，所以地址要左移2位
  for(i=0; i<nWord; i++)
  {
    HT1621B_WR_nbit_HL(*Seg_Buf++, 8);		//   
  }
  //
  HT1621_CS_HIGH;	
}


// 显示一个uint16的十进制数据
extern void HT1621B_ShowUint16Data(uint8 location, uint16 data)
{
  if(!isLCDOn) return;
  
  uint16 UNIT[5] = {10000, 1000, 100, 10, 1};
  uint8 code[5] = {0};
  uint8 num = 0;
  for(uint8 i = 0; i < 5; i++)
  {
    num = data / UNIT[i];
    data = data % UNIT[i];
    code[i] = Digital[num];
  }
  HT1621B_WR_LCD_MEM(location, code, 5);
}

// 显示一个温度，带小数点
extern void HT1621B_ShowTemperature(uint8 location, uint16 temp, bool isPreTemp)
{
  if(!isLCDOn) return;
  
  uint16 UNIT[4] = {1000, 100, 10, 1};
  uint8 code[5] = {0};
  uint8 num = 0;
  for(uint8 i = 0; i < 4; i++)
  {
    num = temp / UNIT[i];
    temp = temp % UNIT[i];
    if(i != 1)
      code[i] = Digital[num];
    else  // 个位数，要显示小数点
      code[i] = Digital_Dot[num];
  }
  // 显示摄氏度符号
  if(isPreTemp)
    code[4] = 0x30;   //预测温度
  else
    code[4] = 0x20;   //实际温度
  HT1621B_WR_LCD_MEM(location, code, 5);
}

// 显示"L"
extern void HT1621B_ShowL(uint8 location)
{
  if(!isLCDOn) return;
  
  HT1621B_ClearLCD();
  HT1621B_WR_ONEBYTE(location, LCODE);
}

// 显示"H"
extern void HT1621B_ShowH(uint8 location)
{
  if(!isLCDOn) return;  
  
  HT1621B_ClearLCD();
  HT1621B_WR_ONEBYTE(location, HCODE);
}

// 开蜂鸣器
extern void HT1621B_ToneOn()
{
  if(!isLCDOn) return;  

  HT1621_CS_LOW;
  //
  HT1621B_WR_nbit_HL(CMD_MODE_ID, 3);	// 写入命令模式100
  HT1621B_WR_nbit_HL(TONE_ON_CMD, 9);   //
  //
  HT1621_CS_HIGH;  
}

// 关蜂鸣器
extern void HT1621B_ToneOff()
{
  if(!isLCDOn) return;  

  HT1621_CS_LOW;
  //
  HT1621B_WR_nbit_HL(CMD_MODE_ID, 3);	// 写入命令模式100
  HT1621B_WR_nbit_HL(TONE_OFF_CMD, 9);   //
  //
  HT1621_CS_HIGH;  
}