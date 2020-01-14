/**************************************************************************************************
* CMTechTempHumid.c: Ӧ����Դ�ļ�
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
//#include "devinfoservice.h"
#include "Service_DevInfo.h"
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

#include "Dev_Battery.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif


/*********************************************************************
 * ����
*/
#define INVALID_CONNHANDLE                    0xFFFF

// ֹͣ�ɼ�״̬
#define STATUS_STOP           0     

// ��ʼ�ɼ�״̬
#define STATUS_START          1            

// ȱʡ���������5��
#define DEFAULT_PERIOD         5


/*********************************************************************
 * �ֲ�����
*/
// ����ID
static uint8 tempHumid_TaskID;   

// ���Ӿ��
uint16 gapConnHandle;

// GAP״̬
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// �������
static uint8 advertData[] = 
{ 
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // service UUID
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,
  LO_UINT16( TEMPHUMID_SERV_UUID ),
  HI_UINT16( TEMPHUMID_SERV_UUID ),

};

// GAP Profile - Name attribute for SCAN RSP data
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

// GAP GATT �豸��
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "CM Temp Humid";

// ��ǰ״̬
static uint8 status = STATUS_STOP;

// ���ݲ������ڣ���λ��ms
static uint16 period = DEFAULT_PERIOD;

// ��ص����ɼ�״̬
static uint8 batteryStatus = STATUS_STOP;

// ��ص����������ڣ���λ������
static uint8 batteryPeriod = 1;


/*********************************************************************
 * �ֲ�����
*/
// OSAL��Ϣ������
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg );

// GAP״̬�ı�ص�����
static void peripheralGapStateCB( gaprole_States_t newState );

// ��ʪ�ȷ���ص�����
static void tempHumidServiceCB( uint8 event );

// ��ʱ����ص�����
static void timerServiceCB( uint8 paramID );

// ��ص�������ص�����
static void batteryServiceCB( uint8 paramID );

// ��ʼ��
static void tempHumidInit();

// ��������
static void tempHumidStart( void );

// ֹͣ����
static void tempHumidStop( void );

// ��ȡ��������
static void tempHumidReadAndTransferData();

// ��ȡ������ÿ��Сʱ������
static void tempHumidReadAndStoreData();

// ��ʼ��IO�ܽ�
static void tempHumidInitIOPin();

// ������ص�������
static void batteryStart( void );

// ֹͣ��ص�������
static void batteryStop( void );

// ��ȡ�����ص�������
static void batteryReadAndTransferData();

/*********************************************************************
 * PROFILE and SERVICE �ص��ṹ��ʵ��
*/

// GAP Role �ص��ṹ��ʵ�����ṹ����Э��ջ������
static gapRolesCBs_t tempHumid_PeripheralCBs =
{
  peripheralGapStateCB,  // Profile State Change Callbacks
  NULL                   // When a valid RSSI is read from controller (not used by application)
};

// ��ʪ�Ȼص��ṹ��ʵ�����ṹ����Serice_TempHumid��������
static tempHumidServiceCBs_t tempHumid_ServCBs =
{
  tempHumidServiceCB    // ��ʪ�ȷ���ص�����ʵ����������Serice_TempHumid��������
};

// ��ʱ����ص��ṹ��ʵ�����ṹ����Serice_Timer��������
static timerServiceCBs_t timer_ServCBs =
{
  timerServiceCB    // ��ʱ����ص�����ʵ����������Serice_Timer��������
};

// ��ص�������ص��ṹ��ʵ�����ṹ����Serice_Battery��������
static batteryServiceCBs_t battery_ServCBs =
{
  batteryServiceCB    // ��ص�������ص�����ʵ����������Serice_Battery��������
};




/*********************************************************************
 * ��������
 */
extern void TempHumid_Init( uint8 task_id )
{
  tempHumid_TaskID = task_id;
  
  // Setup the GAP Peripheral Role Profile
  {
    // ���ù�����ݺ�ɨ����Ӧ����
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanResponseData ), scanResponseData );
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    
    // ���ù��ʱ����
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0 ); // ��ͣ�ع㲥
    
    // �����Ƿ�ʹ�ܹ��
    uint8 initial_advertising_enable = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    
    // �����Ƿ�����������Ӳ����Լ����������Ӳ���
    uint8 enable_update_request = TRUE;
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
    uint16 desired_min_interval = 200;  // units of 1.25ms 
    uint16 desired_max_interval = 1600; // units of 1.25ms
    uint16 desired_slave_latency = 1;
    uint16 desired_conn_timeout = 1000; // units of 10ms
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
    
    //���������ָ�����ӽ��������迪ʼconnection update procedure֮����Ҫ��ʱ��ʱ��(units of second)
    //���������ͬ����²������ӻ�����ѡ��Ͽ����ӻ�����������в���
    GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, 1 ); 
  }
  
  // ����GGS�豸������ֵ
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  // ����Լ��ܰ��й�������
  {
    uint32 passkey = 0; // passkey "000000"
    uint8 pairMode = GAPBOND_PAIRING_MODE_INITIATE;
    uint8 mitm = TRUE;
    uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8 bonding = TRUE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
  }  

  // Setup the Heart Rate Characteristic Values
  {
    uint8 sensLoc = HEARTRATE_SENS_LOC_WRIST;
    HeartRate_SetParameter( HEARTRATE_SENS_LOC, sizeof ( uint8 ), &sensLoc );
  }
  
  // Setup Battery Characteristic Values
  {
    uint8 critical = DEFAULT_BATT_CRITICAL_LEVEL;
    Batt_SetParameter( BATT_PARAM_CRITICAL_LEVEL, sizeof (uint8 ), &critical );
  }
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  HeartRate_AddService( GATT_ALL_SERVICES );
  DevInfo_AddService( );
  Batt_AddService( );
  
  // Register for Heart Rate service callback
  HeartRate_Register( heartRateCB );
  
  // Register for Battery service callback;
  Batt_Register ( heartRateBattCB );
    
  // For keyfob board set GPIO pins into a power-optimized state
  // Note that there is still some leakage current from the buzzer,
  // accelerometer, LEDs, and buttons on the PCB.
  
  P0SEL = 0; // Configure Port 0 as GPIO
  P1SEL = 0; // Configure Port 1 as GPIO
  P2SEL = 0; // Configure Port 2 as GPIO

  P0DIR = 0xFC; // Port 0 pins P0.0 and P0.1 as input (buttons),
                // all others (P0.2-P0.7) as output
  P1DIR = 0xFF; // All port 1 pins (P1.0-P1.7) as output
  P2DIR = 0x1F; // All port 1 pins (P2.0-P2.4) as output
  
  P0 = 0x03; // All pins on port 0 to low except for P0.0 and P0.1 (buttons)
  P1 = 0;   // All pins on port 1 to low
  P2 = 0;   // All pins on port 2 to low  
  
  // Setup a delayed profile startup
  osal_set_event( tempHumid_TaskID, START_DEVICE_EVT );

  // GAP ����
  //���ù㲥����
  GAPConfig_SetAdvParam(2000, TEMPHUMID_SERV_UUID);
  
  // ��ʼ�����̹㲥
  GAPConfig_EnableAdv(TRUE);

  //�������Ӳ���
  GAPConfig_SetConnParam(20, 40, 0, 1500, 1);
  //GAPConfig_SetConnParam(100, 100, 1, 2000, 1);

  //����GGS�������豸��
  GAPConfig_SetGGSParam(attDeviceName);


  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif
  
  GATTConfig_SetTempHumidService(&tempHumid_ServCBs);  
  
  GATTConfig_SetTimerService(&timer_ServCBs); 
  
  GATTConfig_SetBatteryService(&battery_ServCBs);


  //�������ʼ��GPIO
  //��һ�����йܽţ�reset���״̬�������������
  //�ڶ������ڲ��õ�IO�����鲻���ӵ��ⲿ��·������Ϊ��������
  //���������ڻ��õ���IO����Ҫ���ݾ����ⲿ��·�������������Ч���ã���ֹ�ĵ�
  {
    tempHumidInitIOPin();
  }
  
  // ��ʼ��
  tempHumidInit();  
 
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // �����豸
  osal_set_event( tempHumid_TaskID, TEMPHUMID_START_DEVICE_EVT );
}


// ��ʼ��IO�ܽ�
static void tempHumidInitIOPin()
{
  // ȫ����ΪGPIO
  P0SEL = 0; 
  P1SEL = 0; 
  P2SEL = 0; 

  // ȫ����Ϊ����͵�ƽ
  P0DIR = 0xFF; 
  P1DIR = 0xFF; 
  P2DIR = 0x1F; 

  P0 = 0; 
  P1 = 0;   
  P2 = 0;  
  
  // I2C��SDA, SCL����ΪGPIO, ����͵�ƽ�����򹦺ĺܴ�
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
  
  // ��ʱ�����¼�
  if ( events & TEMPHUMID_START_TIMER_EVT )
  {
    uint8 value[4] = {0};
    // ���µ�ǰʱ��
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
  
  // �޸���������¼�
  if ( events & TEMPHUMID_CHANGE_PAIRPWD_EVT )
  {
    GAPConfig_TerminateConn();

    return (events ^ TEMPHUMID_CHANGE_PAIRPWD_EVT);
  }    
  
  if ( events & TEMPHUMID_START_BATTERY_EVT )
  {
    batteryReadAndTransferData();

    if(batteryStatus == STATUS_START)
      osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_BATTERY_EVT, batteryPeriod*10000L );

    return (events ^ TEMPHUMID_START_BATTERY_EVT);
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


static void peripheralGapStateCB( gaprole_States_t newState )
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
      
      batteryReadAndTransferData();
      
      break;

    case GAPROLE_WAITING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Disconnected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
          
        // �Ͽ�����ʱ��ֹͣAD�ɼ�
        tempHumidStop();
        
        // �Ͽ�����ʱ��ֹͣ��ص����ɼ�
        batteryStop();
        
        // I2C��SDA, SCL����ΪGPIO, ����͵�ƽ�����򹦺ĺܴ�
        HalI2CSetAsGPIO();

      }
      break;

    case GAPROLE_WAITING_AFTER_TIMEOUT:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Timed Out",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
        //GAPConfig_EnableAdv(TRUE);
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

static void tempHumidServiceCB( uint8 event )
{
  uint8 newValue;
  uint8 time[2] = {0};
  uint8 data[8] = {-1};
  float invalidValue = -1.0;

  switch (event)
  {
    case TEMPHUMID_CTRL:
      TempHumid_GetParameter( TEMPHUMID_CTRL, &newValue );
      
      // ֹͣ�ɼ�
      if ( newValue == TEMPHUMID_CTRL_STOP)  
      {
        tempHumidStop();
      }
      // ��ʼ�ɼ�
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

static void batteryServiceCB( uint8 paramID )
{
  uint8 newValue;

  switch (paramID)
  {
    case BATTERY_CTRL:
      Battery_GetParameter( BATTERY_CTRL, &newValue );
      
      // ֹͣ�ɼ�
      if ( newValue == BATTERY_CTRL_STOP)  
      {
        batteryStop();
      }
      // ��ʼ�ɼ�
      else if ( newValue == BATTERY_CTRL_START) 
      {
        batteryStart();
      }
      
      break;

    case BATTERY_PERI:
      Battery_GetParameter( BATTERY_PERI, &newValue );
      batteryPeriod = newValue;

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
        osal_stop_timerEx( tempHumid_TaskID, TEMPHUMID_START_TIMER_EVT);  // ֹͣ��ʱ����
      } else {
        if(value[2] >= 1 && value[2] <= 30) // ��ʱ���ڱ�����1-30����
        {
          timerPeriod = value[2];   // ���¼�ʱ����
          uint8 tmp = value[1]%timerPeriod;
          value[1] -= tmp;          // ����minute
          Timer_SetParameter(TIMER_VALUE, 4, value);    // ���¼�ʱ����ֵ
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

// ��ʼ����ʪ�ȷ������
static void tempHumidInit()
{
  status = STATUS_STOP;

  uint8 tempHumidData[TEMPHUMID_DATA_LEN] = { 0 };
  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, tempHumidData );
  
  // ֹͣ�ɼ�
  uint8 tempHumidCtrl = TEMPHUMID_CTRL_STOP;
  TempHumid_SetParameter( TEMPHUMID_CTRL, sizeof(uint8), &tempHumidCtrl );  
  
  // ���ô�������
  uint8 tmp = DEFAULT_PERIOD/TEMPHUMID_PERIOD_UNIT;
  TempHumid_SetParameter( TEMPHUMID_PERI, sizeof(uint8), (uint8*)&tmp ); 
}

// ��������
static void tempHumidStart( void )
{  
  if(status == STATUS_STOP) {
    status = STATUS_START;
    osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_PERIODIC_EVT, period);
  }
}

// ֹͣ����
static void tempHumidStop( void )
{  
  status = STATUS_STOP;
}

// ��ȡ��������
static void tempHumidReadAndTransferData()
{
  uint8 data[TEMPHUMID_DATA_LEN] = {0};
  SI7021_MeasureData(data);

  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, data); 
}

// ��ȡ�����浱ǰ��ʪ������
static void tempHumidReadAndStoreData()
{
  uint8 value[4] = {0};
  Timer_GetParameter(TIMER_VALUE, value);
  uint8 data[10] = {0};
  data[0] = value[0];
  data[1] = value[1];
  
  // ǰ���β��ȶ�����ȡ����������
  SI7021_Measure();
  SI7021_Measure();
  SI7021_MeasureData(data+2);
  
  //���뱣�����ݵĴ���
  Queue_Push(data);
}

// ������ص�������
static void batteryStart( void )
{  
  if(batteryStatus == STATUS_STOP) {
    batteryStatus = STATUS_START;
    osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_START_BATTERY_EVT, batteryPeriod*10000L);
  }
}

// ֹͣ��ص�������
static void batteryStop( void )
{  
  osal_stop_timerEx( tempHumid_TaskID, TEMPHUMID_START_BATTERY_EVT);
  batteryStatus = STATUS_STOP;
}

// ��ȡ�����ص�������
static void batteryReadAndTransferData()
{
  uint8 batteryData = Battery_Measure();

  Battery_SetParameter( BATTERY_DATA, 1, &batteryData);   
}


/*********************************************************************
*********************************************************************/
