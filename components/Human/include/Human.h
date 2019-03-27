
#ifndef _HUMAN_H_
#define _HUMAN_H_

extern void Human_Init(void);
extern void Humanapp(void);

#define GPIO_HUMAN      27

 
extern  uint8_t  Last_Led_Status;
int havehuman_count;
int nohuman_count;

int need_send;

#endif

