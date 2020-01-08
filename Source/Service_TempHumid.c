
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

// ��ʪ������UUID
CONST uint8 tempHumidDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_DATA_UUID)
};

// �������UUID
CONST uint8 tempHumidIntervalUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(TEMPHUMID_INTERVAL_UUID), HI_UINT16(TEMPHUMID_INTERVAL_UUID)
};

// ���������ΧUUID
CONST uint8 tempHumidIRangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(TEMPHUMID_IRANGE_UUID), HI_UINT16(TEMPHUMID_IRANGE_UUID)
};


/*
* �ֲ�����
*/
// ��ʪ�ȷ�����������ֵ
static CONST gattAttrType_t tempHumidService = { ATT_UUID_SIZE, tempHumidServUUID };

// ��ʪ������
static uint8 tempHumidDataProps = GATT_PROP_INDICATE;
static attHandleValueInd_t dataIndi;
static int16* pTemp = (int16*)dataIndi.value; 
static uint16* pHumid = (uint16*)(dataIndi.value+2);
static gattCharCfg_t tempHumidDataConfig[GATT_MAX_NUM_CONN];

// �������
static uint8 tempHumidIntervalProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint16 tempHumidInterval = 1;  

// ���������Χ
static tempHumidIRange_t  tempHumidIRange = {1,3600};

// ��������Ա�
static gattAttribute_t tempHumidAttrTbl[] = 
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

      // ��ʪ����������ֵ
      { 
        { ATT_UUID_SIZE, tempHumidDataUUID },
        GATT_PERMIT_READ, 
        0, 
        dataIndi.value 
      }, 
      
      // ��ʪ������CCC
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)tempHumidDataConfig
      },

    // ���������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidIntervalProps
    },

      // �����������ֵ
      {
        { ATT_UUID_SIZE, tempHumidIntervalUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8*)&tempHumidInterval
      },
      
      { 
        { ATT_BT_UUID_SIZE, tempHumidIRangeUUID },
        GATT_PERMIT_READ,
        0, 
        (uint8 *)&tempHumidIRange 
      },
};


/*
* �ֲ�����
*/
// ����Ӧ�ò���Ļص������ṹ��
static tempHumidServiceCBs_t * tempHumidService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 tempHumid_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

// д���Իص�
static bStatus_t tempHumid_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

// ����״̬�ı�ص�
static void tempHumid_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// �����Э��ջ�Ļص��ṹ��
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
    return ATT_ERR_ATTR_NOT_FOUND;
  }
  
  switch ( uuid )
  {
    // ���������ֵ  
    case TEMPHUMID_INTERVAL_UUID:
      *pLen = 2;
      VOID osal_memcpy(pValue, &tempHumidInterval, 2);
      break;
      
    // �����������Χֵ   
    case TEMPHUMID_IRANGE_UUID:
        *pLen = 4;
         VOID osal_memcpy( pValue, &tempHumidIRange, 4 ) ;
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
  uint16 uuid;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if (utilExtractUuid16(pAttr, &uuid) == FAILURE) {
    return ATT_ERR_ATTR_NOT_FOUND;
  }
  
  switch ( uuid )
  {
    // д��ʪ�����ݵ�CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      if(pAttr->handle == tempHumidAttrTbl[TEMPHUMID_DATA_POS].handle)
      {
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY );
        if(status == SUCCESS)
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          uint8 event = (value == GATT_CFG_NO_OPERATION) ? TEMPHUMID_DATA_IND_DISABLED : TEMPHUMID_DATA_IND_ENABLED;
          tempHumidService_AppCBs->pfnTempHumidServiceCB(event);
        }
      }
      else 
      {
        status = ATT_ERR_INVALID_HANDLE;
      }
      break;

    // д�������
    case TEMPHUMID_INTERVAL_UUID:
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
      
      //validate range
      if ((*pValue >= tempHumidIRange.high) | ((*pValue <= tempHumidIRange.low) & (*pValue != 0)))
      {
        status = ATT_ERR_INVALID_VALUE;
      }
      
      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;        
        // *pCurValue = *pValue; 
        VOID osal_memcpy( pCurValue, pValue, 2 ) ;
        
        //notify application of write
        tempHumidService_AppCBs->pfnTempHumidServiceCB(TEMPHUMID_INTERVAL_SET);
      }
      break;
    
    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
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
    status = GATTServApp_RegisterService( tempHumidAttrTbl, 
                                          GATT_NUM_ATTRS( tempHumidAttrTbl ),
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
    // ���ò�������  
    case TEMPHUMID_INTERVAL:
      if ( len == 2 )
      {
        osal_memcpy(&tempHumidInterval, (uint8*)value, 2);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
    
    // ���ò������ڷ�Χ  
    case TEMPHUMID_IRANGE:    
      if(len == 4)
      {
        tempHumidIRange = *((tempHumidIRange_t*)value);
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
    // ��ȡ�������
    case TEMPHUMID_INTERVAL:
      *((uint8*)value) = tempHumidInterval;
      break;
      
    case TEMPHUMID_IRANGE:
      *((tempHumidIRange_t*)value) = tempHumidIRange;
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t TempHumid_TempHumidIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, tempHumidDataConfig );

  // If indications enabled
  if ( value & GATT_CLIENT_CFG_INDICATE )
  {
    
    // Set the handle (uses stored relative handle to lookup actual handle)
    dataIndi.handle = tempHumidAttrTbl[TEMPHUMID_DATA_POS].handle;
    dataIndi.len = 4;
    *pTemp = temp;
    *pHumid = humid;
  
    // Send the Indication
    return GATT_Indication( connHandle, &dataIndi, FALSE, taskId );
  }

  return bleIncorrectMode;
}