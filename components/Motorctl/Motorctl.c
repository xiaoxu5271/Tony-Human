#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "Motorctl.h"
#include "Wallkey.h"
#include "Json_parse.h"
#include "Http.h"

#define MOTOR_CTL1   26
#define MOTOR_CTL2   25

//#define ALLDOWNTIME         3000
#define ALLDOWNTIME         50*1000
#define ALLUPTIME           50*1000

#define ANGLE_N0TO0         162
#define ANGLE_0TO10         131
#define ANGLE_10TO20        120
#define ANGLE_20TO30        109
#define ANGLE_30TO40        105
#define ANGLE_40TO50        100
#define ANGLE_50TO60        100
#define ANGLE_60TO70        100
#define ANGLE_70TO80        100


#define ANGLE_0TON0         130
#define ANGLE_10TO0         123
#define ANGLE_20TO10        120
#define ANGLE_30TO20        120
#define ANGLE_40TO30        110
#define ANGLE_50TO40        100
#define ANGLE_60TO50        82
#define ANGLE_70TO60        76
#define ANGLE_80TO70        160
#define ANGLE_80TO60        240
#define ANGLE_80TO50        328
#define ANGLE_80TO40        410
#define ANGLE_80TO30        515
#define ANGLE_80TO20        620
#define ANGLE_80TO10        740
#define ANGLE_80TO0         860


//下降
#define HEIGHT_100TO90     3650
#define HEIGHT_90TO80      3796
#define HEIGHT_80TO70      3934
#define HEIGHT_70TO60      4089
#define HEIGHT_60TO50      4263
#define HEIGHT_50TO40      4462
#define HEIGHT_40TO30      4691
#define HEIGHT_30TO20      4960
#define HEIGHT_20TO10      5281
#define HEIGHT_10TO0       5536

//上升
#define HEIGHT_0TO10       5505
#define HEIGHT_10TO20      5238
#define HEIGHT_20TO30      4973
#define HEIGHT_30TO40      4745
#define HEIGHT_40TO50      4545
#define HEIGHT_50TO60      4369
#define HEIGHT_60TO70      4212
#define HEIGHT_70TO80      4070
#define HEIGHT_80TO90      3942
#define HEIGHT_90TO100     3866


#define MOTORSTATUS_RUN      0X01
#define MOTORSTATUS_STOP     0X02

#define KEYCTLANGLETIME     1000

struct Motor_Value_Struct
{
    int8_t height;
    int8_t angle;
};


struct Motor_Value_Struct Motor_Value;

uint8_t Motor_Status=MOTORSTATUS_STOP;

void timer_once_cb(void *arg);//函数声明
esp_timer_handle_t timer_once_handle = 0;//定义单次定时器句柄

//定义一个单次运行的定时器结构体
esp_timer_create_args_t timer_once_arg = { .callback = &timer_once_cb, //设置回调函数
        .arg = NULL, //不携带参数
        .name = "OnceTimer" //定时器名字
        };


void timer_once_cb(void *arg) 
{
    Motor_Status=MOTORSTATUS_STOP;
    //int64_t tick = esp_timer_get_time();
    //printf("方法回调名字: %s , 距离定时器开启时间间隔 = %lld \r\n", __func__, tick);
}

void Motor_Up(void)
{
    gpio_set_level(MOTOR_CTL1, 1);
    gpio_set_level(MOTOR_CTL2, 1);//上升

}


void Motor_Down(void)
{
    gpio_set_level(MOTOR_CTL1, 0);
    gpio_set_level(MOTOR_CTL2, 1);//下降    
    
}


void Motor_Stop(void)
{
    gpio_set_level(MOTOR_CTL1, 0);
    gpio_set_level(MOTOR_CTL2, 0);//停止   
}

static void Motor_Height_100to90(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_100TO90 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=90;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_90to80(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_90TO80 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=80;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_80to70(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_80TO70 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=70;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_70to60(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_70TO60 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=60;
    printf("height=%d\n",Motor_Value.height);
}


static void Motor_Height_60to50(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_60TO50 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=50;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_50to40(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_50TO40 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=40;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_40to30(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_40TO30 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=30;
    printf("height=%d\n",Motor_Value.height);
}


static void Motor_Height_30to20(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_30TO20 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=20;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_20to10(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_20TO10 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=10;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_10to0(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, HEIGHT_10TO0 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=0;
    Motor_Value.angle=0;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_0to10(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_0TO10 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=10;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_10to20(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_10TO20 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=20;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_20to30(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_20TO30 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=30;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_30to40(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_30TO40 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=40;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_40to50(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_40TO50 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=50;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_50to60(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_50TO60 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=60;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_60to70(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_60TO70 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=70;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_70to80(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_70TO80 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=80;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Height_80to90(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_80TO90 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=90;
    printf("height=%d\n",Motor_Value.height);
}


static void Motor_Height_90to100(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, HEIGHT_90TO100 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.height=100;
    printf("height=%d\n",Motor_Value.height);
}

static void Motor_Angle_80to60(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO60 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=60;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_80to50(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO50 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=50;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_80to40(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO40 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=40;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_80to30(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO30 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=30;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_80to20(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO20 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=20;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_80to10(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO10 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=10;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_80to0(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO0 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=0;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_80to70(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_80TO70 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=70;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_70to60(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_70TO60 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=60;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_60to50(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_60TO50 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=50;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_50to40(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_50TO40 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=40;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_40to30(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_40TO30 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=30;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_30to20(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_30TO20 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=20;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_20to10(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_20TO10 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=10;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_10to0(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_10TO0 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=0;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_0toN0(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Up();
    esp_timer_start_once(timer_once_handle, ANGLE_0TON0 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=-10;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_N0to0(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_N0TO0 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=0;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_0to10(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_0TO10 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=10;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_10to20(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_10TO20 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=20;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_20to30(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_20TO30 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=30;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_30to40(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_30TO40 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=40;
    printf("angle=%d\n",Motor_Value.angle);
}

static void Motor_Angle_40to50(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_40TO50 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=50;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_50to60(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_50TO60 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=60;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_60to70(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_60TO70 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=70;
    printf("angle=%d\n",Motor_Value.angle);
}


static void Motor_Angle_70to80(void)
{
    Motor_Status=MOTORSTATUS_RUN;
    Motor_Down();
    esp_timer_start_once(timer_once_handle, ANGLE_70TO80 * 1000); //启动单次定时器
    while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
    Motor_Stop();       
    Motor_Value.angle=80;
    printf("angle=%d\n",Motor_Value.angle);
}



void Motor_Init(void)
{
    gpio_config_t io_conf;

    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO16
    io_conf.pin_bit_mask = (1<<MOTOR_CTL1);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1<<MOTOR_CTL2);
    gpio_config(&io_conf);

    Motor_Value.height=-1;
    Motor_Value.angle=-1;


    esp_err_t err  = esp_timer_create(&timer_once_arg, &timer_once_handle);
    if(err != ESP_OK)
    {
        printf("create once timer errcode:%d\n", err);
    }
}


int Motor_SetAllDown(void)
{
    if(Motor_Status!=MOTORSTATUS_RUN)
    {
        printf("set down start,height=%d,angle=%d\n",Motor_Value.height,Motor_Value.angle);
        Motor_Value.height=100;
        Motor_Value.angle=80;
        Motor_Status=MOTORSTATUS_RUN;
        Motor_Down();
        esp_timer_start_once(timer_once_handle, ALLDOWNTIME * 1000); //启动单次定时器
        while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
        Motor_Stop();       
        Motor_Value.height=100;
        Motor_Value.angle=80;
        printf("set down over,height=%d,angle=%d\n",Motor_Value.height,Motor_Value.angle);
        return MOTOROK;
    }
    else
    {
        return MOTORERR;
    }
}


int Motor_SetAllUp(void)
{
    if(Motor_Status!=MOTORSTATUS_RUN)
    {
        printf("set up start,height=%d,angle=%d\n",Motor_Value.height,Motor_Value.angle);
        Motor_Value.height=0;
        Motor_Value.angle=0;
        Motor_Status=MOTORSTATUS_RUN;
        Motor_Up();
        esp_timer_start_once(timer_once_handle, ALLUPTIME * 1000); //启动单次定
        while(Motor_Status==MOTORSTATUS_RUN){vTaskDelay(10 / portTICK_RATE_MS);}
        Motor_Stop();       
        Motor_Value.height=0;
        Motor_Value.angle=0;
        printf("set up over,height=%d,angle=%d\n",Motor_Value.height,Motor_Value.angle);
        return MOTOROK;
    }
    else
    {
        return MOTORERR;
    }
}

void Motor_Value_Query(int8_t* height,int8_t* angle)
{
    *height=Motor_Value.height;
    *angle=Motor_Value.angle;
}


int Motor_HandCtl_Angle(uint8_t change)
{
    if(change==ADD)
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            switch(Motor_Value.angle)
            {
                case 80:
                    return MOTORERR;
                    break;
                case 70:
                    Motor_Angle_70to80();
                    break;
                case 60:
                    Motor_Angle_60to70();
                    break;
                case 50:
                    Motor_Angle_50to60();
                    break;
                case 40:
                    Motor_Angle_40to50();
                    break;
                case 30:
                    Motor_Angle_30to40();
                    break;
                case 20:
                    Motor_Angle_20to30();
                    break;
                case 10:
                    Motor_Angle_10to20();
                    break;
                case 0:
                    Motor_Angle_0to10();
                    break;
                case -10:
                    Motor_Angle_N0to0();
                default:
                    return MOTORERR;
                    break;

            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }

    }
    else if(change==SUB)
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            switch(Motor_Value.angle)
            {
                case 0:
                    return MOTORERR;
                    break;
                case 10:
                    Motor_Angle_10to0();
                    break;
                case 20:
                    Motor_Angle_20to10();
                    break;
                case 30:
                    Motor_Angle_30to20();
                    break;
                case 40:
                    Motor_Angle_40to30();
                    break;
                case 50:
                    Motor_Angle_50to40();
                    break;
                case 60:
                    Motor_Angle_60to50();
                    break;
                case 70:
                    Motor_Angle_70to60();
                    break;
                case 80:
                    Motor_Angle_80to70();
                    break;
                default:
                    return MOTORERR;
                    break;

            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }
    }
    else
    {
        return MOTORERR;
    }
}



int Motor_HandCtl_Height(uint8_t change)
{
    if(change==SUB)//上升
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            switch(Motor_Value.angle)//先将角度清到-10度
            {
                case 80:
                    Motor_Angle_80to70();
                    Motor_Angle_70to60();
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 70:
                    Motor_Angle_70to60();
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 60:
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 50:
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 40:
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 30:
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 20:
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 10:
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 0:
                    Motor_Angle_0toN0();
                    break;
                case -10:
                    return MOTORERR;
                    break;
                default:
                    break;
            }
            switch(Motor_Value.height)//再调整高度
            {
                case 100:
                    Motor_Height_100to90();
                    Motor_Angle_N0to0();    //角度变为0
                    break;
                case 90:
                    Motor_Height_90to80();
                    Motor_Angle_N0to0();  
                    break;
                case 80:
                    Motor_Height_80to70();
                    Motor_Angle_N0to0();  
                    break;
                case 70:
                    Motor_Height_70to60();
                    Motor_Angle_N0to0();  
                    break;
                case 60:
                    Motor_Height_60to50();
                    Motor_Angle_N0to0();  
                    break;
                case 50:
                    Motor_Height_50to40();
                    Motor_Angle_N0to0();  
                    break;
                case 40:
                    Motor_Height_40to30();
                    Motor_Angle_N0to0();  
                    break;
                case 30:
                    Motor_Height_30to20();
                    Motor_Angle_N0to0();  
                    break;
                case 20:
                    Motor_Height_20to10();
                    Motor_Angle_N0to0();  
                    break;
                case 10:
                    Motor_Height_10to0();
                    break;
                case 0:
                    return MOTORERR;
                    break;
                default:
                    return MOTORERR;
                    break;
            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }

    }
    else if(change==ADD)//下降
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            if(Motor_Value.height!=0)//高度为0时，不转动角度，直接下降
            {
                switch(Motor_Value.angle)//先将角度变为80
                {
                    case -10:
                        Motor_Angle_N0to0();               
                        Motor_Angle_0to10();  
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80();                       
                    case 0:
                        Motor_Angle_0to10();  
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 10:
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 20:
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 30:
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 40:
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 50:
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 60:
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 70:
                        Motor_Angle_70to80(); 
                        break;
                    case 80:
                        break;
                    default:
                        break;
                }
            }
            switch(Motor_Value.height)//再调整高度
            {
                case 0:
                    Motor_Height_0to10(); 
                    Motor_Value.angle=80;
                    break;
                case 10:
                    Motor_Height_10to20(); 
                    break;
                case 20:
                    Motor_Height_20to30(); 
                    break;
                case 30:
                    Motor_Height_30to40(); 
                    break;
                case 40:
                    Motor_Height_40to50(); 
                    break;
                case 50:
                    Motor_Height_50to60(); 
                    break;
                case 60:
                    Motor_Height_60to70(); 
                    break;
                case 70:
                    Motor_Height_70to80(); 
                    break;
                case 80:
                    Motor_Height_80to90(); 
                    break;
                case 90:
                    Motor_Height_90to100(); 
                    break;
                case 100:
                    return MOTORERR;
                    break;
                default:
                    return MOTORERR;
                    break;

            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }
    }
    else
    {
        return MOTORERR;
    }
}




static int Motor_KeyCtl_Height(uint8_t change)
{
    if(change==SUB)//上升
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            switch(Motor_Value.angle)//先将角度清到-10度
            {
                case 80:
                    Motor_Angle_80to70();
                    Motor_Angle_70to60();
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 70:
                    Motor_Angle_70to60();
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 60:
                    Motor_Angle_60to50();
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 50:
                    Motor_Angle_50to40();
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 40:
                    Motor_Angle_40to30();
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 30:
                    Motor_Angle_30to20();
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 20:
                    Motor_Angle_20to10();
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 10:
                    Motor_Angle_10to0();
                    Motor_Angle_0toN0();
                    break;
                case 0:
                    Motor_Angle_0toN0();
                    break;
                case -10:
                    //return MOTORERR;
                    break;
                default:
                    break;
            }
            switch(Motor_Value.height)//再调整高度
            {
                case 100:
                    Motor_Height_100to90();
                    break;
                case 90:
                    Motor_Height_90to80();
                    break;
                case 80:
                    Motor_Height_80to70(); 
                    break;
                case 70:
                    Motor_Height_70to60();
                    break;
                case 60:
                    Motor_Height_60to50(); 
                    break;
                case 50:
                    Motor_Height_50to40(); 
                    break;
                case 40:
                    Motor_Height_40to30(); 
                    break;
                case 30:
                    Motor_Height_30to20(); 
                    break;
                case 20:
                    Motor_Height_20to10(); 
                    break;
                case 10:
                    Motor_Height_10to0();
                    break;
                case 0:
                    return MOTORERR;
                    break;
                default:
                    return MOTORERR;
                    break;
            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }

    }
    else if(change==ADD)//下降
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            if(Motor_Value.height!=0)//高度为0时，不转动角度，直接下降
            {
                switch(Motor_Value.angle)//先将角度变为80
                {
                    case -10:
                        Motor_Angle_N0to0();               
                        Motor_Angle_0to10();  
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80();                       
                    case 0:
                        Motor_Angle_0to10();  
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 10:
                        Motor_Angle_10to20();  
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 20:
                        Motor_Angle_20to30();  
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 30:
                        Motor_Angle_30to40();  
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 40:
                        Motor_Angle_40to50();  
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 50:
                        Motor_Angle_50to60();  
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 60:
                        Motor_Angle_60to70();
                        Motor_Angle_70to80(); 
                        break;
                    case 70:
                        Motor_Angle_70to80(); 
                        break;
                    case 80:
                        break;
                    default:
                        break;
                }
            }
            switch(Motor_Value.height)//再调整高度
            {
                case 0:
                    Motor_Height_0to10(); 
                    Motor_Value.angle=80;
                    break;
                case 10:
                    Motor_Height_10to20(); 
                    break;
                case 20:
                    Motor_Height_20to30(); 
                    break;
                case 30:
                    Motor_Height_30to40(); 
                    break;
                case 40:
                    Motor_Height_40to50(); 
                    break;
                case 50:
                    Motor_Height_50to60(); 
                    break;
                case 60:
                    Motor_Height_60to70(); 
                    break;
                case 70:
                    Motor_Height_70to80(); 
                    break;
                case 80:
                    Motor_Height_80to90(); 
                    break;
                case 90:
                    Motor_Height_90to100(); 
                    break;
                case 100:
                    return MOTORERR;
                    break;
                default:
                    return MOTORERR;
                    break;

            }
            return MOTOROK;
        }
        else
        {
            return MOTORERR;
        }
    }
    else
    {
        return MOTORERR;
    }
}



int Motor_AutoCtl(int8_t auto_height,int8_t auto_angle)//自动控制函数，给高角值
{
    if((auto_height==0)&&(auto_angle==0))//目标为0，0  则判断当前状态不是0，x的情况情况下，执行全收指令
    {
        
        if(Motor_Value.height!=0)
        {
            return Motor_SetAllUp();
        }
        else
        {
            
            return MOTORERR;
        }
    }
    else if((auto_height==100)&&(Motor_Value.height!=100))//目标为100，x  当前高度不是100，执行全放
    {
        Motor_SetAllDown();
    }
    
    if((auto_height==100)&&(Motor_Value.height==100))//目标为100，x  当前是100，x，则转角到位
    {       
        if(Motor_Value.angle==80)//在100，80时给角度值，则直接到位
        {
            switch(auto_angle)//目标角度
            {
                case 80:
                    return MOTOROK;
                    break;
                case 70:
                    Motor_Angle_80to70();
                    break;
                case 60:
                    Motor_Angle_80to60();
                    break;
                case 50:
                    Motor_Angle_80to50();
                    break;
                case 40:
                    Motor_Angle_80to40();
                    break;
                case 30:
                    Motor_Angle_80to30();
                    break;
                case 20:
                    Motor_Angle_80to20();
                    break;
                case 10:
                    Motor_Angle_80to10();
                    break;
                case 0:
                    Motor_Angle_80to0();
                    break;
                default:
                    return MOTORERR;
                    break;
            }
            return MOTOROK;
        }
        else//在100，非80时给目标角度值
        {
            if((auto_angle>=(-10))&&(auto_angle<=80))
            {
                while(auto_angle!=Motor_Value.angle)
                {
                    if(auto_angle>Motor_Value.angle)
                    {
                        Motor_HandCtl_Angle(ADD);
                        vTaskDelay(50 / portTICK_RATE_MS);
                    }    
                    else if(auto_angle<Motor_Value.angle)
                    {
                        Motor_HandCtl_Angle(SUB);
                        vTaskDelay(50 / portTICK_RATE_MS);
                    }
                }
                return MOTOROK;
            }
            else
            {
                return MOTORERR;
            }
        }
    }
    else
    {
        return MOTORERR;
    }

}



int Motor_KeyCtl(uint8_t change)
{
    int ret;
    if(change==ADD)//上升
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            if((Motor_Value.angle==0)&&(Motor_Value.height==0))//当前是00时，直接返回error
            {
                return MOTORERR;
            }
            else if(Motor_Value.angle==10)//当角度=10时，执行角度-10后，返回，停止
            {
                Motor_HandCtl_Angle(SUB); 
                return MOTORERR;
            }
            else if((Motor_Value.angle!=0)&&(Motor_Value.angle!=-10))  //当前角度在非0状态时，角度先变为0
            {
                ret=Motor_HandCtl_Angle(SUB); 
                if(ret==MOTOROK)
                {
                    vTaskDelay(KEYCTLANGLETIME / portTICK_RATE_MS);//每个角度控制改变间隔
                    return ret;
                }
                else
                {
                    return ret;
                }
            }
            else//当前角度=0状态时，高度直接减少
            {
                ret=Motor_KeyCtl_Height(SUB);
                if(ret==MOTOROK)
                {
                    return ret; 
                }
                else
                {
                    return ret;
                }
            }
        }
        else
        {
            return MOTORERR;
        }
    }

    else if(change==SUB)//下降
    {
        if(Motor_Status!=MOTORSTATUS_RUN)
        {
            if((Motor_Value.angle==0)&&(Motor_Value.height==0))  //当前为0，0状态，则先执行高度+1
            {
                return Motor_KeyCtl_Height(ADD);
            }
            else if(Motor_Value.angle==70)//当角度为70时，执行angleadd后，返回停止
            {
                Motor_HandCtl_Angle(ADD);
                return MOTORERR;
            }
            else
            {
                if(Motor_Value.angle!=80)  //当前角度在非80状态时，角度++,达到80
                {
                    ret= Motor_HandCtl_Angle(ADD);
                    if(ret==MOTOROK)
                    {
                        vTaskDelay(KEYCTLANGLETIME / portTICK_RATE_MS);
                        return ret;
                    }
                    else
                    {
                        return ret;
                    }

                }
                else//当前角度在80状态时，高度++
                {
                    return Motor_KeyCtl_Height(ADD);
                }
            }
        }
        else
        {
            return MOTORERR;
        }
    }
    else if(change==STOP)
    {
        change=WAIT;
        if(Motor_Value.angle==-10)  //当前正在上升，停止后角度为-10，则执行-10～10
        {
             Motor_Angle_N0to0();//正常上升结束后执行一次-10～10
           
        }
        
    }

    return MOTORERR;

}


void Motor_Ctl_App(void)
{
    switch(WallKeyCtl_Status)
    {
        case WallKeyUpStart:

            work_status=WORK_WALLKEY;
            mqtt_json_s.mqtt_last=MAX_WALLKEY_TIME;
            
            if(Motor_KeyCtl(ADD)==MOTORERR)
            {
                WallKeyCtl_Status=WallKeyUpStop;
                printf("WallKeyCtl_Status=stop\n");
            }


            break;
        case WallKeyDownStart:

            work_status=WORK_WALLKEY;
            mqtt_json_s.mqtt_last=MAX_WALLKEY_TIME;
            
            if(Motor_KeyCtl(SUB)==MOTORERR)
            {
                WallKeyCtl_Status=WallKeyDownStop;
                printf("WallKeyCtl_Status=stop\n");
            }


            break;
        case WallKeyUpStop: //上升停止，高度停止时执行-10～10，角度停止时不执行                  
            Motor_KeyCtl(STOP);
            WallKeyCtl_Status=WallKeyWait;
            strcpy(mqtt_json_s.mqtt_mode, "0");//手动模式
            if(WifiStatus==WIFISTATUS_CONNET)
            {
                http_send_mes(POST_NORMAL); 
            }
            break;
        case WallKeyDownStop://下降停止
            WallKeyCtl_Status=WallKeyWait;
            strcpy(mqtt_json_s.mqtt_mode, "0");//手动模式
            if(WifiStatus==WIFISTATUS_CONNET)
            {
                http_send_mes(POST_NORMAL); 
            }
            break;

        default:
            break;
    }

}



