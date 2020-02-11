/**************************************************************************************************
* Temp&Humid application header file
**************************************************************************************************/

#ifndef CMTECHTEMPHUMID_H
#define CMTECHTEMPHUMID_H


// 
#define TEMPHUMID_START_DEVICE_EVT 0x0001     // 
#define TEMPHUMID_MEAS_PERIODIC_EVT 0x0002     // temp&humid periodic measurement event


/*
 * Task Initialization for the BLE Application
 */
extern void TempHumid_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 TempHumid_ProcessEvent( uint8 task_id, uint16 events );


#endif 
