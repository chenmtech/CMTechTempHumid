/**************************************************************************************************
* CMTechTempHumid.h : ��Ӧ��ͷ�ļ�
**************************************************************************************************/

#ifndef CMTECHTEMPHUMID_H
#define CMTECHTEMPHUMID_H


/*********************************************************************
 * ����
 */


// ��ʪ�������¼�
#define TEMPHUMID_START_DEVICE_EVT                   0x0001     // �豸�����¼�
#define TEMPHUMID_START_PERIODIC_EVT                 0x0002     // ���ڲ��������¼�


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void TempHumid_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 TempHumid_ProcessEvent( uint8 task_id, uint16 events );



/*********************************************************************
*********************************************************************/


#endif 
