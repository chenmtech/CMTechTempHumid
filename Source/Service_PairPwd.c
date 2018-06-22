
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "cmutil.h"

#include "Service_PairPwd.h"
#include "app_gapconfig.h"

/*
* 常量
*/
// 产生服务和特征的128位UUID
// 定时服务UUID
CONST uint8 pairPwdServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(PAIRPWD_SERV_UUID)
};

CONST uint8 pairPwdValueUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(PAIRPWD_VALUE_UUID)
};




/*
* 局部变量
*/
// 服务属性值
static CONST gattAttrType_t pairPwdService = { ATT_UUID_SIZE, pairPwdServUUID };

// 当前时间的相关属性：可读可写
static uint8 pairPwdValueProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 nothing;   


// 服务的属性表
static gattAttribute_t pairPwdServAttrTbl[] = 
{
  // 配对密码服务
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&pairPwdService                /* pValue */
  },

    // 配对密码特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &pairPwdValueProps
    },

      // 配对密码特征值
      {
        { ATT_UUID_SIZE, pairPwdValueUUID },
        GATT_PERMIT_AUTHEN_READ | GATT_PERMIT_AUTHEN_WRITE,
        0,
        (uint8*)(&nothing)
      },      
};


/*
* 局部函数
*/
// 保存应用层给的回调函数实例
static pairPwdServiceCBs_t * pairPwdService_AppCBs = NULL;

// 服务给协议栈的回调函数
// 读属性回调
static uint8 pairPwd_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// 写属性回调
static bStatus_t pairPwd_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// 连接状态改变回调
static void pairPwd_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 服务给协议栈的回调结构体实例
CONST gattServiceCBs_t pairPwdServCBs =
{
  pairPwd_ReadAttrCB,           // Read callback function pointer
  pairPwd_WriteAttrCB,          // Write callback function pointer
  NULL                        // Authorization callback function pointer
};


static uint8 pairPwd_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;
  uint32 password;

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
    case PAIRPWD_VALUE_UUID:
      *pLen = sizeof(uint32);
      password = GapConfig_ReadPairPassword();
      VOID osal_memcpy( pValue, (uint8*)(&password), sizeof(uint32) );
      break;
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }  
  
  return status;
}


static bStatus_t pairPwd_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint16 uuid;
  uint32 password;
  
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
    // 写配对密码值
    case PAIRPWD_VALUE_UUID:
     
      if ( offset == 0 )
      {
        if ( len != 4 )
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
        osal_memcpy((uint8*)(&password), pValue, 4);
        
        if(password > GAP_PASSCODE_MAX)
          status = ATT_ERR_INVALID_VALUE;
        else {
          GapConfig_SNV_Password(GAP_PARI_PASSWORD_WRITE, pValue, 4);
        
          notifyApp = PAIRPWD_VALUE;
        }
      }
      
      break;
      

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && pairPwdService_AppCBs && pairPwdService_AppCBs->pfnPairPwdServiceCB )
  {
    pairPwdService_AppCBs->pfnPairPwdServiceCB( notifyApp );
  }  
  
  return status;
}

static void pairPwd_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
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
extern bStatus_t PAIRPWD_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  //GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tempHumidDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( pairPwd_HandleConnStatusCB );  
  
  if ( services & PAIRPWD_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( pairPwdServAttrTbl, 
                                          GATT_NUM_ATTRS( pairPwdServAttrTbl ),
                                          &pairPwdServCBs );
  }

  return ( status );
}

// 登记应用层给的回调
extern bStatus_t PAIRPWD_RegisterAppCBs( pairPwdServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    pairPwdService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

