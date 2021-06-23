
#ifndef _HUMAN_H_
#define _HUMAN_H_

#include "freertos/task.h"

extern void Human_Init(void);
void Change_Fn_sen(uint32_t Fn_sen);

#define GPIO_HUMAN 27

int havehuman_count;
int nohuman_count;
extern bool HUM_FLAG;
extern uint64_t human_intr_num;
extern bool human_status;

enum
{
    GPIO_CB = 1,
    TIMER_CB,
    TIMER2_CB
};

#endif
