/**
* ������ʪ�ȷ����ṩʵʱ��ʪ�Ȳ������������ȥ24Сʱ��ʷ��������
*/

#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H


// �������λ
#define TEMPHUMID_DATA          0       //ʵʱ��ʪ������
#define TEMPHUMID_CTRL          1       //ʵʱ��������
#define TEMPHUMID_PERI          2       //ʵʱ��������
#define TEMPHUMID_HISTORYTIME   3       //��ʷ�������ݵ�ʱ���
#define TEMPHUMID_HISTORYDATA   4       //��ʷ��������


// �����������16λUUID
#define TEMPHUMID_SERV_UUID               0xAA60     // ��ʪ�ȷ���UUID
#define TEMPHUMID_DATA_UUID               0xAA61     // ʵʱ��ʪ������UUID
#define TEMPHUMID_CTRL_UUID               0xAA62     // ʵʱ��������UUID
#define TEMPHUMID_PERI_UUID               0xAA63     // ʵʱ��������UUID

/*��ʷ���ݵĲ�������������ʱ�����еĶ�ʱ���ڣ��ɶ�ʱ���ڿ���*/
#define TEMPHUMID_HISTORY_TIME_UUID       0xAA64      // ��ʷ�������ݵ�ʱ��UUID
#define TEMPHUMID_HISTORY_DATA_UUID       0xAA65      // ��ʷ��������UUID


// �����bit field
#define TEMPHUMID_SERVICE               0x00000001


// ��ʪ��ֵ���ֽڳ���
#define TEMPHUMID_DATA_LEN             8       // ����float�ĳ���

#define TEMPHUMID_PERIOD_MIN_V         5       //��С��������Ϊ500ms

#define TEMPHUMID_PERIOD_UNIT          100     //�������ڵĵ�λ��100ms


// ��ҪӦ�ò��ṩ�Ļص���������
// �����������仯ʱ��֪ͨӦ�ò�
typedef NULL_OK void (*tempHumidServiceCB_t)( uint8 paramID );

// ��ҪӦ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  tempHumidServiceCB_t        pfnTempHumidServiceCB;
} tempHumidServiceCBs_t;


// ���ر�����
extern bStatus_t TempHumid_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// ������������
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ��������
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

#endif












