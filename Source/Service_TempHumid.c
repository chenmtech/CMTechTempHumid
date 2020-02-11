
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "cmutil.h"
#include "Dev_Si7021.h"

#include "Service_TempHumid.h"


// T&H service UUID
CONST uint8 tempHumidServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_SERV_UUID)
};

// T&H data UUID
CONST uint8 tempHumidDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(TEMPHUMID_DATA_UUID)
};

// measurement interval UUID
CONST uint8 tempHumidIntervalUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(TEMPHUMID_INTERVAL_UUID), HI_UINT16(TEMPHUMID_INTERVAL_UUID)
};

// interval range UUID
CONST uint8 tempHumidIRangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(TEMPHUMID_IRANGE_UUID), HI_UINT16(TEMPHUMID_IRANGE_UUID)
};


// 
static CONST gattAttrType_t tempHumidService = { ATT_UUID_SIZE, tempHumidServUUID };

static uint8 tempHumidDataProps = GATT_PROP_INDICATE;
static uint8 tempHumidData = 0;
static attHandleValueInd_t dataInd;
static gattCharCfg_t tempHumidDataConfig[GATT_MAX_NUM_CONN];

// interval，units of second
static uint8 tempHumidIntervalProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint16 tempHumidInterval = 15;  

// interval range
static tempHumidIRange_t  tempHumidIRange = {1,3600};

// 服务的属性表
static gattAttribute_t tempHumidAttrTbl[] = 
{
  // tempHumid service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&tempHumidService                /* pValue */
  },

    // T&H data
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &tempHumidDataProps 
    },

      // 
      { 
        { ATT_UUID_SIZE, tempHumidDataUUID },
        GATT_PERMIT_READ, 
        0, 
        &tempHumidData
      }, 
      
      // 
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)tempHumidDataConfig
      },

    // interval
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempHumidIntervalProps
    },

      // 
      {
        { ATT_BT_UUID_SIZE, tempHumidIntervalUUID },
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

static thServiceCBs_t * thService_AppCBs = NULL;

static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static CONST gattServiceCBs_t servCBs =
{
  readAttrCB,      // Read callback function pointer
  writeAttrCB,     // Write callback function pointer
  NULL             // Authorization callback function pointer
};

static void handleConnStatusCB( uint16 connHandle, uint8 changeType );


// add T&H service
extern bStatus_t TempHumid_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tempHumidDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( handleConnStatusCB );  
  
  if ( services & TEMPHUMID_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( tempHumidAttrTbl, 
                                          GATT_NUM_ATTRS( tempHumidAttrTbl ),
                                          &servCBs );
  }

  return ( status );
}

// register service
extern bStatus_t TempHumid_RegisterAppCBs( thServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    thService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

// set service parameter
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // set interval
    case TEMPHUMID_INTERVAL:
      if ( len == 2 )
      {
        osal_memcpy(&tempHumidInterval, (uint8*)value, 2);
      }
      else
      {
        ret = INVALIDPARAMETER;
      }
      break;
    
    // set interval range 
    case TEMPHUMID_IRANGE:    
      if(len == 4)
      {
        tempHumidIRange = *((tempHumidIRange_t*)value);
      }
      else
      {
        ret = INVALIDPARAMETER;
      }
      break;  

    default:
      ret = INVALIDPARAMETER;
      break;
  }  
  
  return ( ret );
}

// get service parameter
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // get interval
    case TEMPHUMID_INTERVAL:
      *((uint16*)value) = tempHumidInterval;
      break;
   
    // get interval range 
    case TEMPHUMID_IRANGE:
      *((tempHumidIRange_t*)value) = tempHumidIRange;
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t TempHumid_TempHumidIndicate( uint16 connHandle, uint8 taskId )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, tempHumidDataConfig );

  // If indications enabled
  if ( value & GATT_CLIENT_CFG_INDICATE )
  {
    uint16 humid = SI7021_MeasureHumidity();
    int16 temp = SI7021_ReadTemperature();
    
    // Set the handle (uses stored relative handle to lookup actual handle)
    dataInd.handle = tempHumidAttrTbl[TEMPHUMID_DATA_POS].handle;
    dataInd.len = 4;
    uint8* p = dataInd.value;
    *p++ = LO_UINT16(temp);
    *p++ = HI_UINT16(temp);
    *p++ = LO_UINT16(humid);
    *p++ = HI_UINT16(humid);
  
    // Send the Indication
    return GATT_Indication( connHandle, &dataInd, FALSE, taskId );
  }

  return bleIncorrectMode;
}


static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
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
    // read interval  
    case TEMPHUMID_INTERVAL_UUID:
      *pLen = 2;
      VOID osal_memcpy(pValue, pAttr->pValue, 2);
      break;
      
    // read interval range 
    case TEMPHUMID_IRANGE_UUID:
      *pLen = 4;
       VOID osal_memcpy( pValue, pAttr->pValue, 4 ) ;
      break;   
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }  
  
  return status;
}


static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;
  uint16 interval;
  
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
    // write T&H data CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_INDICATE );
      
      if(status == SUCCESS)
      {
        uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
        
        uint8 event = (value == GATT_CFG_NO_OPERATION) ? TEMPHUMID_DATA_IND_DISABLED : TEMPHUMID_DATA_IND_ENABLED;
        thService_AppCBs->pfnTHServiceCB(event);
      }
      break;

    // write interval
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
      
      interval = (uint16)*pValue;
      
      //validate range
      if ((interval >= tempHumidIRange.high) | ((interval <= tempHumidIRange.low) & (interval != 0)))
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
        thService_AppCBs->pfnTHServiceCB(TEMPHUMID_INTERVAL_SET);
      }
      break;
    
    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return status;
}

static void handleConnStatusCB( uint16 connHandle, uint8 changeType )
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

