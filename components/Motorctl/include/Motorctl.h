/*
百叶帘电机驱动程序
创建日期：2018年10月29日
作者：孙浩

Motor_Init(void);
初始化函数，主要初始化GPIO和电机转动计时用的定时器
MOTOR_CTL1 = 26  继电器1
MOTOR_CTL2 = 25  继电器2

以下的百叶帘电机控制函数，会在电机控制结束后返回，建议放在task中使用，以免占用系统时间降低效率,不能在定时器中使用

int Motor_SetAllDown(void);
百叶帘控制全部放下（清零）
对应目标高度-角度=100-80
返回值：
控制成功    MOTOROK     = 1
控制错误    MOTORERR    = -1
当前在动作时，返回错误
当前已经是全部放下，返回错误

int Motor_SetAllUp(void);
百叶帘控制全部升起
对应目标高度-角度=0-0
返回值：
控制成功    MOTOROK     = 1
控制错误    MOTORERR    = -1
当前在动作时，返回错误
当前已经是全部升起，返回错误

void Motor_Value_Query(int8_t* height,int8_t* angle);
百叶帘状态查询
参数：高度指针，角度指针，变量类型int8_t


int Motor_HandCtl_Angle(uint8_t change);
手动控制角度变化
参数：
角度增加ADD  1  
角度减少SUB  2
返回值：
控制成功    MOTOROK     = 1
控制错误    MOTORERR    = -1
当前在动作时，返回错误
当前角度为80，再控制增加，返回错误
当前角度为0，再控制减少，返回错误


int Motor_HandCtl_Height(uint8_t change);
手动控制高度变化
参数：
高度增加ADD  1  ——实际控制百叶帘降低10，停留位置为高-角：X-80
高度减少SUB  2  ——实际控制百叶帘升高10，停留位置为高-角：X-0
返回值：
控制成功    MOTOROK     = 1
控制错误    MOTORERR    = -1
当前在动作时，返回错误
当前高度为0，再控制减少高度，则返回错误
当前角度为100，再控制增加高度，则返回错误


int Motor_AutoCtl(int8_t auto_height,int8_t auto_angle);
自动控制函数
参数：
目标高度和角度
高度范围是100或0
角度范围是0-80

返回值
控制成功    MOTOROK     = 1
控制错误    MOTORERR    = -1
当前在动作时，返回错误
给定目标值不满足范围时，返回错误

void Motor_Ctl_App(void);
电机控制函数，用于墙壁开关控制和本地计算控制
在task中使用


extern void Motor_Up(void);
extern void Motor_Down(void);
extern void Motor_Stop(void);
以上三个函数仅为调试时使用，形成产品后，变为内部函数，不再对外调用

*/
#include "freertos/FreeRTOS.h"
#ifndef _MOTORCTL_H_
#define _MOTORCTL_H_

#define MOTOROK              1
#define MOTORERR            -1


#define ADD                 1
#define SUB                 2
#define STOP                3    
#define WAIT                4    

extern void Motor_Init(void);
extern int Motor_SetAllDown(void);
extern int Motor_SetAllUp(void);
extern void Motor_Value_Query(int8_t* height,int8_t* angle);
extern int Motor_HandCtl_Angle(uint8_t change);
extern int Motor_HandCtl_Height(uint8_t change);
extern int Motor_AutoCtl(int8_t auto_height,int8_t auto_angle);
extern int Motor_KeyCtl(uint8_t change);
extern void Motor_Ctl_App(void);

/*******测试调试用************************/
extern void Motor_Up(void);
extern void Motor_Down(void);
extern void Motor_Stop(void);



#endif

