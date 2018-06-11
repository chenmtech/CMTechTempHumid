/**************************************************************************************************
* CMTechTempHumid.c: 应用主源文件
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"

#include "OnBoard.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "Service_TempHumid.h"

#include "Service_Timer.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#include "gapbondmgr.h"

#include "App_GAPConfig.h"

#include "App_GATTConfig.h"

#include "hal_i2c.h"

#include "Dev_Si7021.h"

#include "CMTechTempHumid.h"



#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif


/*********************************************************************
 * 常量
*/
#define INVALID_CONNHANDLE                    0xFFFF

// 停止采集状态
#define STATUS_STOP           0     

// 开始采集状态
#define STATUS_START          1            

// 缺省采样周期，1秒
#define DEFAULT_PERIOD         1000


/*********************************************************************
 * 局部变量
*/
// 任务ID
static uint8 tempHumid_TaskID;   

// GAP状态
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// GAP GATT 设备名
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "TempHumid";

// 当前状态
static uint8 status = STATUS_STOP;

// 数据采样周期，单位：ms
static uint16 period = DEFAULT_PERIOD;

// 定时周期，单位：ms
static uint32 timerPeriod = 600000L;

/*********************************************************************
 * 局部函数
*/
// OSAL消息处理函数
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg );

// 状态变化通知回调函数
static void peripheralStateNotificationCB( gaprole_States_t newState );

// 温湿度服务回调函数
static void tempHumidServiceCB( uint8 paramID );

// 定时服务回调函数
static void timerServiceCB( uint8 paramID );

// 启动定时
static void timerStart( void );

// 停止定时
static void timerStop( void );

// 初始化
static void tempHumidInit();

// 启动采样
static void tempHumidStart( void );

// 停止采样
static void tempHumidStop( void );

// 读取传输数据
static void tempHumidReadAndTransferData();

// 读取并保存每半小时的数据
static void tempHumidReadAndStoreData();

// 初始化IO管脚
static void tempHumidInitIOPin();


/*********************************************************************
 * PROFILE and SERVICE 回调结构体实例
*/

// GAP Role 回调结构体实例，结构体是协议栈声明的
static gapRolesCBs_t tempHumid_PeripheralCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                            // When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager 回调结构体实例，结构体是协议栈声明的
static gapBondCBs_t tempHumid_BondMgrCBs =
{
  NULL,                     // Passcode callback (not used by application)
  NULL                      // Pairing / Bonding state Callback (not used by application)
};

// 温湿度回调结构体实例，结构体是Serice_TempHumid中声明的
static tempHumidServiceCBs_t tempHumid_ServCBs =
{
  tempHumidServiceCB    // 温湿度服务回调函数实例，函数是Serice_TempHumid中声明的
};

// 定时服务回调结构体实例，结构体是Serice_Timer中声明的
static timerServiceCBs_t timer_ServCBs =
{
  timerServiceCB    // 定时服务回调函数实例，函数是Serice_Timer中声明的
};


/*********************************************************************
 * 公共函数
 */
extern void TempHumid_Init( uint8 task_id )
{
  tempHumid_TaskID = task_id;
  
  //设置发射功率为4dB(可惜CC2541不支持4dB)
  //HCI_EXT_SetTxPowerCmd (HCI_EXT_TX_POWER_4_DBM);  

  // GAP 配置
  //配置广播参数
  GAPConfig_SetAdvParam(2000, TEMPHUMID_SERV_UUID);
  
  // 初始化立刻广播
  GAPConfig_EnableAdv(TRUE);

  //配置连接参数
  GAPConfig_SetConnParam(100, 100, 1, 2000, 1);

  //配置GGS，设置设备名
  GAPConfig_SetGGSParam(attDeviceName);

  //配置绑定参数
  GAPConfig_SetBondingParam(0, GAPBOND_PAIRING_MODE_WAIT_FOR_REQ);

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif
  
  GATTConfig_SetTimerService(&timer_ServCBs);
  
  GATTConfig_SetTempHumidService(&tempHumid_ServCBs);

  //在这里初始化GPIO
  //第一：所有管脚，reset后的状态都是输入加上拉
  //第二：对于不用的IO，建议不连接到外部电路，且设为输入上拉
  //第三：对于会用到的IO，就要根据具体外部电路连接情况进行有效设置，防止耗电
  {
    tempHumidInitIOPin();
  }
  
  // 初始化
  tempHumidInit();  
 
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // 启动设备
  osal_set_event( tempHumid_TaskID, TEMPHUMID_START_DEVICE_EVT );
}


// 初始化IO管脚
static void tempHumidInitIOPin()
{
  // 全部设为GPIO
  P0SEL = 0; 
  P1SEL = 0; 
  P2SEL = 0; 

  // 全部设为输出低电平
  P0DIR = 0xFF; 
  P1DIR = 0xFF; 
  P2DIR = 0x1F; 

  P0 = 0; 
  P1 = 0;   
  P2 = 0;  
  
  // I2C的SDA, SCL设置为GPIO, 输出低电平，否则功耗很大
  HalI2CSetAsGPIO();
}

extern uint16 TempHumid_ProcessEvent( uint8 task_id, uint16 events )
{
  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( tempHumid_TaskID )) != NULL )
    {
      tempHumid_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & TEMPHUMID_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &tempHumid_PeripheralCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &tempHumid_BondMgrCBs );

    return ( events ^ TEMPHUMID_START_DEVICE_EVT );
  }
  
  if ( events & TEMPHUMID_START_PERIODIC_EVT )
  {
    tempHumidReadAndTransferData();

    if(status == STATUS_START)
      osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_PERIODIC_EVT, period );

    return (events ^ TEMPHUMID_START_PERIODIC_EVT);
  }
  
  if ( events & TEMPHUMID_START_TIMER_EVT )
  {
    uint8 time[2] = {0};
    Timer_GetParameter(TIMER_CURTIME, time);
    uint8 minPeriod = 0;
    Timer_GetParameter(TIMER_PERIOD, &minPeriod);
    time[1] += minPeriod;
    if(time[1] >= 60) {
      time[1] -= 60;
      time[0]++;
      if(time[0] == 24) {
        time[0] = 0;
      }
    }
    
    Timer_SetParameter(TIMER_CURTIME, 2, time);

    osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT, timerPeriod );

    return (events ^ TEMPHUMID_START_TIMER_EVT);
  }  
  

  // Discard unknown events
  return 0;
}




/*********************************************************************
 * @fn      simpleBLEPeripheral_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    default:
      // do nothing
      break;
  }
}


static void peripheralStateNotificationCB( gaprole_States_t newState )
{
  switch ( newState )
  {
    case GAPROLE_STARTED:
      {
        uint8 ownAddress[B_ADDR_LEN];
        uint8 systemId[DEVINFO_SYSTEM_ID_LEN];

        GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

        // use 6 bytes of device address for 8 bytes of system ID value
        systemId[0] = ownAddress[0];
        systemId[1] = ownAddress[1];
        systemId[2] = ownAddress[2];

        // set middle bytes to zero
        systemId[4] = 0x00;
        systemId[3] = 0x00;

        // shift three bytes up
        systemId[7] = ownAddress[5];
        systemId[6] = ownAddress[4];
        systemId[5] = ownAddress[3];

        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
      }
      break;

    case GAPROLE_ADVERTISING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Advertising",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

    case GAPROLE_CONNECTED:
      tempHumidInit();  
      
      break;

    case GAPROLE_WAITING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Disconnected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
          
        // 断开连接时，停止AD采集
        tempHumidStop();
        
        // I2C的SDA, SCL设置为GPIO, 输出低电平，否则功耗很大
        HalI2CSetAsGPIO();

      }
      break;

    case GAPROLE_WAITING_AFTER_TIMEOUT:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Timed Out",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

    case GAPROLE_ERROR:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Error",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

    default:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

  }
  
  gapProfileState = newState;

}


static void tempHumidServiceCB( uint8 paramID )
{
  uint8 newValue;

  switch (paramID)
  {
    case TEMPHUMID_CTRL:
      TempHumid_GetParameter( TEMPHUMID_CTRL, &newValue );
      
      // 停止采集
      if ( newValue == TEMPHUMID_CTRL_STOP)  
      {
        tempHumidStop();
      }
      // 开始采集
      else if ( newValue == TEMPHUMID_CTRL_START) 
      {
        tempHumidStart();
      }
      
      break;

    case TEMPHUMID_PERI:
      TempHumid_GetParameter( TEMPHUMID_PERI, &newValue );
      period = newValue*TEMPHUMID_PERIOD_UNIT;

      break;

    default:
      // Should not get here
      break;
  }
}

static void timerServiceCB( uint8 paramID )
{
  uint8 newValue;

  switch (paramID)
  {
    case TIMER_CTRL:
      Timer_GetParameter( TIMER_CTRL, &newValue );
      
      // 停止定时
      if ( newValue == TIMER_CTRL_STOP)  
      {
        timerStop();
      }
      // 开始定时
      else if ( newValue == TIMER_CTRL_START) 
      {
        timerStart();
      }
      
      break;

    case TIMER_PERIOD:
      Timer_GetParameter( TIMER_PERIOD, &newValue );
      timerPeriod = newValue*60000L;

      break;

    default:
      // Should not get here
      break;
  }
}

// 启动定时
static void timerStart( void )
{  
  osal_start_reload_timer( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT, timerPeriod );
}

// 停止定时
static void timerStop( void )
{  
  osal_stop_timerEx( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT);
}


// 初始化温湿度服务参数
static void tempHumidInit()
{
  status = STATUS_STOP;

  uint8 tempHumidData[TEMPHUMID_DATA_LEN] = { 0 };
  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, tempHumidData );
  
  // 停止采集
  uint8 tempHumidCtrl = TEMPHUMID_CTRL_STOP;
  TempHumid_SetParameter( TEMPHUMID_CTRL, sizeof(uint8), &tempHumidCtrl );  
  
  // 设置传输周期
  uint8 tmp = DEFAULT_PERIOD/TEMPHUMID_PERIOD_UNIT;
  TempHumid_SetParameter( TEMPHUMID_PERI, sizeof(uint8), (uint8*)&tmp ); 
}

// 启动采样
static void tempHumidStart( void )
{  
  if(status == STATUS_STOP) {
    status = STATUS_START;
    osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_PERIODIC_EVT, period);
  }
}

// 停止采样
static void tempHumidStop( void )
{  
  status = STATUS_STOP;
}

// 读取传输数据
static void tempHumidReadAndTransferData()
{
  SI7021_HumiAndTemp humidTemp = SI7021_Measure();
  uint8 data[TEMPHUMID_DATA_LEN] = {0};
  uint8* pt = (uint8*)(&(humidTemp.humid));
  for(int i = 0; i < sizeof(float); i++) 
    data[i] = *pt++;
  pt = (uint8*)(&(humidTemp.temp));
  for(int i = 0; i < sizeof(float); i++) 
    data[i+4] = *pt++;
  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, (uint8*)data); 
}

// 读取并保存每半小时的数据
static void tempHumidReadAndStoreData()
{
  SI7021_HumiAndTemp humidTemp = SI7021_Measure();
  uint8 data[TEMPHUMID_DATA_LEN] = {0};
  uint8* pt = (uint8*)(&(humidTemp.humid));
  for(int i = 0; i < sizeof(float); i++) 
    data[i] = *pt++;
  pt = (uint8*)(&(humidTemp.temp));
  for(int i = 0; i < sizeof(float); i++) 
    data[i+4] = *pt++;
  
  //加入保存数据的代码
  
}



/*********************************************************************
*********************************************************************/
