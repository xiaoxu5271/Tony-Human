/*
无线无源开关驱动程序

创建日期：2018年10月29日
作者：孙浩

Wallkey_Init(void);
初始化函数，主要为UART初始化和GPIO初始化
本例使用UART1，9600，8N1
UART1_TXD = (GPIO_NUM_5)
UART1_RXD = (GPIO_NUM_4)

void Wallkey_App(uint8_t* Key_Id,int8_t Switch);
WallKey应用函数，给入Key_Id(蓝牙发送过来的)和方向左0右1
在task中使用

*/
#include "freertos/FreeRTOS.h"
#ifndef _WALLKEY_H_
#define _WALLKEY_H_

//双控按键左上
#define KEY_DOU_LEFT_UP         0X01
//双控按键左下
#define KEY_DOU_LEFT_DOWN       0X02
//双控按键右上
#define KEY_DOU_RIGHT_UP        0X10
//双控按键右下
#define KEY_DOU_RIGHT_DOWN      0X20
//按键抬起
#define KEY_RELEASE             0X00

//单控按键上
#define KEY_SIN_UP              0X01
//单控按键下
#define KEY_SIN_DOWN            0X02

//#define WallKeyStop         0x00
#define WallKeyUpStart      0x01
#define WallKeyDownStart    0x02
#define WallKeyUpStop       0x03
#define WallKeyDownStop     0x04
#define WallKeyWait         0x05

extern void Wallkey_Init(void);
extern void Wallkey_App(uint8_t* Key_Id,int8_t Switch);
extern uint8_t WallKeyCtl_Status;//墙壁开关状态，上开始，下开始，停止

#endif

