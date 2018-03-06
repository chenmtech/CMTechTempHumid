

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "Service_RHAmbT.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */




/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// service
CONST uint8 RHAmbTServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(RHAMBT_SERV_UUID), HI_UINT16(RHAMBT_SERV_UUID)
};

// value characteristic
CONST uint8 RHAmbTValueUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(RHAMBT_VALUE_UUID), HI_UINT16(RHAMBT_VALUE_UUID)
};





/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
//��������Ӧ�ò�Ļص�����
static RHAmbTAppServiceCB_t appServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

// Service attribute��Service��Attribute����
static CONST gattAttrType_t RHAmbTService = { ATT_BT_UUID_SIZE, RHAmbTServUUID };

// Thermometer Temperature Characteristic
static uint8 RHAmbTValueProps = GATT_PROP_INDICATE;
static uint8 RHAmbTValue = 0;
static gattCharCfg_t RHAmbTValueConfig[GATT_MAX_NUM_CONN];  //ÿ�������϶�Ӧ��һ������ֵ


/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t RHAmbTAttrTbl[] = 
{
  // Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&RHAmbTService                   /* pValue */
  },
      
    // VALUE
    // 1. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &RHAmbTValueProps 
    },

    // 2. Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, RHAmbTValueUUID },
      0, 
      0, 
      &RHAmbTValue 
    },

    // 3. Characteristic Configuration 
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&RHAmbTValueConfig
    },

};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 RHAmbT_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t RHAmbT_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static void RHAmbT_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Service Callbacks������Service��GATT�Ļص�����Ҫ��Ӧ�ò�Ļص������
CONST gattServiceCBs_t RHAmbTCBs =
{
  RHAmbT_ReadAttrCB,  // Read callback function pointer
  RHAmbT_WriteAttrCB, // Write callback function pointer
  NULL                     // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Thermometer_AddService
 *
 * @brief   Initializes the Thermometer   service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t RHAmbT_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  // ָ��INVALID_CONNHANDLE����ָ���е�����
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, RHAmbTValueConfig );

  // Register with Link DB to receive link status change callback
  // �Ǽ�����״̬�ı�Ļص�����
  VOID linkDB_Register( RHAmbT_HandleConnStatusCB );  
  
  if ( services & RHAMBT_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    // �Ǽ�GATT���Ա��Service��GATT�Ļص�����
    status = GATTServApp_RegisterService( RHAmbTAttrTbl, 
                                          GATT_NUM_ATTRS( RHAmbTAttrTbl ),
                                          &RHAmbTCBs );
  }

  return ( status );
}

/*********************************************************************
 * @fn      Thermometer_Register
 *
 * @brief   Register a callback function with the Thermometer Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
// �Ǽ�Ӧ�ò�ص�����
extern void RHAmbT_Register( RHAmbTAppServiceCB_t pfnServiceCB )
{
  appServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      Thermometer_SetParameter
 *
 * @brief   Set a thermomter parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
// ��Ӧ�ò���ã����÷�������ֵ
bStatus_t RHAmbT_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case RHAMBT_VALUE_CHAR_CFG:      
      // Need connection handle
      //thermometerTempConfig.value = *((uint8*)value);
      break;
    
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      Thermometer_GetParameter
 *
 * @brief   Get a Thermometer   parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
// ��Ӧ�ò���ã���ȡ��������ֵ
bStatus_t RHAmbT_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case RHAMBT_VALUE_CHAR_CFG:
        // Need connection handle
        //*((uint16*)value) = thermometerTempConfig.value;
        break;      
      
    default:
        ret = INVALIDPARAMETER;
        break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          Thermometer_TempIndicate
 *
 * @brief       Send a indication containing a thermometer
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
// ����һ���¶Ȳ�����Indication
bStatus_t RHAmbT_ValueIndicate( uint16 connHandle, attHandleValueInd_t *pNoti, uint8 taskId )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, RHAmbTValueConfig );

  // If indications enabled���������Indication
  if ( value & GATT_CLIENT_CFG_INDICATE )
  {
    // Set the handle (uses stored relative handle to lookup actual handle)
    pNoti->handle = RHAmbTAttrTbl[pNoti->handle].handle;
  
    // Send the Indication, ����Indication
    return GATT_Indication( connHandle, pNoti, FALSE, taskId );
  }

  return bleIncorrectMode;
}


/*********************************************************************
 * @fn          thermometer_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
// ��һ��Attribute��ֵ�ŵ�pValue��ȥ
static uint8 RHAmbT_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  //blob��Binary Large Object, �����ƴ����ݶ���
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      default:
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      thermometer_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written 
 *
 * @return  Success or Failure
 */
// дһ��Attribute����pValueд��pAttr��ȥ
static bStatus_t RHAmbT_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                          uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;

  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  switch ( uuid )
  {
    case  GATT_CLIENT_CHAR_CFG_UUID:  //�ͻ����޸���������CCC
      //����GATT����
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                               offset, GATT_CLIENT_CFG_INDICATE );  
      //����ɹ�����֪ͨӦ�ò�
      if ( status == SUCCESS )
      {
        uint16 value = BUILD_UINT16( pValue[0], pValue[1] );      
      
        //֪ͨӦ�ò�TEMP INDICATION�����Ѿ��޸���
        (*appServiceCB)( (value == GATT_CFG_NO_OPERATION) ? 
                                 RHAMBT_VALUE_IND_DISABLED :
                                 RHAMBT_VALUE_IND_ENABLED );    
      }
      break;
    
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  
  return ( status );
}


/*********************************************************************
 * @fn          thermometer_HandleConnStatusCB
 *
 * @brief       Simple Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
// ����״̬�ı�ص�����
static void RHAmbT_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, RHAmbTValueConfig );
    }
  }
}


/*********************************************************************
*********************************************************************/
