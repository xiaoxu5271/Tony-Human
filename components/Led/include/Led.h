
#ifndef _LED_H_
#define _LED_H_

#include "freertos/FreeRTOS.h"

#if 1
#define GPIO_R_NUM GPIO_NUM_17
#define GPIO_G_NUM GPIO_NUM_22
#define GPIO_B_NUM GPIO_NUM_21

#define GPIO_LED_R GPIO_ID_PIN(GPIO_R_NUM)
#define GPIO_LED_B GPIO_ID_PIN(GPIO_B_NUM)
#define GPIO_LED_G GPIO_ID_PIN(GPIO_G_NUM)

#define LEDC_TEST_FADE_TIME 1500

#else

#define GPIO_LED_B 21
#define GPIO_LED_G 22
#define GPIO_LED_R 23

#endif

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
