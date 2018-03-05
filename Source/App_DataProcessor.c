

#include "App_DataProcessor.h"
#include "App_TempHumid.h"
#include "CMTechTempHumid.h"
#include "OSAL.h"

#include "Dev_HT1621B.h"



// 触发开始有效测量的温度值：温度值只有大于等于此温度，才算开始有效测量
#define START_TEMP   3400

// 测量稳定的判定阈值：一分钟之间的温度变化阈值
#define DELTA_TEMP    3    // 0.03摄氏度


// 缓冲区长度：60个数据
#define NBUF    60   // 最多存一分钟数据

// 从开机到开始有效温度测量之间的时间限
#define NBEG    60    // 开机后一分钟，还没有进入有效测量，就会触发关机

// 从开始有效测量到可以预测温度，所需的数据长度，应该小于NBUF
#define NPRE    51    // 开始测量后50秒开始预测

// 从开始有效测量到判断温度稳定，所需的最小时间，应该大于NBUF
#define NSTA    300   // 起码5分钟才开始判断是否稳定

// 从开始有效测量到关机的极限时间
#define NMAX    1800    // 半个小时一定会关机

// 从测温稳定后到关机的时间限
#define NAFT    180     // 稳定后3分钟关机，可以考虑每分钟提示一下

/*
// 用于预测5分钟的线性模型参数
#define A30      4.7330          // 30-20秒温度变化参数
#define A40     -12.1624         // 40-30秒温度变化参数
#define A50      14.5035         // 50-40秒温度变化参数
*/

// 用于预测10分钟的线性模型参数
#define A30      9.4545          // 30-20秒温度变化参数
#define A40     -23.6205         // 40-30秒温度变化参数
#define A50      23.0105         // 50-40秒温度变化参数


// 数据缓存区
static uint16 buf[NBUF] = {0};

// 当前数据保存的缓存区位置索引
static uint8 idx = 0;

// 保存应用层的任务ID，以便可以发送通知事件
static uint8 taskID;

// 开始有效测量后记录数据总数
static uint16 totalNum = 0;

// 测温稳定后记录数据数
static uint16 stableNum = 0;

// 体温计是否初始化，即开始温度值必须小于34度
static bool isInit = FALSE;

// 是否开始有效测温（判定条件是：温度值 >= START_TEMP）
static bool isStart = FALSE;

// 测温是否已经稳定（判定条件是：一分钟内温度变化值 <= DELTA_TEMP）
static bool isStable = FALSE;

// 1分钟的温度变化值
static int16 delta = 0;


// 触发事件操作
static void trigEvent(uint16 event);




// 初始化
extern void DP_Init(uint8 TaskID)
{
  taskID = TaskID;
  
  for(int i = 0; i < NBUF; i++) buf[i] = 0;
  
  idx = 0;
  
  totalNum = 0;
  
  stableNum = 0;
  
  isInit = FALSE;
  
  isStart = FALSE;
  
  isStable = FALSE;
}

// 处理温度数据
extern void DP_Process(uint16 data)
{
  // 如果还没有初始化
  if( !isInit )
  {
    // 可以初始化了
    if(data < START_TEMP)
      isInit = TRUE;
    
    return;
  }
  
  // 如果还没有效开始
  if(!isStart)
  {
    // 有效开始了
    if( isStart = (data >= START_TEMP) )
    {
      // 保存第一次有效测量数据
      buf[0] = data;
      idx = 1;
      // 开始计数
      totalNum = 1;
      return;
    }
    
    // 过了指定时间还没开始有效测量，对不起要关机省电
    if(++totalNum == NBEG)
    {
      // 通知转换到待机状态
      trigEvent(TH_SWITCH_MODE_EVT);
      return;
    }
    
    // 少于指定时间还没开始有效测量，就什么也不做，等待
    // 不可能长于指定时间
    return;
  }
  
  // 下面是已经开始有效测量后的操作
  
  // 先把数据存入
  buf[idx++] = data;
  if(idx >= NBUF) idx = 0;
  totalNum++;
  
  // 到了预测的时间
  if(totalNum == NPRE)
  {
    // 触发预测事件
    trigEvent(TH_DP_PRECAST_EVT);
    return;    
  }
  
  // 在到达稳定判定时间之前，直接返回
  if(totalNum < NSTA)
  {
    return;
  }

  // 下面是达到稳定判定时间
  
  // 已经达到最大极限时间，关机
  if(totalNum > NMAX)
  {
    trigEvent(TH_SWITCH_MODE_EVT);
    return;
  }
  
  // 还没有稳定
  if(!isStable)
  {
    //求当前温度与一分钟前的温度之差
    delta = data - buf[idx];
    isStable = ( (delta >= 0) && (delta <= DELTA_TEMP)  );
    if(isStable)  // 第一次稳定
    {
      stableNum = 1;
      // 触发测温稳定事件
      trigEvent(TH_DP_STABLE_EVT);
      return;
    }
  }
  else  // 测温已经稳定
  {
    // 已经稳定了指定时间了，要关机
    if(++stableNum == NAFT)
    {
      trigEvent(TH_SWITCH_MODE_EVT);
    }    
  } 
}


// 预测温度操作
extern uint16 getPrecastTemp()
{
  // 用线性模型预测温度值
  return A30*(buf[30]-buf[20])+A40*(buf[40]-buf[30])+A50*(buf[50]-buf[40])+buf[50];
}


// 测温稳定提示操作
extern void notifyTempStable()
{
  // 响三声提醒
  //osal_set_event( taskID, TH_TONE_ON_EVT);
  //osal_start_timerEx(taskID, TH_TONE_ON_EVT, 1000 );
  //osal_start_timerEx(taskID, TH_TONE_ON_EVT, 2000 );
  
  return;
}


// 触发事件操作
static void trigEvent(uint16 event)
{
  osal_set_event( taskID, event);
}
