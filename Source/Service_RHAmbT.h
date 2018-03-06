
/*

用Si7021实现环境湿度和温度测量
Written by chenm, 2017-05-10

*/

#ifndef SERVICE_RHAMBT_H
#define SERVICE_RHAMBT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Service Parameters，用来表示要操作的Characteristic
#define RHAMBT_VALUE                          0
#define RHAMBT_VALUE_CHAR_CFG                 1
  
// Value在Attribute Table中的位置
#define RHAMBT_VALUE_POS                      2
 

// RHAMBT Service UUIDs，自定义的
#define RHAMBT_SERV_UUID                 0xFFF0 //service
#define RHAMBT_VALUE_UUID                0xFFF1 //RH and Ambient Temperature


// Maximum length of thermometer
// measurement characteristic
//#define THERMOMETER_MEAS_MAX  (ATT_MTU_SIZE -5)

  
// Service bit fields
#define RHAMBT_SERVICE                      0x00000001

// Callback events，这些事件是用来通知应用层要处理的事件
#define RHAMBT_VALUE_IND_ENABLED          1
#define RHAMBT_VALUE_IND_DISABLED         2


/*********************************************************************
 * TYPEDEFS
 */

// Service callback function，应用层的回调函数
typedef void (*RHAmbTAppServiceCB_t)(uint8 event);


  
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */


/*********************************************************************
 * API FUNCTIONS 
 */

/*
 * Thermometer_AddService- Initializes the Thermometer service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */
//加载RHAmbT服务
extern bStatus_t RHAmbT_AddService( uint32 services );

/*
 * Thermometer_Register - Register a callback function with the
 *          Thermometer Service
 *
 * @param   pfnServiceCB - Callback function.
 */
//登记应用层回调函数
extern void RHAmbT_Register( RHAmbTAppServiceCB_t pfnServiceCB );

/*
 * Thermometer_SetParameter - Set a Thermometer parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
//让应用层设置服务的特征参数
extern bStatus_t RHAmbT_SetParameter( uint8 param, uint8 len, void *value );
  
/*
 * Thermometer_GetParameter - Get a Thermometer parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
//让应用层获取指定的服务特征参数
extern bStatus_t RHAmbT_GetParameter( uint8 param, void *value );

/*********************************************************************
 * @fn          Thermometer_TempIndicate
 *
 * @brief       Send an Indication containing a thermometer
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
//发送一个测量值的Indication
extern bStatus_t RHAmbT_ValueIndicate( uint16 connHandle, attHandleValueInd_t *pNoti, uint8 taskId );



/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_RHAMBT_H */
