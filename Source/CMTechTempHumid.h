/**************************************************************************************************
* CMTechTempHumid.h : 应用主头文件
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
#define TH_START_DEVICE_EVT                   0x0001     // 启动设备事件
#define TH_PERIODIC_EVT                       0x0002     // 周期采集温度事件 
#define TH_CALIBRATION_EVT                    0x0004     // 标定事件
#define TH_SWITCH_MODE_EVT                    0x0008     // 工作模式转换事件
#define TH_DP_PRECAST_EVT                     0x0010     // 预测温度事件
#define TH_DP_STABLE_EVT                      0x0020     // 测温稳定事件
#define TH_TONE_ON_EVT                        0x0040     // 蜂鸣器响
#define TH_TONE_OFF_EVT                       0x0080     // 蜂鸣器停  
#define TH_STOP_SHOW_PRETEMP_EVT              0x0100     // 停止显示预测温度值

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

#ifdef __cplusplus
}
#endif

#endif 
