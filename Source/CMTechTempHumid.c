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

#include "hal_i2c.h"

#include "Dev_Si7021.h"

#include "CMTechTempHumid.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif


/*********************************************************************
 * ����
*/
#define INVALID_CONNHANDLE                    0xFFFF

// ֹͣ״̬
#define STATUS_STOP           0     

// ��ʼ״̬
#define STATUS_START          1            

// ȱʡ�������ڣ�1��
#define DEFAULT_PERIOD         1000

/* ��ʱn���� */
#define ST_HAL_DELAY(n) st( { volatile uint32 i; for (i=0; i<(n); i++) { }; } )


/*********************************************************************
 * �ֲ�����
*/
// ����ID
static uint8 tempHumid_TaskID;   

// GAP״̬
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// ��ǰ״̬
static uint8 status = STATUS_STOP;

// ���ݲ������ڣ���λ��ms
static uint16 period = DEFAULT_PERIOD;


/*********************************************************************
 * �ֲ�����
*/
// OSAL��Ϣ������
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg );

// ״̬�仯֪ͨ�ص�����
static void peripheralStateNotificationCB( gaprole_States_t newState );

// ��ʪ�ȷ���ص�����
static void tempHumidServiceCB( uint8 paramID );

// ��ʼ��
static void tempHumidInit();

// ��������
static void tempHumidStart( void );

// ֹͣ����
static void tempHumidStop( void );

// ��ȡ����������
static void tempHumidReadAndProcessData();

// ��ʼ��IO�ܽ�
static void tempHumidInitIOPin();


/*********************************************************************
 * PROFILE and SERVICE �ص��ṹ��ʵ��
*/

// GAP Role �ص��ṹ��ʵ�����ṹ����Э��ջ������
static gapRolesCBs_t tempHumid_PeripheralCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                            // When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager �ص��ṹ��ʵ�����ṹ����Э��ջ������
static gapBondCBs_t tempHumid_BondMgrCBs =
{
  NULL,                     // Passcode callback (not used by application)
  NULL                      // Pairing / Bonding state Callback (not used by application)
};

// ��ʪ�Ȼص��ṹ��ʵ�����ṹ����Serice_TempHumid��������
static tempHumidServiceCBs_t tempHumid_ServCBs =
{
  tempHumidServiceCB    // ��ʪ�ȷ���ص�����ʵ����������Serice_TempHumid��������
};


/*********************************************************************
 * ��������
 */

extern void TempHumid_Init( uint8 task_id )
{
  tempHumid_TaskID = task_id;

  // GAP ����
  //���ù㲥����
  GAPConfig_SetAdvParam(800, TEMPHUMID_SERV_UUID);
  // ��ʼ�����̹㲥
  GAPConfig_EnableAdv(TRUE);

  //�������Ӳ���
  GAPConfig_SetConnParam(200, 200, 5, 10000, 1);

  //����GGS�������豸��
  GAPConfig_SetGGSParam("Temp&Humid");

  //���ð󶨲���
  GAPConfig_SetBondingParam(0, GAPBOND_PAIRING_MODE_WAIT_FOR_REQ);

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif
  
  GATTConfig_SetTempHumidService(&tempHumid_ServCBs);

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
  osal_set_event( tempHumid_TaskID, TH_START_DEVICE_EVT );
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

  if ( events & TH_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &tempHumid_PeripheralCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &tempHumid_BondMgrCBs );

    return ( events ^ TH_START_DEVICE_EVT );
  }
  
  if ( events & TH_PERIODIC_EVT )
  {
    tempHumidReadAndProcessData();

    if(status == STATUS_START)
      osal_start_timerEx( tempHumid_TaskID, TH_PERIODIC_EVT, period );

    return (events ^ TH_PERIODIC_EVT);
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

      break;

    case GAPROLE_WAITING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Disconnected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
          
        // �Ͽ�����ʱ��ֹͣAD�ɼ�
        tempHumidStop();
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

    default:
      // Should not get here
      break;
  }
}


// �������ģʽ
static void tempHumidInit()
{
  status = STATUS_STOP;

  uint8 tempHumidData[TEMPHUMID_DATA_LEN] = { 0 };
  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, tempHumidData );
  
  // ֹͣ�ɼ�
  uint8 tempHumidCtrl = TEMPHUMID_CTRL_STOP;
  TempHumid_SetParameter( TEMPHUMID_CTRL, sizeof(uint8), &tempHumidCtrl );  
  
  // ���ô�������
  uint8 tmp = period/TEMPHUMID_PERIOD_UNIT;
  TempHumid_SetParameter( TEMPHUMID_PERI, sizeof(uint8), (uint8*)&(tmp) ); 
}

// ��������
static void tempHumidStart( void )
{  
  if(status == STATUS_STOP) {
    status = STATUS_START;

    osal_start_timerEx( tempHumid_TaskID, TH_PERIODIC_EVT, period);
  }
}

// ֹͣ����
static void tempHumidStop( void )
{  
  status = STATUS_STOP;
}

// ������
static void tempHumidReadAndProcessData()
{
  SI7021_HumiAndTemp humidTemp = SI7021_Measure();
  uint8 data[TEMPHUMID_DATA_LEN] = {0};
  uint8* pt = (uint8*)(&(humidTemp.Humidity));
  for(int i = 0; i < sizeof(long); i++) 
    data[i] = *pt++;
  pt = (uint8*)(&(humidTemp.Temperature));
  for(int i = 0; i < sizeof(long); i++) 
    data[i+4] = *pt++;
  TempHumid_SetParameter( TEMPHUMID_DATA, TEMPHUMID_DATA_LEN, (uint8*)data); 
}



/*********************************************************************
*********************************************************************/
