
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


// 产生服务和特征的128位UUID
CONST uint8 thermoServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_SERV_UUID)
};

CONST uint8 thermoDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_DATA_UUID)
};

CONST uint8 thermoCfgUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_CONF_UUID)
};

CONST uint8 thermoPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_PERI_UUID)
};



// 服务属性值
static CONST gattAttrType_t thermoService = { ATT_UUID_SIZE, thermoServUUID };


static uint8 thermoDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 thermoData[2] = {0, 0};
static gattCharCfg_t thermoDataConfig[GATT_MAX_NUM_CONN];


static uint8 thermoCfgProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 thermoCfg = 0;


static uint8 thermoPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 thermoPeriod = 0;    



// 服务的属性表
static gattAttribute_t thermoServAttrTbl[] = 
{
  // thermo Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&thermoService                   /* pValue */
  },

    // thermo data Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &thermoDataProps 
    },

      // thermo data Characteristic Value
      { 
        { ATT_UUID_SIZE, thermoDataUUID },
        GATT_PERMIT_READ, 
        0, 
        thermoData 
      }, 
      
      // Characteristic configuration
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)thermoDataConfig
      },
      
    // Characteristic Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &thermoCfgProps
    },

      // Characteristic Value "Configuration"
      {
        { ATT_UUID_SIZE, thermoCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &thermoCfg
      },

     // Characteristic Declaration "Period"
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &thermoPeriodProps
    },

      // Characteristic Value "Period"
      {
        { ATT_UUID_SIZE, thermoPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &thermoPeriod
      },
};

// 保存应用层给的回调函数
static tempHumidServiceCBs_t *tempHumidService_AppCBs = NULL;


// 服务给协议栈的回调函数
static uint8 tempHumid_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t tempHumid_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static void tempHumid_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 服务给协议栈的回调结构体实例
CONST gattServiceCBs_t thermoServCBs =
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
      // 读身高值的时候，启动测量
      *pLen = TEMPHUMID_DATA_LEN;
      VOID osal_memcpy( pValue, pAttr->pValue, TEMPHUMID_DATA_LEN );
      break;
      
    case TEMPHUMID_CONF_UUID:
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
      // Should not get here
      break;

    case TEMPHUMID_CONF_UUID:
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

        if( pAttr->pValue == &thermoCfg )
        {
          notifyApp = TEMPHUMID_CONF;
        }
      }
      break;

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
        if ( pValue[0] >= (TEMPHUMID_MIN_PERIOD/TEMPHUMID_TIME_UNIT) )
        {

          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          *pCurValue = pValue[0];

          if( pAttr->pValue == &thermoPeriod )
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
      GATTServApp_InitCharCfg( connHandle, thermoDataConfig );
      //GATTServApp_InitCharCfg( connHandle, simpleProfileChar6Config );
    }
  }
}







// 下面是外部函数

// 加载服务
extern bStatus_t TempHumid_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, thermoDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( tempHumid_HandleConnStatusCB );  
  
  if ( services & TEMPHUMID_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( thermoServAttrTbl, 
                                          GATT_NUM_ATTRS( thermoServAttrTbl ),
                                          &thermoServCBs );
  }

  return ( status );
}





// 登记应用层给的回调
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





extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case TEMPHUMID_DATA:
    if ( len == TEMPHUMID_DATA_LEN )
    {
      VOID osal_memcpy( thermoData, value, TEMPHUMID_DATA_LEN );
      // See if Notification has been enabled
      GATTServApp_ProcessCharCfg( thermoDataConfig, thermoData, FALSE,
                                 thermoServAttrTbl, GATT_NUM_ATTRS( thermoServAttrTbl ),
                                 INVALID_TASK_ID );
    }
    else
    {
      ret = bleInvalidRange;
    }
    break;

    case TEMPHUMID_CONF:
      if ( len == sizeof ( uint8 ) )
      {
        thermoCfg = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case TEMPHUMID_PERI:
      if ( len == sizeof ( uint8 ) )
      {
        thermoPeriod = *((uint8*)value);
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




extern bStatus_t TempHumid_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    case TEMPHUMID_DATA:
      VOID osal_memcpy( value, thermoData, TEMPHUMID_DATA_LEN );
      break;

    case TEMPHUMID_CONF:
      *((uint8*)value) = thermoCfg;
      break;

    case TEMPHUMID_PERI:
      *((uint8*)value) = thermoPeriod;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

