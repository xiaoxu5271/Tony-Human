
#ifndef _HUMAN_H_
#define _HUMAN_H_

extern void Human_Init(void);
extern void Humanapp(void);
extern void Human_Task(void *arg);

#define GPIO_HUMAN 27

int havehuman_count;
int nohuman_count;

int need_send;

#endif
