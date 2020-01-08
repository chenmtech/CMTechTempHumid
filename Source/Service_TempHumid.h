/**
* �Զ��廷����ʪ�ȷ����ṩʵʱ��ʪ�Ȳ�������
*/

#ifndef SERVICE_TEMPHUMID_H
#define SERVICE_TEMPHUMID_H


// ��ʪ�ȷ������
#define TEMPHUMID_DATA                   0       //ʵʱ��ʪ������
#define TEMPHUMID_DATA_CHAR_CFG          1       //ʵʱ��������
#define TEMPHUMID_INTERVAL               2       //ʵʱ�������
#define TEMPHUMID_IRANGE                 3       //ʵʱ���������Χ

// ��ʪ���������������Ա��е�����
#define TEMPHUMID_DATA_POS               2

// ����16λUUID
#define TEMPHUMID_SERV_UUID               0xAA60     // ��ʪ�ȷ���UUID
#define TEMPHUMID_DATA_UUID               0xAA61     // ʵʱ��ʪ������UUID
#define TEMPHUMID_INTERVAL_UUID           0x2A21     // ʵʱ�������UUID
#define TEMPHUMID_IRANGE_UUID             0x2906     // ʵʱ���������ΧUUID



// �����bit field
#define TEMPHUMID_SERVICE               0x00000001


// ��ʪ�����ݵ��ֽڳ���
#define TEMPHUMID_DATA_LEN             4       


// �ص��¼�
#define TEMPHUMID_DATA_IND_ENABLED          1
#define TEMPHUMID_DATA_IND_DISABLED         2
#define TEMPHUMID_INTERVAL_SET              3

/**
 * Thermometer Interval Range 
 */
typedef struct
{
  uint16 low;         
  uint16 high; 
} tempHumidIRange_t;


// ��ĳ���¼�����ʱ���õĻص���������
typedef void (*tempHumidServiceCB_t)( uint8 event );


// ��Ӧ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  tempHumidServiceCB_t pfnTempHumidServiceCB;
} tempHumidServiceCBs_t;


// ���ر�����
extern bStatus_t TempHumid_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t TempHumid_RegisterAppCBs( tempHumidServiceCBs_t *appCallbacks );

// ������������
extern bStatus_t TempHumid_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ��������
extern bStatus_t TempHumid_GetParameter( uint8 param, void *value );

extern bStatus_t TempHumid_TempHumidIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId );

#endif












