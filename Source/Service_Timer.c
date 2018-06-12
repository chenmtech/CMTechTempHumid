
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
* 常量
*/
// 产生服务和特征的128位UUID
// 定时服务UUID
CONST uint8 timerServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_SERV_UUID)
};

// 当前时间UUID
CONST uint8 timerCurTimeUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_CURTIME_UUID)
};

// 定时控制点UUID
CONST uint8 timerCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_CTRL_UUID)
};

// 定时周期UUID
CONST uint8 timerPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TIMER_PERIOD_UUID)
};








/*
* 局部变量
*/
// 服务属性值
static CONST gattAttrType_t timerService = { ATT_UUID_SIZE, timerServUUID };

// 当前时间的相关属性：可读可写
static uint8 timerCurTimeProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerCurTime[2] = {0};

// 定时控制点的相关属性：可读可写
static uint8 timerCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerCtrl = 0;

// 定时周期的相关属性：可读可写，单位：分钟
static uint8 timerPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 timerPeriod = 10;    


// 服务的属性表
static gattAttribute_t timerServAttrTbl[] = 
{
  // 计时服务
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&timerService                /* pValue */
  },

    // 当前时间特征声明
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &timerCurTimeProps 
    },

      // 当前时间特征值
      { 
        { ATT_UUID_SIZE, timerCurTimeUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        timerCurTime 
      }, 

      
    // 计时控制点特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &timerCtrlProps
    },

      // 计时控制点特征值
      {
        { ATT_UUID_SIZE, timerCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &timerCtrl
      },

     // 计时周期特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &timerPeriodProps
    },

      // 计时周期特征值
      {
        { ATT_UUID_SIZE, timerPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &timerPeriod
      },
};







/*
* 局部函数
*/
// 保存应用层给的回调函数实例
static timerServiceCBs_t * timerService_AppCBs = NULL;

// 服务给协议栈的回调函数
// 读属性回调
static uint8 timer_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// 写属性回调
static bStatus_t timer_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// 连接状态改变回调
static void timer_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 服务给协议栈的回调结构体实例
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
      // 读当前时间值
      *pLen = 2;
      VOID osal_memcpy( pValue, pAttr->pValue, 2 );
      break;
      
    // 读定时控制点和定时周期值，都是单字节  
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
      // 写当前时间值
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

    // 写定时控制点
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

    // 写定时周期
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
        // 定时周期必须在1-60分钟之间
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
 * 公共函数
*/

// 加载服务
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

// 登记应用层给的回调
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

// 设置本服务的特征参数
extern bStatus_t Timer_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // 设置当前时间
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

    // 设置定时控制点
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

    // 设置定时周期  
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

// 获取本服务特征参数
extern bStatus_t Timer_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // 获取当前时间
    case TIMER_CURTIME:
      VOID osal_memcpy( value, timerCurTime, 2 );
      break;

    // 获取定时控制点  
    case TIMER_CTRL:
      *((uint8*)value) = timerCtrl;
      break;

    // 获取定时周期  
    case TIMER_PERIOD:
      *((uint8*)value) = timerPeriod;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

