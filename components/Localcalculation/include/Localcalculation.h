/*
百叶帘状态本地计算函数
创建日期：2018年11月01日
作者：孙浩

功能：给定相关参数后，计算当前百叶帘所要达到的状态，计算结果为目标高度和角度
参数：
年，月，日，时，分，经度，纬度，窗户朝向（+-180），
上班时间小时，上班时间分钟，午休时间开始小时，午休时间开始分钟，午休时间结束小时，午休时间结束分钟，下班时间小时，下班时间分钟
计算结果高度指针，计算结果角度指针

例子：
计算2018年3月2日 8：45状态，经度121.6，纬度38.9，窗户朝向-120，上班时间：8：00，午休开始时间11：30，午休结束时间13：10，下班时间18：00
Localcalculation(2017,3,2,8,45,121.6,38.9,-120,8,0,11,30,13,10,18,0,&Height,&Angle);  
*/
#ifndef _LOCALCALCULATION_H_
#define _LOCALCALCULATION_H_

void Localcalculation(int year,int month,int day,int hour,int minute,float lon,float lat,float orientation,
                      int T1_h,int T1_m,int T2_h,int T2_m,int T3_h,int T3_m,int T4_h,int T4_m,
                      int* Height,int* Angle);


#endif

