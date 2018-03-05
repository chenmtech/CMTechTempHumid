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
#include "hal_key.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "Service_TempHumid.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#include "gapbondmgr.h"

#include "App_GAPConfig.h"

#include "App_GATTConfig.h"

#include "App_TempHumid.h"

#include "CMTechTempHumid.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

#include "App_DataProcessor.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */


#define INVALID_CONNHANDLE                    0xFFFF

#if defined ( PLUS_BROADCASTER )
  #define ADV_IN_CONN_WAIT                    500 // delay 500 ms
#endif


// 两种模式：开机模式和待机模式
#define MODE_ACTIVE         0             // 表示开机模式
#define MODE_STANDBY        1             // 表示待机模式


// 显示上次最大温度值的持续时间，默认3秒
#define THERMO_LASTMAXTEMP_SHOWTIME           3000

// 显示预测温度值的持续时间，20秒
#define THERMO_SHOW_PRETEMP                   20000

// 默认的传输周期，每1秒传输一个数据
#define DEFAULT_TRANSMIT_PERIOD         1

/* Ative delay: 125 cycles ~1 msec */
#define ST_HAL_DELAY(n) st( { volatile uint32 i; for (i=0; i<(n); i++) { }; } )


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8 CMTechThermometer_TaskID;   // Task ID for internal task/event processing

static gaprole_States_t gapProfileState = GAPROLE_INIT;


// 当前工作模式，初始化为待机模式
static uint8 curMode = MODE_STANDBY;

// 是否开始AD采集，初始化为停止采集
static bool thermoADEnabled = FALSE;

// 数据采集周期，固定为传输的计时单位1秒
static uint16 ADPeriod = THERMOMETER_TIME_UNIT;

// 蓝牙数据传输周期，用ADPeriod的倍数来表示，默认为1倍
static uint8 transNumOfADPeriod = DEFAULT_TRANSMIT_PERIOD;

// 用于记录采样次数，是否需要通过蓝牙发送数据
static uint8 haveSendNum = 0;

// 蜂鸣器响声次数
static uint8 bzTimes = 0;



/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void CMTechThermometer_ProcessOSALMsg( osal_event_hdr_t *pMsg );

static void CMTechThermometer_HandleKeys( uint8 shift, uint8 keys );

static void peripheralStateNotificationCB( gaprole_States_t newState );

static void thermometerServiceCB( uint8 paramID );

// 初始化为待机模式
static void initAsStandbyMode();

// 从待机模式转换到开机模式
static void switchFromStandbyToActive( void );

// 从开机模式转换到待机模式
static void switchFromActiveToStandby( void );

// 读取并处理数据
static void readAndProcessThermoData();

// 初始化IO管脚
static void initIOPin();

// 让蜂鸣器响指定次数
static void turnOnTone(uint8 times);


/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t CMTechThermometer_PeripheralCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                            // When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
static gapBondCBs_t CMTechThermometer_BondMgrCBs =
{
  NULL,                     // Passcode callback (not used by application)
  NULL                      // Pairing / Bonding state Callback (not used by application)
};

static thermometerServiceCBs_t CMTechThermometer_ServCBs =
{
  thermometerServiceCB    // Charactersitic value change callback
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleBLEPeripheral_Init
 *
 * @brief   Initialization function for the Simple BLE Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void CMTechTempHumid_Init( uint8 task_id )
{
  CMTechThermometer_TaskID = task_id;

  // GAP 配置
  //配置广播参数
  GAPConfig_SetAdvParam(800, THERMOMETER_SERV_UUID);
  // 初始化不广播
  GAPConfig_EnableAdv(FALSE);

  //配置连接参数
  GAPConfig_SetConnParam(200, 200, 5, 10000, 1);

  //配置GGS，设置设备名
  GAPConfig_SetGGSParam("Thermometer");

  //配置绑定参数
  GAPConfig_SetBondingParam(0, GAPBOND_PAIRING_MODE_WAIT_FOR_REQ);

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif
  
  GATTConfig_SetThermoService(&CMTechThermometer_ServCBs);

  RegisterForKeys( CMTechThermometer_TaskID );

  //在这里初始化GPIO
  //第一：所有管脚，reset后的状态都是输入加上拉
  //第二：对于不用的IO，建议不连接到外部电路，且设为输入上拉
  //第三：对于会用到的IO，就要根据具体外部电路连接情况进行有效设置，防止耗电
  {
    // For keyfob board set GPIO pins into a power-optimized state
    // Note that there is still some leakage current from the buzzer,
    // accelerometer, LEDs, and buttons on the PCB.
    
    // Register for all key events - This app will handle all key events
    
    initIOPin();
  }

  Thermo_Init();
  
  // 初始化为待机模式
  initAsStandbyMode();  
 
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // Setup a delayed profile startup
  osal_set_event( CMTechThermometer_TaskID, TH_START_DEVICE_EVT );
}


// 初始化IO管脚
static void initIOPin()
{
  // 全部设为GPIO
  P0SEL = 0; // Configure Port 0 as GPIO
  P1SEL = 0; // Configure Port 1 as GPIO
  P2SEL = 0; // Configure Port 2 as GPIO
//
//  // 除了P0.0和P0.1配置为button输入高电平，其他全部设为输出低电平
  P0DIR = 0xFC; // Port 0 pins P0.0 and P0.1 as input (buttons),
//                // all others (P0.1-P0.7) as output
  P1DIR = 0xFF; // All port 1 pins (P1.0-P1.7) as output
  P2DIR = 0x1F; // All port 1 pins (P2.0-P2.4) as output
//  
  P0 = 0x03; // All pins on port 0 to low except for P0.0 and P0.1(buttons)
  P1 = 0;   // All pins on port 1 to low
  P2 = 0;   // All pins on port 2 to low   
  
  // I2C的SDA, SCL设置为GPIO, 输出低电平，否则功耗很大
  I2CWC = 0x83;
  I2CIO = 0x00;
}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_ProcessEvent
 *
 * @brief   Simple BLE Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 CMTechTempHumid_ProcessEvent( uint8 task_id, uint16 events )
{

  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( CMTechThermometer_TaskID )) != NULL )
    {
      CMTechThermometer_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & TH_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &CMTechThermometer_PeripheralCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &CMTechThermometer_BondMgrCBs );
    
    switchFromStandbyToActive();

    return ( events ^ TH_START_DEVICE_EVT );
  }
  
  if ( events & TH_PERIODIC_EVT )
  {
    if(thermoADEnabled)
    {
      readAndProcessThermoData();
      osal_start_timerEx( CMTechThermometer_TaskID, TH_PERIODIC_EVT, ADPeriod );
    }
    else
    {
      // 停止AD采集
      //Thermo_TurnOff_AD();
    }

    return (events ^ TH_PERIODIC_EVT);
  }

  
  // 标定
  if ( events & TH_CALIBRATION_EVT )
  {
    Thermo_DoCalibration();

    return (events ^ TH_CALIBRATION_EVT);
  }  
  
  // 切换工作模式
  if ( events & TH_SWITCH_MODE_EVT )
  {
    if(curMode == MODE_ACTIVE)
    {
      switchFromActiveToStandby();
    }
    else
    {
      switchFromStandbyToActive();
    }    

    return (events ^ TH_SWITCH_MODE_EVT);
  }  
  
  // 预测温度事件
  if ( events & TH_DP_PRECAST_EVT )
  {
    turnOnTone(1);
    
    Thermo_SetPreTemp(getPrecastTemp());
    Thermo_SetShowPreTemp(TRUE);
    
    osal_start_timerEx(CMTechThermometer_TaskID, TH_STOP_SHOW_PRETEMP_EVT, THERMO_SHOW_PRETEMP);
  
    return (events ^ TH_DP_PRECAST_EVT);
  }  
  
  // 停止显示预测温度事件
  if ( events & TH_STOP_SHOW_PRETEMP_EVT )
  {
    Thermo_SetShowPreTemp(FALSE);
  
    return (events ^ TH_STOP_SHOW_PRETEMP_EVT);
  }  
  
  
  // 测温稳定事件
  if ( events & TH_DP_STABLE_EVT )
  {
    turnOnTone(3);
    
    notifyTempStable();
  
    return (events ^ TH_DP_STABLE_EVT);
  }    
  
  if ( events & TH_TONE_ON_EVT )
  {
    Thermo_ToneOn();
    
    osal_start_timerEx(CMTechThermometer_TaskID, TH_TONE_OFF_EVT, 500 );
  
    return (events ^ TH_TONE_ON_EVT);
  }   
  
  if ( events & TH_TONE_OFF_EVT )
  {
    Thermo_ToneOff();
    
    if(--bzTimes > 0)
    {
      osal_start_timerEx(CMTechThermometer_TaskID, TH_TONE_ON_EVT, 500 );
    }
  
    return (events ^ TH_TONE_OFF_EVT);
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
static void CMTechThermometer_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case KEY_CHANGE:
      CMTechThermometer_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
      break;

    default:
      // do nothing
      break;
  }
}



#if defined( CC2540_MINIDK )
/*********************************************************************
 * @fn      CMTechThermometer_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void CMTechThermometer_HandleKeys( uint8 shift, uint8 keys )
{
  VOID shift;  // Intentionally unreferenced parameter

  if ( keys & HAL_KEY_SW_1 )
  {
    osal_set_event( CMTechThermometer_TaskID, TH_SWITCH_MODE_EVT);

  } 
}
#endif // #if defined( CC2540_MINIDK )



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


      break;

    case GAPROLE_WAITING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Disconnected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
          
        // 断开连接时，停止AD采集
        //Thermo_TurnOff_AD();
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


static void thermometerServiceCB( uint8 paramID )
{
  uint8 newValue;

  switch (paramID)
  {
    case THERMOMETER_CONF:
      Thermometer_GetParameter( THERMOMETER_CONF, &newValue );
      
      if ( newValue == THERMOMETER_CONF_STANDBY)  // 停止采集，进入待机模式
      {
        switchFromActiveToStandby();
      }
      
      else if ( newValue == THERMOMETER_CONF_CALIBRATION) // 进行标定
      {
        osal_set_event( CMTechThermometer_TaskID, TH_CALIBRATION_EVT);
      }
      
      else if ( newValue == THERMOMETER_CONF_LCDON) // 开LCD
      {
        Thermo_TurnOn_LCD();
      }   
      
      else if ( newValue == THERMOMETER_CONF_LCDOFF) // 关LCD
      {
        Thermo_TurnOff_LCD();
      }  
      
      else // 剩下的就是设置数据类型
      { 
        Thermo_SetValueType(newValue);
      }
      
      break;

    case THERMOMETER_PERI:
      Thermometer_GetParameter( THERMOMETER_PERI, &newValue );
      transNumOfADPeriod = newValue;
      break;

    default:
      // Should not get here
      break;
  }
}


// 初始化为待机模式
static void initAsStandbyMode()
{
  curMode = MODE_STANDBY;
  
  // 采样周期为1秒
  ADPeriod = THERMOMETER_TIME_UNIT;
  
  // 传输周期为采样周期的1倍
  transNumOfADPeriod = DEFAULT_TRANSMIT_PERIOD;  
  
  // 初始化蓝牙属性
  // 温度为0
  uint8 thermoData[THERMOMETER_DATA_LEN] = { 0, 0 };
  Thermometer_SetParameter( THERMOMETER_DATA, THERMOMETER_DATA_LEN, thermoData );
  
  // 停止采集
  uint8 thermoCfg = THERMOMETER_CONF_STANDBY;
  Thermometer_SetParameter( THERMOMETER_CONF, sizeof(uint8), &thermoCfg );  
  
  // 设置传输周期
  Thermometer_SetParameter( THERMOMETER_PERI, sizeof(uint8), &transNumOfADPeriod ); 
  
  // 停止采样
  thermoADEnabled = FALSE;
}


static void switchFromStandbyToActive( void )
{  
  curMode = MODE_ACTIVE;
  
  // 开硬件
  Thermo_HardwareOn();
  
  // 开始广播
  GAPConfig_EnableAdv(TRUE);    

  // 获取输出值类型  
  uint8 thermoCfg = Thermo_GetValueType();
  Thermometer_SetParameter( THERMOMETER_CONF, sizeof(uint8), &thermoCfg ); 
  
  // 显示上次最大值后，开始AD采样
  thermoADEnabled = TRUE;
  osal_start_timerEx( CMTechThermometer_TaskID, TH_PERIODIC_EVT, THERMO_LASTMAXTEMP_SHOWTIME);
  
  // 重新计数
  haveSendNum = 0;
  
  DP_Init(CMTechThermometer_TaskID);
}


static void switchFromActiveToStandby( void )
{ 
  curMode = MODE_STANDBY;
  
  // 停止AD
  thermoADEnabled = FALSE;
  osal_stop_timerEx( CMTechThermometer_TaskID, TH_PERIODIC_EVT);
  
  // 终止蓝牙连接
  if ( gapProfileState == GAPROLE_CONNECTED )
  {
    GAPConfig_TerminateConn();
  }
  
  // 停止广播
  GAPConfig_EnableAdv(FALSE);   
  
  // 关硬件
  Thermo_HardwareOff();
  
  initIOPin();
}




static void readAndProcessThermoData()
{
  // 获取数据
  uint16 value = Thermo_GetValue();  
  
  // 错误数据，不发送，不显示
  if(value == FAILURE) return;   
  
  // 更新数据最大值
  Thermo_UpdateMaxValue(value);  
  
  // 在液晶屏上显示数据
  Thermo_ShowValueOnLCD(1, value);
  
  // 到了传输周期，就发送蓝牙数据属性值
  if(++haveSendNum >= transNumOfADPeriod)
  {
    Thermometer_SetParameter( THERMOMETER_DATA, THERMOMETER_DATA_LEN, (uint8*)&value); 
    haveSendNum = 0;
  }

  // 如果是温度数据，去处理它
  if(Thermo_GetValueType() == THERMOMETER_CONF_VALUETYPE_T)
  {
    DP_Process(value);
  }
}

// 让蜂鸣器响指定次数
static void turnOnTone(uint8 times)
{
  bzTimes = times;
  osal_set_event(CMTechThermometer_TaskID, TH_TONE_ON_EVT);
}


/*********************************************************************
*********************************************************************/
