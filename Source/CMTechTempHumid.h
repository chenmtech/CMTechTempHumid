/**************************************************************************************************
* CMTechTempHumid.h : Ӧ����ͷ�ļ�
**************************************************************************************************/

#ifndef CMTECHTEMPHUMID_H
#define CMTECHTEMPHUMID_H

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


// CMTech Thermometer Task Events
#define TH_START_DEVICE_EVT                   0x0001     // �����豸�¼�
#define TH_PERIODIC_EVT                       0x0002     // ���ڲɼ��¶��¼� 
#define TH_CALIBRATION_EVT                    0x0004     // �궨�¼�
#define TH_SWITCH_MODE_EVT                    0x0008     // ����ģʽת���¼�
#define TH_DP_PRECAST_EVT                     0x0010     // Ԥ���¶��¼�
#define TH_DP_STABLE_EVT                      0x0020     // �����ȶ��¼�
#define TH_TONE_ON_EVT                        0x0040     // ��������
#define TH_TONE_OFF_EVT                       0x0080     // ������ͣ  
#define TH_STOP_SHOW_PRETEMP_EVT              0x0100     // ֹͣ��ʾԤ���¶�ֵ

// �������Ʊ��ֵ
#define TEMPHUMID_CTRL_STOP               0x00    // ֹͣ����
#define TEMPHUMID_CTRL_START              0x01    // ��ʼ���� 

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

#ifdef __cplusplus
}
#endif

#endif 
