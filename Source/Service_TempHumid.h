/**
* Temperature&Humid service header file
*/

#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H


// Mark for TempHumid service parameters
#define TEMPHUMID_DATA                   0       // temp&humid data
#define TEMPHUMID_DATA_CHAR_CFG          1       // temp&humid data ccc
#define TEMPHUMID_INTERVAL               2       // temp&humid measurement interval
#define TEMPHUMID_IRANGE                 3       // interval range: [min, max]

// position of temp humid data
#define TEMPHUMID_DATA_POS      2

// UUID of temp&humid service and characteristics
#define TEMPHUMID_SERV_UUID               0xAA60     // temp&humid service UUID
#define TEMPHUMID_DATA_UUID               0xAA61     // temp&humid data UUID
#define TEMPHUMID_INTERVAL_UUID           0x2A21     // measurement interval UUID
#define TEMPHUMID_IRANGE_UUID             0x2906     // measurement interval range UUID

// 
#define TEMPHUMID_SERVICE               0x00000001

// length of temp&humid data
#define TEMPHUMID_DATA_LEN             4       

// callback events
#define TEMPHUMID_DATA_IND_ENABLED          1
#define TEMPHUMID_DATA_IND_DISABLED         2
#define TEMPHUMID_INTERVAL_SET              4

/**
 * T&H Interval Range 
 */
typedef struct
{
  uint16 low;         
  uint16 high; 
} tempHumidIRange_t;


// the callback function
typedef void (*thServiceCB_t)( uint8 event );


// the callback struct
typedef struct
{
  thServiceCB_t pfnTHServiceCB;
} thServiceCBs_t;


// add T&H service
extern bStatus_t TempHumid_AddService( uint32 services );

// register T&H service
extern bStatus_t TempHumid_RegisterAppCBs( thServiceCBs_t *appCallbacks );

// 
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// 
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

// indicate T&H data
extern bStatus_t TempHumid_TempHumidIndicate( uint16 connHandle, uint8 taskId );

#endif












