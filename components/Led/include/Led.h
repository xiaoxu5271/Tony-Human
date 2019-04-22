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

#define LED_STA_INIT 0x00 //初始化
#define LED_STA_AP 0x01   //配网
#define LED_STA_WIFIERR 0x02
#define LED_STA_NOSER 0x03 //无序列号
#define LED_STA_WORK 0x08  //正常工作
#define LED_STA_SEND 0X09  //

void Led_R_On(void);
void Led_G_On(void);
void Led_B_On(void);
void Led_Y_On(void);
void Led_C_On(void);
void Led_Off(void);
void Led_B_Off(void);

#endif
