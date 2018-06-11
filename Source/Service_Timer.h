#ifndef SERVICE_TIMER_H
#define SERVICE_TIMER_H


// �������λ
#define TIMER_CURTIME                 0       //��ǰʱ��
#define TIMER_CTRL                    1       //��ʱ����
#define TIMER_PERIOD                  2       //��ʱ����


// �����������16λUUID
#define TIMER_SERV_UUID               0xAA70     // ��ʱ����UUID
#define TIMER_CURTIME_UUID            0xAA71     // ��ǰʱ��UUID
#define TIMER_CTRL_UUID               0xAA72     // ��ʱ����UUID
#define TIMER_PERIOD_UUID             0xAA73     // ��ʱ����


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