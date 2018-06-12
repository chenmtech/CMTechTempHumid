
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "cmutil.h"

#include "Service_Timer.h"

/*
* ����
*/
// ���������������128λUUID
// ��ʱ����UUID
CONST uint8 timerServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_SERV_UUID)
};

// ��ǰʱ��UUID
CONST uint8 timerCurTimeUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_CURTIME_UUID)
};

// ��ʱ���Ƶ�UUID
CONST uint8 timerCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_CTRL_UUID)
};

// ��ʱ����UUID
CONST uint8 timerPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_PERIOD_UUID)
};








/*
* �ֲ�����
*/
// ��������ֵ
static CONST gattAttrType_t timerService = { ATT_UUID_SIZE, timerServUUID };

// ��ǰʱ���������ԣ��ɶ���д
static uint8 timerCurTimeProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerCurTime[2] = {0};

// ��ʱ���Ƶ��������ԣ��ɶ���д
static uint8 timerCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerCtrl = 0;

// ��ʱ���ڵ�������ԣ��ɶ���д����λ������
static uint8 timerPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerPeriod = 10;    


// ��������Ա�
static gattAttribute_t timerServAttrTbl[] = 
{
  // ��ʱ����
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&timerService                /* pValue */
  },

    // ��ǰʱ����������
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &timerCurTimeProps 
    },

      // ��ǰʱ������ֵ
      { 
        { ATT_UUID_SIZE, timerCurTimeUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        timerCurTime 
      }, 

      
    // ��ʱ���Ƶ���������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &timerCtrlProps
    },

      // ��ʱ���Ƶ�����ֵ
      {
        { ATT_UUID_SIZE, timerCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &timerCtrl
      },

     // ��ʱ������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &timerPeriodProps
    },

      // ��ʱ��������ֵ
      {
        { ATT_UUID_SIZE, timerPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &timerPeriod
      },
};







/*
* �ֲ�����
*/
// ����Ӧ�ò���Ļص�����ʵ��
static timerServiceCBs_t * timerService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 timer_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// д���Իص�
static bStatus_t timer_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// ����״̬�ı�ص�
static void timer_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// �����Э��ջ�Ļص��ṹ��ʵ��
CONST gattServiceCBs_t timerServCBs =
{
  timer_ReadAttrCB,           // Read callback function pointer
  timer_WriteAttrCB,          // Write callback function pointer
  NULL                        // Authorization callback function pointer
};


static uint8 timer_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
  
  if (utilExtractUuid16(pAttr, &uuid) == FAILURE) {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }
  
  switch ( uuid )
  {
    // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
    // gattserverapp handles those reads
    case TIMER_CURTIME_UUID:
      // ����ǰʱ��ֵ
      *pLen = 2;
      VOID osal_memcpy( pValue, pAttr->pValue, 2 );
      break;
      
    // ����ʱ���Ƶ�Ͷ�ʱ����ֵ�����ǵ��ֽ�  
    case TIMER_CTRL_UUID:
    case TIMER_PERIOD_UUID:
      *pLen = 1;
      pValue[0] = *pAttr->pValue;
      break;
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }  
  
  return status;
}


static bStatus_t timer_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint16 uuid;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if (utilExtractUuid16(pAttr,&uuid) == FAILURE) {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }
  
  switch ( uuid )
  {
    case TIMER_CURTIME_UUID:
      // д��ǰʱ��ֵ
      if ( offset == 0 )
      {
        if ( len != 2 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }      
      
      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;

        *pCurValue = pValue[0];
        *(pCurValue+1) = pValue[1];
      }
      
      break;

    // д��ʱ���Ƶ�
    case TIMER_CTRL_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }

      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;

        *pCurValue = pValue[0];

        if( pAttr->pValue == &timerCtrl )
        {
          notifyApp = TIMER_CTRL;
        }
      }
      break;

    // д��ʱ����
    case TIMER_PERIOD_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }
      // Write the value
      if ( status == SUCCESS )
      {
        // ��ʱ���ڱ�����1-60����֮��
        if ( pValue[0] >= 1 && pValue[0] <= 60 )
        {
          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          *pCurValue = pValue[0];

          if( pAttr->pValue == &timerPeriod )
          {
            notifyApp = TIMER_PERIOD;
          }
        }
        else
        {
           status = ATT_ERR_INVALID_VALUE;
        }
      }
      break;

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && timerService_AppCBs && timerService_AppCBs->pfnTimerServiceCB )
  {
    timerService_AppCBs->pfnTimerServiceCB( notifyApp );
  }  
  
  return status;
}

static void timer_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      //GATTServApp_InitCharCfg( connHandle, tempHumidDataConfig );
    }
  }
}










/*
 * ��������
*/

// ���ط���
extern bStatus_t Timer_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  //GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tempHumidDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( timer_HandleConnStatusCB );  
  
  if ( services & TIMER_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( timerServAttrTbl, 
                                          GATT_NUM_ATTRS( timerServAttrTbl ),
                                          &timerServCBs );
  }

  return ( status );
}

// �Ǽ�Ӧ�ò���Ļص�
extern bStatus_t Timer_RegisterAppCBs( timerServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    timerService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

// ���ñ��������������
extern bStatus_t Timer_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // ���õ�ǰʱ��
    case TIMER_CURTIME:
      if ( len == 2 )
      {
        VOID osal_memcpy( timerCurTime, value, 2 );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // ���ö�ʱ���Ƶ�
    case TIMER_CTRL:
      if ( len == sizeof ( uint8 ) )
      {
        timerCtrl = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // ���ö�ʱ����  
    case TIMER_PERIOD:
      if ( len == sizeof ( uint8 ) )
      {
        timerPeriod = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }  
  
  return ( ret );
}

// ��ȡ��������������
extern bStatus_t Timer_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // ��ȡ��ǰʱ��
    case TIMER_CURTIME:
      VOID osal_memcpy( value, timerCurTime, 2 );
      break;

    // ��ȡ��ʱ���Ƶ�  
    case TIMER_CTRL:
      *((uint8*)value) = timerCtrl;
      break;

    // ��ȡ��ʱ����  
    case TIMER_PERIOD:
      *((uint8*)value) = timerPeriod;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

