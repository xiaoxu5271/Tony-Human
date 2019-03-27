/*
FIRE火灾接口驱动程序
创建日期：2018年11月26日
作者：孙浩

void Fire_Init(void);
初始化XF接口，主要为GPIO初始化
GPIO_LED=27



*/
#ifndef _FIRE_H_
#define _FIRE_H_

extern void Fire_Init(void);

#define GPIO_XF    27


#endif

