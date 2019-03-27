#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "Localcalculation.h"

#define PI  3.14159
//H叶片间距
#define H  80.0
//W叶片宽度
#define W  95.0
//𝜀最大入射角
#define E  39.0
//北方纬度定义
#define LAT_NORTH   38
//南部纬度定义
#define LAT_SOUTH   15

//北方冬季开始积日
#define NORTH_WINTER_START      293
//北方冬季结束积日
#define NORTH_WINTER_END        110
//中部冬季开始积日
#define CENTRAL_WINTER_START    293
//中部冬季结束积日
#define CENTRAL_WINTER_END      110
//南部冬季开始积日
#define SOUTH_WINTER_START      293
//南部冬季结束积日
#define SOUTH_WINTER_END        110


static const char *TAG = "Local_Calculation";

static float degrees(float r)//弧度转角度
{
    return 180/PI*r;
}

static float radians(float d)//角度转换弧度
{
    return PI/180*d;
}

static int cal_N(int y,int m,int d)//给定年月日，算积日
{
    int day_N = 0;
    switch(m-1)    //故意没有在case里加break
    {
        case 11:
            day_N += 30;
        case 10:
            day_N += 31;
        case 9:
            day_N += 30;
        case 8:
            day_N += 31;
        case 7:
            day_N += 31;
        case 6:
            day_N += 30;
        case 5:
            day_N += 31;
        case 4:
            day_N += 30;
        case 3:
            day_N += 31;
        case 2:
            if((y % 4 == 0 && y % 100 != 0) ||y%400==0) 
            {
                day_N += 29;
            }
            else
            {
                day_N += 28;
            }
         case 1:
            day_N += 31;
              
         default:
             break;
      }
      day_N+=d;
      return day_N;
 }

 //问题
 //三段纬度标准

void Localcalculation(int year,int month,int day,int hour,int minute,float lon,float lat,float orientation,
                      int T1_h,int T1_m,int T2_h,int T2_m,int T3_h,int T3_m,int T4_h,int T4_m,
                      int* Height,int* Angle)
{
    int N;//积日
    float N0;//N0标准积日
    float t;//t公转时间
    float Fsun;//𝜃日角
    //float Er;//Er日地距离
    float Ed;//Ed赤纬角
    float Et;//Et时差
    float Sd;//Sd地方太阳时
    float So;//So真太阳时
    float At;//𝜏时角
    float Ah;//h高度角
    float A;//A方位角
    float As;//As方位角加符号
    float Ab;//𝛽入射角
    float Aa0;//𝛼0翻转角
    float Aa1;//𝛼1日落修正
    float Aa2;//𝛼2无直射修正
    float Aa3=0;//𝛼3圆整
    float Aa4;//𝛼4零度角修正
    float Aa5;//𝛼5窗框修正
    float Aa6;//𝛼6日出日落修正
    float G1;//G1高度夜晚修正
    float G2;//G2高度无直射修正
    float Aa;//𝛼标准旋转角度
    int N1;//N1冬季开始积日
    int N2;//N2冬季结束积日
    int X;//人员使用信号
    int G;//G最终高度
    int AA;//最终角度
    N=cal_N(year,month,day);
    //ESP_LOGI(TAG, "N=%d",N);
    N0=79.6764+0.2422*(year-1985)-(int)((year-1985)/4);
    t=N-N0;
    //ESP_LOGI(TAG, "t=%f",t);
    Fsun=2*PI*t/365.2422;
    //Er=1.000423+0.032359*sin(Fsun)+0.000086*sin(2*Fsun)-0.008349*cos(Fsun)+0.000115*cos(2*Fsun);
    Ed=0.3723+23.2567*sin(Fsun)+0.1149*sin(2*Fsun)-0.1712*sin(3*Fsun)-0.758*cos(Fsun)+0.3656*cos(2*Fsun)+0.0201*cos(3*Fsun);
    Et=0.0028-1.9857*sin(Fsun)+9.9059*sin(2*Fsun)-7.0924*cos(Fsun)-0.6882*cos(2*Fsun);
    Sd=hour+(minute-4*(120-lon))/60;
    So=Sd+Et/60;
    At=(So-12)*15;
    Ah=degrees(asin((sin(radians(Ed))*sin(radians(lat))+cos(radians(Ed))*cos(radians(lat))*cos(radians(At)))));
    //ESP_LOGI(TAG, "Ah=%f",Ah);
    A=degrees(acos((sin(radians(Ah))*sin(radians(lat))-sin(radians(Ed)))/(cos(radians(Ah))*cos(radians(lat)))));
    //ESP_LOGI(TAG, "A=%f",A);

    if(Sd<12)
    {
        As=-A;
    }
    else
    {
        As=A;
    }

    if(fabs(As-orientation)==90)
    {
        Ab=90;
    }
    else
    {
       Ab=fabs(degrees(atan(tan(radians(Ah))/cos(radians(As-orientation)))));
    }
    //ESP_LOGI(TAG, "Ab=%f",Ab);

    Aa0=(acos((H/(2*W))*sin(2*(Ab/180)*PI)+sqrt((1-((H*H)/(W*W)))*(pow(cos((Ab/180)*PI),2))+(((H/(2*W))*(H/(2*W)))*(sin(2*(Ab/180)*PI)*sin(2*(Ab/180)*PI)))))*180/PI);
    //ESP_LOGI(TAG, "Aa0=%f",Aa0);

    
    if(Ah>0)
    {
        Aa1=Aa0;
    }
    else
    {
        Aa1=0;
    }
    //ESP_LOGI(TAG, "Aa1=%f",Aa1);

    if((orientation>=-90)&&(orientation<0))
    {
        if(((As>=(orientation+90)) && (As<=180)) || ((As<=(orientation-90))&& (As>=-180)))
        {
            Aa2=0;
        }
        else
        {
            Aa2=Aa1;
        }
    }
    else
    {
        if((orientation>-180) &&(orientation<-90))
        {
            if((As>=(orientation+90)) && (As<=(orientation+270)))
            {
                Aa2=0;
            }
            else
            {
                Aa2=Aa1;
            }
        }
        else
        {
            if((orientation<90) &&( orientation>=0))
            {
                if(((As>=-180) && (As<(orientation-90))) || ((As>=(90+orientation)) && (As<=180)))
                {
                    Aa2=0;
                }
                else
                {
                    Aa2=Aa1;
                }			
            }
            else
            {
                if((orientation>=90) && (orientation<=180))
                {
                    if((As>=(orientation-270)) && (As<=(orientation-90)))
                    {
                        Aa2=0;
                    }
                    else
                    {
                        Aa2=Aa1;
                    }
                }
                else
                {
                    Aa2=Aa1;
                }	   
            }
        }	
    }
    //ESP_LOGI(TAG, "Aa2=%f",Aa2);
    
    if(Aa2==0)
    {
        Aa3=0;
    }
    else if((Aa2>0)&&(Aa2<=10))
    {
        Aa3=10;
    }
    else if((Aa2>10)&&(Aa2<=20))
    {
        Aa3=20;
    }
    else if((Aa2>20)&&(Aa2<=30))
    {
        Aa3=30;
    }
    else if((Aa2>30)&&(Aa2<=40))
    {
        Aa3=40;
    }
    else if((Aa2>40)&&(Aa2<=50))
    {
        Aa3=50;
    }
    else if((Aa2>50)&&(Aa2<=60))
    {
        Aa3=60;
    }
    else if((Aa2>60)&&(Aa2<=70))
    {
        Aa3=70;
    }
    else if((Aa2>70)&&(Aa2<=80))
    {
        Aa3=80;
    }
    else if(Aa2>80)
    {
        Aa3=80;
    }    
    //ESP_LOGI(TAG, "Aa3=%f",Aa3);


    if(Ab>E)
    {
        if(Aa2==0)
        {
            Aa4=Aa3;
        }
        else
        {
            Aa4=1;
        }
    }
    else
    {
        Aa4=Aa3;
    }
    //ESP_LOGI(TAG, "Aa4=%f",Aa4);

    if(fabs(As-orientation-90)<5)
    {
        if(Aa4==0)
        {
            Aa5=0;
        }
        else
        {
            if(Aa4==1)
            {
                Aa5=Aa4;
            }
            else
            {
                Aa5=2;
            }
        }
    }
    else
    {
        Aa5=Aa4;
    }
    //ESP_LOGI(TAG, "Aa5=%f",Aa5);

    if(fabs(Ah)<5)
    {
        if(Aa5==0)
        {
            Aa6=0;
        }
        else
        {
            Aa6=3;
        }
    }
    else
    {
        Aa6=Aa5;
    }
    //ESP_LOGI(TAG, "Aa6=%f",Aa6);

    if((Aa6==0)||(Aa6==2)||(Aa6==3))
    {
        G1=0;
    }
    else
    {
        G1=100;
    }
    //ESP_LOGI(TAG, "G1=%f",G1);

    /*********计算G2***************************************************************/
    if(Ah>5)
    {
        if((orientation>=-90) && (orientation<0))
        {
            if(((As>=orientation+90) && (As<=180)) || ((As>=-180) && (As<=orientation-90)))
            {
                G2=0;
            }
            else
            {
                G2=G1;
            }
        }
        else
        {
            if((orientation>-180) && (orientation<-90))
            {
                if((As>=orientation+90) && (As<=orientation+270))
                {
                    G2=0;
                }
                else
                {
                    G2=G1;
                }			
            }
            else
            {
                if((orientation<90) && (orientation>=0))
                {
                    if(((As>=-180) && (As<=orientation-90)) || ((As>=90+orientation) && (As<=180)))
                    {
                        G2=0;
                    }
                    else
                    {
                        G2=G1;
                    }				
                }
                else
                {
                    if((orientation>=90) && (orientation<=180))
                    {
                        if((As>=orientation-270) && (As<=orientation-90))
                        {
                            G2=0;
                        }
                        else
                        {
                            G2=G1;					
                        }
                    }
                    else
                    {
                        G2=G1;
                    }
                }
            }
        }
    }
    else
    {
        G2=G1;
    }
    //ESP_LOGI(TAG, "G2=%f",G2);

    if((Aa6==1)||(Aa6==2)||(Aa6==3))
    {
        Aa=0;
    }
    else
    {
        Aa=Aa6;
    }
    //ESP_LOGI(TAG, "Aa=%f",Aa);

    /************************人员过程**********************************/
    if(lat>LAT_NORTH)//北方
    {
        N1=NORTH_WINTER_START;
        N2=NORTH_WINTER_END;
    }
    else if((lat>=LAT_SOUTH)&&(lat<LAT_NORTH))//中部
    {
        N1=CENTRAL_WINTER_START;
        N2=CENTRAL_WINTER_END;
    }
    else if(lat<LAT_SOUTH)//南部
    {
        N1=SOUTH_WINTER_START;
        N2=SOUTH_WINTER_END;
    }

    if(((hour*60+minute)>=(T2_h*60+T2_m))&&((hour*60+minute)<(T3_h*60+T3_m)))
    {
        X=1;
    }
    else
    {
        if(((hour*60+minute)>=(T1_h*60+T1_m))&&((hour*60+minute)<(T4_h*60+T4_m)))
        {
            X=2;
        }
        else
        {
            if((Ah>=5)&&((hour*60+minute)<(T2_h*60+T2_m)))
            {
                if(((N>=1)&&(N<=N2))||((N>=N1)&&(N<=366)))
                {
                    X=0;
                }
                else
                {
                    X=1;
                }
            }
            else
            {
                if(((N>=1)&&(N<=N2))||((N>=N1)&&(N<=366)))
                {
                    X=1;
                }
                else
                {
                    X=0;
                }
            }
        }
    }
    //ESP_LOGI(TAG, "X=%d",X);

    if(X==2)
    {
        G=G2;
    }
    else
    {
        if(X==1)
        {
            G=100;
        }
        else
        {
            G=0;
        }
    }
    ESP_LOGI(TAG, "G=%d",G);

    if(X==2)
    {
        AA=Aa;
    }
    else
    {
        if(X==1)
        {
            AA=80;
        }
        else
        {
            AA=0;
        }
    }
    ESP_LOGI(TAG, "AA=%d",AA);

    *Height=G;
    *Angle=AA;

}