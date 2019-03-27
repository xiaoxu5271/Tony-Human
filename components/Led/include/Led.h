/*
LED驱动程序
创建日期：2018年10月29日
作者：孙浩

*/
#ifndef _LED_H_
#define _LED_H_


#include "freertos/FreeRTOS.h"


extern void Led_Init(void);

uint8_t Led_Status;

#define LED_STA_INIT            0x00
#define LED_STA_TOUCH           0x01//配网
#define LED_STA_WIFIERR         0x02
#define LED_STA_NOSER           0x03//无序列号
#define LED_STA_HAND            0x04//只配置开关或者手动模式
#define LED_STA_LOCAL           0x05//5配置wifi本地运行或4状态切本地计算状态
#define LED_STA_AUTO            0x06//配置wifi接收php运行，work_status=WORK_AUTO
#define LED_STA_PROTECT         0x07//除火灾外的保护状态
#define LED_STA_FIRE            0x08//火灾
#define LED_STA_SEND          0X09 //


#endif

