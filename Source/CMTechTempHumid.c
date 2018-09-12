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

#include "hal_queue.h"

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

// 缺省采样周期，5秒
#define DEFAULT_PERIOD         5000


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

// 定时周期，单位：分钟
static uint8 timerPeriod = 30;

typedef enum  
{  
  PAIRSTATUS_PAIRED = 0,  
  PAIRSTATUS_NO_PAIRED,  
}PAIRSTATUS;  

static PAIRSTATUS pairStatus = PAIRSTATUS_NO_PAIRED;       //配对状态，默认是没配对

/*********************************************************************
 * 局部函数
*/
// OSAL消息处理函数
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg );

// 状态变化通知回调函数
static void peripheralStateNotificationCB( gaprole_States_t newState );

static void processPasscodeCB(uint8 *deviceAddr, uint16 connectionHandle, uint8 uiInputs, uint8 uiOutputs);

static void processPairStateCB(uint16 connHandle, uint8 state, uint8 status);

// 温湿度服务回调函数
static void tempHumidServiceCB( uint8 paramID );

// 定时服务回调函数
static void timerServiceCB( uint8 paramID );

// 配对密码服务回调函数
static void pairPwdServiceCB( uint8 paramID );



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
  processPasscodeCB,                     // Passcode callback
  processPairStateCB                     // Pairing / Bonding state Callback
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

// 配对密码服务回调结构体实例，结构体是Serice_PairPwd中声明的
static pairPwdServiceCBs_t pairPwd_ServCBs =
{
  pairPwdServiceCB    // 配对密码服务回调函数实例，函数是Serice_Timer中声明的
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
  GAPConfig_SetConnParam(80, 120, 0, 1000, 1);
  //GAPConfig_SetConnParam(100, 100, 1, 2000, 1);

  //配置GGS，设置设备名
  GAPConfig_SetGGSParam(attDeviceName);

  //配置配对和绑定参数
  //GAPConfig_SetPairBondingParam(GAPBOND_PAIRING_MODE_INITIATE, TRUE);
  GAPConfig_SetPairBondingParam(GAPBOND_PAIRING_MODE_WAIT_FOR_REQ, TRUE);
  
  // 每次重启，将密码修改为"000000"
  //GapConfig_WritePairPassword(0L);

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif
  
  GATTConfig_SetTempHumidService(&tempHumid_ServCBs);  
  
  GATTConfig_SetTimerService(&timer_ServCBs);  
  
  GATTConfig_SetPairPwdService(&pairPwd_ServCBs);


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
  
  // 定时服务事件
  if ( events & TEMPHUMID_START_TIMER_EVT )
  {
    uint8 value[4] = {0};
    // 更新当前时间
    Timer_GetParameter(TIMER_VALUE, value);
    value[1] += timerPeriod;
    if(value[1] >= 60) {
      value[1] -= 60;
      value[0]++;
      if(value[0] == 24) {
        value[0] = 0;
      }
    }
    Timer_SetParameter(TIMER_VALUE, 4, value);

    osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT, timerPeriod*60000L );
    
    tempHumidReadAndStoreData();

    return (events ^ TEMPHUMID_START_TIMER_EVT);
  }  
  
  // 修改配对密码事件
  if ( events & TEMPHUMID_CHANGE_PAIRPWD_EVT )
  {
    GAPConfig_TerminateConn();

    return (events ^ TEMPHUMID_CHANGE_PAIRPWD_EVT);
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
      
      tempHumidReadAndTransferData();
      
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
        GAPConfig_EnableAdv(TRUE);
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

// 主机请求密码时的回调
static void processPasscodeCB(uint8 *deviceAddr, uint16 connectionHandle, uint8 uiInputs, uint8 uiOutputs)    
{    
  //读密码  
  uint32 password = 0L;
  GapConfig_SNV_Password(GAP_PARI_PASSWORD_READ, (uint8 *)(&password), sizeof(uint32));  
    
  //发送密码响应给主机  
  GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, password);  
} 

static void processPairStateCB(uint16 connHandle, uint8 state, uint8 status)  
{  
  //主机发起连接，会进入开始配对状态  
  if(state == GAPBOND_PAIRING_STATE_STARTED)  
  {  
    pairStatus = PAIRSTATUS_NO_PAIRED;  
  }  
    
  //当主机提交密码后，会进入配对完成状态    
  else if(state == GAPBOND_PAIRING_STATE_COMPLETE)  
  {  
    //配对成功  
    if(status == SUCCESS)      
    { 
      pairStatus = PAIRSTATUS_PAIRED;  
    }  
      
    //已配对过  
    else if(status == SMP_PAIRING_FAILED_UNSPECIFIED)  
    {       
      pairStatus = PAIRSTATUS_PAIRED;  
    }  
      
    //配对失败  
    else  
    {  
      pairStatus = PAIRSTATUS_NO_PAIRED;  
    }  
      
    //配对失败则断开连接  
    if(pairStatus == PAIRSTATUS_NO_PAIRED)  
    {  
      GAPRole_TerminateConnection();  
    }  
  }  
  else if (state == GAPBOND_PAIRING_STATE_BONDED)  
  {
    
  }  
} 



static void tempHumidServiceCB( uint8 paramID )
{
  uint8 newValue;
  uint8 time[2] = {0};
  uint8 data[8] = {-1};
  float invalidValue = -1.0;

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
      
    case TEMPHUMID_HISTORYTIME:
      TempHumid_GetParameter(TEMPHUMID_HISTORYTIME, time);
      osal_memcpy(data, (uint8*)&invalidValue, sizeof(float));
      osal_memcpy(data+4, (uint8*)&invalidValue, sizeof(float));
      Queue_GetDataAtTime(data, time);
      TempHumid_SetParameter(TEMPHUMID_HISTORYDATA, 8, data);
    
      break;
    default:
      // Should not get here
      break;
  }
}

static void timerServiceCB( uint8 paramID )
{
  uint8 value[4];

  switch (paramID)
  {
    case TIMER_VALUE:
      Timer_GetParameter(TIMER_VALUE, value);
      
      if(value[3] == 0x00) {
        osal_stop_timerEx( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT);  // 停止计时服务
      } else {
        if(value[2] >= 1 && value[2] <= 30) // 计时周期必须在1-30分钟
        {
          timerPeriod = value[2];   // 更新计时周期
          uint8 tmp = value[1]%timerPeriod;
          value[1] -= tmp;          // 调整minute
          Timer_SetParameter(TIMER_VALUE, 4, value);    // 更新计时特征值
          Queue_Init();
          osal_start_reload_timer( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT, (timerPeriod-tmp)*60000L );
        }
      }
      
      break;
      
    default:
      // Should not get here
      break;
  }
}

// 配对密码服务回调函数
static void pairPwdServiceCB( uint8 paramID )
{
  switch (paramID)
  {
    case PAIRPWD_VALUE:
      // 修改配对密码，1秒后断开连接
      osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_CHANGE_PAIRPWD_EVT, 1000 ); 
      
      break;
      
    default:
      // Should not get here
      break;
  }  
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
  uint8 data[TEMPHUMID_DATA_LEN] = {0};
  SI7021_MeasureData(data);

  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, data); 
}

// 读取并保存当前温湿度数据
static void tempHumidReadAndStoreData()
{
  uint8 value[4] = {0};
  Timer_GetParameter(TIMER_VALUE, value);
  uint8 data[10] = {0};
  data[0] = value[0];
  data[1] = value[1];
  
  // 前两次不稳定，获取第三次数据
  SI7021_Measure();
  SI7021_Measure();
  SI7021_MeasureData(data+2);
  
  //加入保存数据的代码
  Queue_Push(data);
}



/*********************************************************************
*********************************************************************/
