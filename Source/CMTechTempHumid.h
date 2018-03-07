/**************************************************************************************************
* CMTechTempHumid.h : 主应用头文件
**************************************************************************************************/

#ifndef CMTECHTEMPHUMID_H
#define CMTECHTEMPHUMID_H


/*********************************************************************
 * 常量
 */


// 温湿度任务事件
#define TEMPHUMID_START_DEVICE_EVT                   0x0001     // 设备启动事件
#define TEMPHUMID_START_PERIODIC_EVT                 0x0002     // 周期采集启动事件

// 测量控制标记值
#define TEMPHUMID_CTRL_STOP               0x00    // 停止测量
#define TEMPHUMID_CTRL_START              0x01    // 开始测量 

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
