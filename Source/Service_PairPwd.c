
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
* ����
*/
// ���������������128λUUID
// ��ʱ����UUID
CONST uint8 pairPwdServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(PAIRPWD_SERV_UUID)
};

CONST uint8 pairPwdValueUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(PAIRPWD_VALUE_UUID)
};




/*
* �ֲ�����
*/
// ��������ֵ
static CONST gattAttrType_t pairPwdService = { ATT_UUID_SIZE, pairPwdServUUID };

// ��ǰʱ���������ԣ��ɶ���д
static uint8 pairPwdValueProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 nothing;   


// ��������Ա�
static gattAttribute_t pairPwdServAttrTbl[] = 
{
  // ����������
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&pairPwdService                /* pValue */
  },

    // ���������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &pairPwdValueProps
    },

      // �����������ֵ
      {
        { ATT_UUID_SIZE, pairPwdValueUUID },
        GATT_PERMIT_AUTHEN_READ | GATT_PERMIT_AUTHEN_WRITE,
        0,
        (uint8*)(&nothing)
      },      
};


/*
* �ֲ�����
*/
// ����Ӧ�ò���Ļص�����ʵ��
static pairPwdServiceCBs_t * pairPwdService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 pairPwd_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// д���Իص�
static bStatus_t pairPwd_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// ����״̬�ı�ص�
static void pairPwd_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// �����Э��ջ�Ļص��ṹ��ʵ��
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
    // д�������ֵ
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
 * ��������
*/

// ���ط���
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

// �Ǽ�Ӧ�ò���Ļص�
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

