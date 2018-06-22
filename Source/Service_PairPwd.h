/**
* ���������������ṩ�����������в����ķ���
*/

#ifndef SERVICE_PAIRPWD_H
#define SERVICE_PAIRPWD_H


// �������λ
#define PAIRPWD_VALUE                   0


// �����������16λUUID
#define PAIRPWD_SERV_UUID               0xAA80     // ����������UUID
#define PAIRPWD_VALUE_UUID              0xAA81     // �����������ֵUUID


// �����bit field
#define PAIRPWD_SERVICE               0x00000001


// ��ҪӦ�ò��ṩ�Ļص���������
// �����������仯ʱ��֪ͨӦ�ò�
typedef NULL_OK void (*pairPwdServiceCB_t)( uint8 paramID );

// ��ҪӦ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  pairPwdServiceCB_t        pfnPairPwdServiceCB;
} pairPwdServiceCBs_t;


// ���ر�����
extern bStatus_t PAIRPWD_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t PAIRPWD_RegisterAppCBs( pairPwdServiceCBs_t *appCallbacks );


#endif