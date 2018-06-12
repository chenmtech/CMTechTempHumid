
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
* 常量
*/
// 产生服务和特征的128位UUID
// 温湿度服务UUID
CONST uint8 tempHumidServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_SERV_UUID)
};

// 实时温湿度数据UUID
CONST uint8 tempHumidDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_DATA_UUID)
};

// 实时测量控制点UUID
CONST uint8 tempHumidCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_CTRL_UUID)
};

// 实时测量周期UUID
CONST uint8 tempHumidPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_PERI_UUID)
};

// 历史数据的时间UUID
CONST uint8 tempHumidHistoryTimeUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_HISTORY_TIME_UUID)
};

// 历史数据UUID
CONST uint8 tempHumidHistoryDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_HISTORY_DATA_UUID)
};






/*
* 局部变量
*/
// 温湿度服务属性值
static CONST gattAttrType_t tempHumidService = { ATT_UUID_SIZE, tempHumidServUUID };

// 实时温湿度数据的相关属性：可读可通知
static uint8 tempHumidDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 tempHumidData[TEMPHUMID_DATA_LEN] = {0};
static gattCharCfg_t tempHumidDataConfig[GATT_MAX_NUM_CONN];

// 实时测量控制点的相关属性：可读可写
static uint8 tempHumidCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 tempHumidCtrl = 0;

// 实时测量周期的相关属性：可读可写
static uint8 tempHumidPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 tempHumidPeriod = 0;  

// 历史测量数据的时间相关属性：可写
static uint8 tempHumidHistoryTimeProps = GATT_PROP_WRITE;
static uint8 tempHumidHistoryTime[2] = {0}; 

// 历史数据的相关属性：可读
static uint8 tempHumidHistoryDataProps = GATT_PROP_READ;
static uint8 tempHumidHistoryData[TEMPHUMID_DATA_LEN] = {0}; 



// 服务的属性表
static gattAttribute_t tempHumidServAttrTbl[] = 
{
  // tempHumid 服务
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&tempHumidService                /* pValue */
  },

    // 实时温湿度数据特征声明
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &tempHumidDataProps 
    },

      // 实时温湿度数据特征值
      { 
        { ATT_UUID_SIZE, tempHumidDataUUID },
        GATT_PERMIT_READ, 
        0, 
        tempHumidData 
      }, 
      
      // 实时温湿度数据特征的配置
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)tempHumidDataConfig
      },
      
    // 实时测量控制点特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidCtrlProps
    },

      // 实时测量控制点特征值
      {
        { ATT_UUID_SIZE, tempHumidCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &tempHumidCtrl
      },

    // 实时测量周期特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidPeriodProps
    },

      // 实时测量周期特征值
      {
        { ATT_UUID_SIZE, tempHumidPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &tempHumidPeriod
      },
      
    // 历史数据的时间特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidHistoryTimeProps
    },

      // 历史数据时间特征值
      {
        { ATT_UUID_SIZE, tempHumidHistoryTimeUUID },
        GATT_PERMIT_WRITE,
        0,
        tempHumidHistoryTime
      },    
      
    // 历史数据特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidHistoryDataProps
    },

      // 历史数据特征值
      {
        { ATT_UUID_SIZE, tempHumidHistoryDataUUID },
        GATT_PERMIT_READ,
        0,
        tempHumidHistoryData
      },         
};







/*
* 局部函数
*/
// 保存应用层给的回调函数实例
static tempHumidServiceCBs_t *tempHumidService_AppCBs = NULL;

// 服务给协议栈的回调函数
// 读属性回调
static uint8 tempHumid_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// 写属性回调
static bStatus_t tempHumid_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// 连接状态改变回调
static void tempHumid_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 服务给协议栈的回调结构体实例
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
      // 读温湿度值
      *pLen = TEMPHUMID_DATA_LEN;
      VOID osal_memcpy( pValue, pAttr->pValue, TEMPHUMID_DATA_LEN );
      break;
      
    // 读实时控制点和周期值，都是单字节  
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
      // 温湿度值不能写
      break;

    // 写测量控制点
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

    // 写测量周期
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

    // 写温湿度数据的CCC  
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
 * 公共函数
*/

// 加载服务
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

// 设置本服务的特征参数
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // 设置温湿度数据，触发Notification
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

    // 设置测量控制点
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

    // 设置测量周期  
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
      
    // 设置历史数据的时间
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
      
    // 设置历史数据
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

// 获取本服务特征参数
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // 获取温湿度数据
    case TEMPHUMID_DATA:
      VOID osal_memcpy( value, tempHumidData, TEMPHUMID_DATA_LEN );
      break;

    // 获取测量控制点  
    case TEMPHUMID_CTRL:
      *((uint8*)value) = tempHumidCtrl;
      break;

    // 获取测量周期  
    case TEMPHUMID_PERI:
      *((uint8*)value) = tempHumidPeriod;
      break;
      
    // 获取历史数据的时间
    case TEMPHUMID_HISTORYTIME:
      VOID osal_memcpy( value, tempHumidHistoryTime, 2 );
      break;
      
    // 获取历史数据
    case TEMPHUMID_HISTORYDATA:
      VOID osal_memcpy( value, tempHumidHistoryData, TEMPHUMID_DATA_LEN );
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

