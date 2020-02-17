/**************************************************************************************************
* CMTechTempHumid.c: 应用主源文件
**************************************************************************************************/

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"
#include "linkdb.h"
#include "OnBoard.h"
#include "gatt.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "Service_DevInfo.h"
#include "Service_TempHumid.h"
#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif
#include "gapbondmgr.h"
#include "hal_i2c.h"
#include "CMTechTempHumid.h"
#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif


#define INVALID_CONNHANDLE 0xFFFF

#define STATUS_MEAS_STOP  0     
#define STATUS_MEAS_START 1   

#define DEFAULT_MEAS_PERIOD 10 // default T&H measurement period, uints: second
// 
static uint8 taskID;   
static uint16 gapConnHandle = INVALID_CONNHANDLE;
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// advertising data
static uint8 advertData[] = 
{ 
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // T&H service UUID
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,
  LO_UINT16( TEMPHUMID_SERV_UUID ),
  HI_UINT16( TEMPHUMID_SERV_UUID ),
};

// scan response data
static uint8 scanResponseData[] =
{
  0x07,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_SHORT,   
  'C',
  'M',
  '_',
  'T',
  'H',
  'S'
};

// GGS device name
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "CM Temp Humid";

// current measurement status
static uint8 status = STATUS_MEAS_STOP;

// measurement interval
static uint16 interval = DEFAULT_MEAS_PERIOD;


static void gapRoleStateCB( gaprole_States_t newState ); // GAP Role callback function
static void thServiceCB( uint8 event ); // T&H callback function

// GAP Role callback struct
static gapRolesCBs_t gapRoleStateCBs =
{
  gapRoleStateCB,  // Profile State Change Callbacks
  NULL                   // When a valid RSSI is read from controller (not used by application)
};

// bond callback struct
static gapBondCBs_t thBondCBs =
{
  NULL,                   // Passcode callback
  NULL                    // Pairing state callback
};

// T&H callback struct
static thServiceCBs_t thServCBs =
{
  thServiceCB    
};


static void processOSALMsg( osal_event_hdr_t *pMsg ); // 
static void startTHMeas( void ); // start T&H measurement
static void stopTHMeas( void ); // stop T&H measurement
static void initIOPin(); // initialize I/O pins

/*********************************************************************
 * 公共函数
 */
extern void TempHumid_Init( uint8 task_id )
{
  taskID = task_id;
  
  // Setup the GAP Peripheral Role Profile
  {
    // 
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanResponseData ), scanResponseData );
    
    // 
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0 ); // 不停地广播
    
    // enable advertising
    uint8 advertising = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &advertising );
    
    // set the pause time from the connection and the update of the connection parameters
    // during the time, client can finish the tasks e.g. service discovery 
    // the unit of time is second
    GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, 2 ); 
    
    // set the connection parameter
    uint16 desired_min_interval = 200;  // units of 1.25ms 
    uint16 desired_max_interval = 1600; // units of 1.25ms
    uint16 desired_slave_latency = 1;
    uint16 desired_conn_timeout = 1000; // units of 10ms
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
    uint8 enable_update_request = TRUE;
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
  }
  
  // set GGS device name
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  {
    uint32 passkey = 0; // passkey "000000"
    uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8 mitm = TRUE;
    uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8 bonding = TRUE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
  }  

  // set T&H Characteristic Values
  {
    TempHumid_SetParameter( TEMPHUMID_INTERVAL, sizeof(uint16), &interval );
  }
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  TempHumid_AddService( GATT_ALL_SERVICES ); // Temperature&Humid service
  DevInfo_AddService( ); // device information service
  
  TempHumid_RegisterAppCBs( &thServCBs );
  
  //在这里初始化GPIO
  //第一：所有管脚，reset后的状态都是输入加上拉
  //第二：对于不用的IO，建议不连接到外部电路，且设为输入上拉
  //第三：对于会用到的IO，就要根据具体外部电路连接情况进行有效设置，防止耗电
  {
    initIOPin();
  }
  
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // 启动设备
  osal_set_event( taskID, TEMPHUMID_START_DEVICE_EVT );
}


// 初始化IO管脚
static void initIOPin()
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

    if ( (pMsg = osal_msg_receive( taskID )) != NULL )
    {
      processOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & TEMPHUMID_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &gapRoleStateCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &thBondCBs );

    return ( events ^ TEMPHUMID_START_DEVICE_EVT );
  }
  
  if ( events & TEMPHUMID_MEAS_PERIODIC_EVT )
  {
    if(gapProfileState == GAPROLE_CONNECTED && status == STATUS_MEAS_START) {
      osal_start_timerEx( taskID, TEMPHUMID_MEAS_PERIODIC_EVT, interval*1000 );
      TempHumid_TempHumidIndicate( gapConnHandle, taskID); 
    }

    return (events ^ TEMPHUMID_MEAS_PERIODIC_EVT);
  }
  
  // Discard unknown events
  return 0;
}

static void processOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    default:
      // do nothing
      break;
  }
}

static void gapRoleStateCB( gaprole_States_t newState )
{
  // 已连接
  if( newState == GAPROLE_CONNECTED)
  {
    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &gapConnHandle );
  }
  // 断开连接
  else if(gapProfileState == GAPROLE_CONNECTED && 
            newState != GAPROLE_CONNECTED)
  {
    stopTHMeas();
    //initIOPin();
  }
  // if started
  else if (newState == GAPROLE_STARTED)
  {
    // Set the system ID from the bd addr
    uint8 systemId[DEVINFO_SYSTEM_ID_LEN];
    GAPRole_GetParameter(GAPROLE_BD_ADDR, systemId);
    
    // shift three bytes up
    systemId[7] = systemId[5];
    systemId[6] = systemId[4];
    systemId[5] = systemId[3];
    
    // set middle bytes to zero
    systemId[4] = 0;
    systemId[3] = 0;
    
    DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
  }
  
  gapProfileState = newState;

}

static void thServiceCB( uint8 event )
{
  switch (event)
  {
    case TEMPHUMID_DATA_IND_ENABLED:
      startTHMeas();
      break;
        
    case TEMPHUMID_DATA_IND_DISABLED:
      stopTHMeas();
      break;

    case TEMPHUMID_INTERVAL_SET:
      TempHumid_GetParameter( TEMPHUMID_INTERVAL, &interval );
      break;
      
    default:
      // Should not get here
      break;
  }
}

// 
static void startTHMeas( void )
{  
  if(status == STATUS_MEAS_STOP) {
    status = STATUS_MEAS_START;
    osal_start_timerEx( taskID, TEMPHUMID_MEAS_PERIODIC_EVT, interval*1000);
  }
}

// 
static void stopTHMeas( void )
{  
  status = STATUS_MEAS_STOP;
  osal_stop_timerEx( taskID, TEMPHUMID_MEAS_PERIODIC_EVT ); 
}
