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
 * ����
*/
#define INVALID_CONNHANDLE                    0xFFFF

// ֹͣ�ɼ�״̬
#define STATUS_STOP           0     

// ��ʼ�ɼ�״̬
#define STATUS_START          1            

// ȱʡ�������ڣ�5��
#define DEFAULT_PERIOD         5000


/*********************************************************************
 * �ֲ�����
*/
// ����ID
static uint8 tempHumid_TaskID;   

// GAP״̬
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// GAP GATT �豸��
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "TempHumid";

// ��ǰ״̬
static uint8 status = STATUS_STOP;

// ���ݲ������ڣ���λ��ms
static uint16 period = DEFAULT_PERIOD;

// ��ʱ���ڣ���λ������
static uint8 timerPeriod = 30;

typedef enum  
{  
  PAIRSTATUS_PAIRED = 0,  
  PAIRSTATUS_NO_PAIRED,  
}PAIRSTATUS;  

static PAIRSTATUS pairStatus = PAIRSTATUS_NO_PAIRED;       //���״̬��Ĭ����û���

/*********************************************************************
 * �ֲ�����
*/
// OSAL��Ϣ������
static void tempHumid_ProcessOSALMsg( osal_event_hdr_t *pMsg );

// ״̬�仯֪ͨ�ص�����
static void peripheralStateNotificationCB( gaprole_States_t newState );

static void processPasscodeCB(uint8 *deviceAddr, uint16 connectionHandle, uint8 uiInputs, uint8 uiOutputs);

static void processPairStateCB(uint16 connHandle, uint8 state, uint8 status);

// ��ʪ�ȷ���ص�����
static void tempHumidServiceCB( uint8 paramID );

// ��ʱ����ص�����
static void timerServiceCB( uint8 paramID );

// ����������ص�����
static void pairPwdServiceCB( uint8 paramID );



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
  processPasscodeCB,                     // Passcode callback
  processPairStateCB                     // Pairing / Bonding state Callback
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

// ����������ص��ṹ��ʵ�����ṹ����Serice_PairPwd��������
static pairPwdServiceCBs_t pairPwd_ServCBs =
{
  pairPwdServiceCB    // ����������ص�����ʵ����������Serice_Timer��������
};


/*********************************************************************
 * ��������
 */
extern void TempHumid_Init( uint8 task_id )
{
  tempHumid_TaskID = task_id;
  
  //���÷��书��Ϊ4dB(��ϧCC2541��֧��4dB)
  //HCI_EXT_SetTxPowerCmd (HCI_EXT_TX_POWER_4_DBM);  

  // GAP ����
  //���ù㲥����
  GAPConfig_SetAdvParam(2000, TEMPHUMID_SERV_UUID);
  
  // ��ʼ�����̹㲥
  GAPConfig_EnableAdv(TRUE);

  //�������Ӳ���
  GAPConfig_SetConnParam(80, 120, 0, 1000, 1);
  //GAPConfig_SetConnParam(100, 100, 1, 2000, 1);

  //����GGS�������豸��
  GAPConfig_SetGGSParam(attDeviceName);

  //������ԺͰ󶨲���
  //GAPConfig_SetPairBondingParam(GAPBOND_PAIRING_MODE_INITIATE, TRUE);
  GAPConfig_SetPairBondingParam(GAPBOND_PAIRING_MODE_WAIT_FOR_REQ, TRUE);
  
  // ÿ���������������޸�Ϊ"000000"
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
          
        // �Ͽ�����ʱ��ֹͣAD�ɼ�
        tempHumidStop();
        
        // I2C��SDA, SCL����ΪGPIO, ����͵�ƽ�����򹦺ĺܴ�
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

// ������������ʱ�Ļص�
static void processPasscodeCB(uint8 *deviceAddr, uint16 connectionHandle, uint8 uiInputs, uint8 uiOutputs)    
{    
  //������  
  uint32 password = 0L;
  GapConfig_SNV_Password(GAP_PARI_PASSWORD_READ, (uint8 *)(&password), sizeof(uint32));  
    
  //����������Ӧ������  
  GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, password);  
} 

static void processPairStateCB(uint16 connHandle, uint8 state, uint8 status)  
{  
  //�����������ӣ�����뿪ʼ���״̬  
  if(state == GAPBOND_PAIRING_STATE_STARTED)  
  {  
    pairStatus = PAIRSTATUS_NO_PAIRED;  
  }  
    
  //�������ύ����󣬻����������״̬    
  else if(state == GAPBOND_PAIRING_STATE_COMPLETE)  
  {  
    //��Գɹ�  
    if(status == SUCCESS)      
    { 
      pairStatus = PAIRSTATUS_PAIRED;  
    }  
      
    //����Թ�  
    else if(status == SMP_PAIRING_FAILED_UNSPECIFIED)  
    {       
      pairStatus = PAIRSTATUS_PAIRED;  
    }  
      
    //���ʧ��  
    else  
    {  
      pairStatus = PAIRSTATUS_NO_PAIRED;  
    }  
      
    //���ʧ����Ͽ�����  
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

// ����������ص�����
static void pairPwdServiceCB( uint8 paramID )
{
  switch (paramID)
  {
    case PAIRPWD_VALUE:
      // �޸�������룬1���Ͽ�����
      osal_start_timerEx( tempHumid_TaskID, TEMPHUMID_CHANGE_PAIRPWD_EVT, 1000 ); 
      
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



/*********************************************************************
*********************************************************************/
