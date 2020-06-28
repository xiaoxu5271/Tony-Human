
#ifndef _LED_H_
#define _LED_H_

#include "freertos/FreeRTOS.h"
TaskHandle_t Led_Task_Handle;

extern void Led_Init(void);

uint8_t Led_Status;

extern bool E2P_FLAG;
extern bool ETH_FLAG;
extern bool INT_FLAG;

extern bool Set_defaul_flag;
extern bool Net_sta_flag;
extern bool Cnof_net_flag;
extern bool No_ser_flag;

void Led_R_On(void);
void Led_G_On(void);
void Led_B_On(void);
void Led_C_On(void);
void Led_Off(void);
void Led_B_Off(void);

void Turn_ON_LED(void);
void Turn_Off_LED(void);
void Led_R_fade_Off(void);
void Led_R_fade_On(void);

#endif
