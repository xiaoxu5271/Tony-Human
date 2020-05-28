
#ifndef _HUMAN_H_
#define _HUMAN_H_

#include "freertos/task.h"

extern void Human_Init(void);
extern void Humanapp(void);
extern void Human_Task(void *arg);

#define GPIO_HUMAN 27

TaskHandle_t Human_Handle;
int havehuman_count;
int nohuman_count;
extern uint8_t human_chack;
extern uint64_t human_intr_num;
extern bool human_status;

#endif
