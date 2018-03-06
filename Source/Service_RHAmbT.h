
/*

��Si7021ʵ�ֻ���ʪ�Ⱥ��¶Ȳ���
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

// Service Parameters��������ʾҪ������Characteristic
#define RHAMBT_VALUE                          0
#define RHAMBT_VALUE_CHAR_CFG                 1
  
// Value��Attribute Table�е�λ��
#define RHAMBT_VALUE_POS                      2
 

// RHAMBT Service UUIDs���Զ����
#define RHAMBT_SERV_UUID                 0xFFF0 //service
#define RHAMBT_VALUE_UUID                0xFFF1 //RH and Ambient Temperature


// Maximum length of thermometer
// measurement characteristic
//#define THERMOMETER_MEAS_MAX  (ATT_MTU_SIZE -5)

  
// Service bit fields
#define RHAMBT_SERVICE                      0x00000001

// Callback events����Щ�¼�������֪ͨӦ�ò�Ҫ������¼�
#define RHAMBT_VALUE_IND_ENABLED          1
#define RHAMBT_VALUE_IND_DISABLED         2


/*********************************************************************
 * TYPEDEFS
 */

// Service callback function��Ӧ�ò�Ļص�����
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
//����RHAmbT����
extern bStatus_t RHAmbT_AddService( uint32 services );

/*
 * Thermometer_Register - Register a callback function with the
 *          Thermometer Service
 *
 * @param   pfnServiceCB - Callback function.
 */
//�Ǽ�Ӧ�ò�ص�����
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
//��Ӧ�ò����÷������������
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
//��Ӧ�ò��ȡָ���ķ�����������
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
//����һ������ֵ��Indication
extern bStatus_t RHAmbT_ValueIndicate( uint16 connHandle, attHandleValueInd_t *pNoti, uint8 taskId );



/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_RHAMBT_H */
