
#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H

// �������
#define TEMPHUMID_DATA    0       //����ֵ
#define TEMPHUMID_CONF    1       //�������Ƶ�
#define TEMPHUMID_PERI    2       //��������




// �����������16λUUID
#define TEMPHUMID_SERV_UUID    0xAA60     // ��ʪ�ȷ���UUID
#define TEMPHUMID_DATA_UUID    0xAA31
#define TEMPHUMID_CONF_UUID    0xAA32
#define TEMPHUMID_PERI_UUID    0xAA33


// �����bit field
#define TEMPHUMID_SERVICE               0x00000001

// ����ֵ���ֽڳ���
#define TEMPHUMID_DATA_LEN     2

#define TEMPHUMID_MIN_PERIOD        1000    //��С��������Ϊ1000ms

#define TEMPHUMID_TIME_UNIT         1000    //��������ʱ�䵥λΪ1000ms


typedef NULL_OK void (*tempHumidServiceCB_t)( uint8 paramID );


typedef struct
{
  tempHumidServiceCB_t        pfnTempHumidServiceCB;  // Called when characteristic value changes
} tempHumidServiceCBs_t;


// ���ر�����
extern bStatus_t TempHumid_AddService( uint32 services );

// �Ǽ�Ӧ�ò�ص�
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// ��������ֵ
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ����ֵ
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

#endif












