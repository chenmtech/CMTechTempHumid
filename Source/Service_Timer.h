/**
* ��ʱ�����ṩһ�����ȵļ�ʱ����
* ��ʱ���Ȳ���ܸߣ���λ������
*/

#ifndef SERVICE_TIMER_H
#define SERVICE_TIMER_H


// �������λ
#define TIMER_VALUE                   0


// �����������16λUUID
#define TIMER_SERV_UUID               0xAA70     // ��ʱ����UUID
#define TIMER_VALUE_UUID              0xAA71     // ��ʱ����ֵUUID


// �����bit field
#define TIMER_SERVICE               0x00000001


// ��ҪӦ�ò��ṩ�Ļص���������
// �����������仯ʱ��֪ͨӦ�ò�
typedef NULL_OK void (*timerServiceCB_t)( uint8 paramID );

// ��ҪӦ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  timerServiceCB_t        pfnTimerServiceCB;
} timerServiceCBs_t;


// ���ر�����
extern bStatus_t Timer_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t Timer_RegisterAppCBs( timerServiceCBs_t *appCallbacks );

// ������������
extern bStatus_t Timer_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ��������
extern bStatus_t Timer_GetParameter( uint8 param, void *value );

#endif