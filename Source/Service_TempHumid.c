
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "cmutil.h"

#include "Service_TempHumid.h"

/*
* ����
*/
// ���������������128λUUID
// ��ʪ�ȷ���UUID
CONST uint8 tempHumidServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_SERV_UUID)
};

// ʵʱ��ʪ������UUID
CONST uint8 tempHumidDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_DATA_UUID)
};

// ʵʱ�������Ƶ�UUID
CONST uint8 tempHumidCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_CTRL_UUID)
};

// ʵʱ��������UUID
CONST uint8 tempHumidPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_PERI_UUID)
};

// ��ʷ���ݵ�ʱ��UUID
CONST uint8 tempHumidHistoryTimeUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_HISTORY_TIME_UUID)
};

// ��ʷ����UUID
CONST uint8 tempHumidHistoryDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_HISTORY_DATA_UUID)
};






/*
* �ֲ�����
*/
// ��ʪ�ȷ�������ֵ
static CONST gattAttrType_t tempHumidService = { ATT_UUID_SIZE, tempHumidServUUID };

// ʵʱ��ʪ�����ݵ�������ԣ��ɶ���֪ͨ
static uint8 tempHumidDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 tempHumidData[TEMPHUMID_DATA_LEN] = {0};
static gattCharCfg_t tempHumidDataConfig[GATT_MAX_NUM_CONN];

// ʵʱ�������Ƶ��������ԣ��ɶ���д
static uint8 tempHumidCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 tempHumidCtrl = 0;

// ʵʱ�������ڵ�������ԣ��ɶ���д
static uint8 tempHumidPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 tempHumidPeriod = 0;  

// ��ʷ�������ݵ�ʱ��������ԣ���д
static uint8 tempHumidHistoryTimeProps = GATT_PROP_WRITE;
static uint8 tempHumidHistoryTime[2] = {0}; 

// ��ʷ���ݵ�������ԣ��ɶ�
static uint8 tempHumidHistoryDataProps = GATT_PROP_READ;
static uint8 tempHumidHistoryData[TEMPHUMID_DATA_LEN] = {0}; 



// ��������Ա�
static gattAttribute_t tempHumidServAttrTbl[] = 
{
  // tempHumid ����
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&tempHumidService                /* pValue */
  },

    // ʵʱ��ʪ��������������
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &tempHumidDataProps 
    },

      // ʵʱ��ʪ����������ֵ
      { 
        { ATT_UUID_SIZE, tempHumidDataUUID },
        GATT_PERMIT_READ, 
        0, 
        tempHumidData 
      }, 
      
      // ʵʱ��ʪ����������������
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)tempHumidDataConfig
      },
      
    // ʵʱ�������Ƶ���������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidCtrlProps
    },

      // ʵʱ�������Ƶ�����ֵ
      {
        { ATT_UUID_SIZE, tempHumidCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &tempHumidCtrl
      },

    // ʵʱ����������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidPeriodProps
    },

      // ʵʱ������������ֵ
      {
        { ATT_UUID_SIZE, tempHumidPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &tempHumidPeriod
      },
      
    // ��ʷ���ݵ�ʱ����������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidHistoryTimeProps
    },

      // ��ʷ����ʱ������ֵ
      {
        { ATT_UUID_SIZE, tempHumidHistoryTimeUUID },
        GATT_PERMIT_WRITE,
        0,
        tempHumidHistoryTime
      },    
      
    // ��ʷ������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidHistoryDataProps
    },

      // ��ʷ��������ֵ
      {
        { ATT_UUID_SIZE, tempHumidHistoryDataUUID },
        GATT_PERMIT_READ,
        0,
        tempHumidHistoryData
      },         
};







/*
* �ֲ�����
*/
// ����Ӧ�ò���Ļص�����ʵ��
static tempHumidServiceCBs_t *tempHumidService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 tempHumid_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// д���Իص�
static bStatus_t tempHumid_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// ����״̬�ı�ص�
static void tempHumid_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// �����Э��ջ�Ļص��ṹ��ʵ��
CONST gattServiceCBs_t tempHumidServCBs =
{
  tempHumid_ReadAttrCB,      // Read callback function pointer
  tempHumid_WriteAttrCB,     // Write callback function pointer
  NULL                       // Authorization callback function pointer
};


static uint8 tempHumid_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
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
    case TEMPHUMID_DATA_UUID:
    case TEMPHUMID_HISTORY_DATA_UUID:
      // ����ʪ��ֵ
      *pLen = TEMPHUMID_DATA_LEN;
      VOID osal_memcpy( pValue, pAttr->pValue, TEMPHUMID_DATA_LEN );
      break;
      
    // ��ʵʱ���Ƶ������ֵ�����ǵ��ֽ�  
    case TEMPHUMID_CTRL_UUID:
    case TEMPHUMID_PERI_UUID:
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


static bStatus_t tempHumid_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
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
    case TEMPHUMID_DATA_UUID:
      // ��ʪ��ֵ����д
      break;

    // д�������Ƶ�
    case TEMPHUMID_CTRL_UUID:
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

        if( pAttr->pValue == &tempHumidCtrl )
        {
          notifyApp = TEMPHUMID_CTRL;
        }
      }
      break;

    // д��������
    case TEMPHUMID_PERI_UUID:
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
        if ( pValue[0] >= TEMPHUMID_PERIOD_MIN_V )
        {
          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          *pCurValue = pValue[0];

          if( pAttr->pValue == &tempHumidPeriod )
          {
            notifyApp = TEMPHUMID_PERI;
          }
        }
        else
        {
           status = ATT_ERR_INVALID_VALUE;
        }
      }
      break;
      
    case TEMPHUMID_HISTORY_TIME_UUID:
      // Validate the value
      // Make sure it's not a blob oper
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

        if( pAttr->pValue == tempHumidHistoryTime )
        {
          notifyApp = TEMPHUMID_HISTORYTIME;
        }
      }      
      break;

    // д��ʪ�����ݵ�CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY );
      break;

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && tempHumidService_AppCBs && tempHumidService_AppCBs->pfnTempHumidServiceCB )
  {
    tempHumidService_AppCBs->pfnTempHumidServiceCB( notifyApp );
  }  
  
  return status;
}

static void tempHumid_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, tempHumidDataConfig );
    }
  }
}










/*
 * ��������
*/

// ���ط���
extern bStatus_t TempHumid_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tempHumidDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( tempHumid_HandleConnStatusCB );  
  
  if ( services & TEMPHUMID_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( tempHumidServAttrTbl, 
                                          GATT_NUM_ATTRS( tempHumidServAttrTbl ),
                                          &tempHumidServCBs );
  }

  return ( status );
}

// �Ǽ�Ӧ�ò���Ļص�
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    tempHumidService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

// ���ñ��������������
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // ������ʪ�����ݣ�����Notification
    case TEMPHUMID_DATA:
      if ( len == TEMPHUMID_DATA_LEN )
      {
        VOID osal_memcpy( tempHumidData, value, TEMPHUMID_DATA_LEN );
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( tempHumidDataConfig, tempHumidData, FALSE,
                                   tempHumidServAttrTbl, GATT_NUM_ATTRS( tempHumidServAttrTbl ),
                                   INVALID_TASK_ID );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // ���ò������Ƶ�
    case TEMPHUMID_CTRL:
      if ( len == sizeof ( uint8 ) )
      {
        tempHumidCtrl = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // ���ò�������  
    case TEMPHUMID_PERI:
      if ( len == sizeof ( uint8 ) )
      {
        tempHumidPeriod = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    // ������ʷ���ݵ�ʱ��
    case TEMPHUMID_HISTORYTIME:
      if ( len == 2 )
      {
        VOID osal_memcpy( tempHumidHistoryTime, value, len );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;   
      
    // ������ʷ����
    case TEMPHUMID_HISTORYDATA:
      if ( len == TEMPHUMID_DATA_LEN )
      {
        VOID osal_memcpy( tempHumidHistoryData, value, len );
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
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // ��ȡ��ʪ������
    case TEMPHUMID_DATA:
      VOID osal_memcpy( value, tempHumidData, TEMPHUMID_DATA_LEN );
      break;

    // ��ȡ�������Ƶ�  
    case TEMPHUMID_CTRL:
      *((uint8*)value) = tempHumidCtrl;
      break;

    // ��ȡ��������  
    case TEMPHUMID_PERI:
      *((uint8*)value) = tempHumidPeriod;
      break;
      
    // ��ȡ��ʷ���ݵ�ʱ��
    case TEMPHUMID_HISTORYTIME:
      VOID osal_memcpy( value, tempHumidHistoryTime, 2 );
      break;
      
    // ��ȡ��ʷ����
    case TEMPHUMID_HISTORYDATA:
      VOID osal_memcpy( value, tempHumidHistoryData, TEMPHUMID_DATA_LEN );
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

