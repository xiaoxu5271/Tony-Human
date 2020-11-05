#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "Human.h"
#include "Json_parse.h"
#include "Smartconfig.h"

#include "Led.h"

bool E2P_FLAG = true;
bool ETH_FLAG = true;
bool INT_FLAG = true;

bool Set_defaul_flag = false;
bool Net_sta_flag = false;
bool Cnof_net_flag = false;
bool No_ser_flag = false;

TaskHandle_t Led_Task_Handle = NULL;
uint8_t Last_Led_Status;
ledc_channel_config_t ledc_channel = {.channel = LEDC_CHANNEL_0,
                                      .duty = 1023, //初始占空比
                                      .gpio_num = GPIO_LED_G,
                                      .speed_mode = LEDC_HIGH_SPEED_MODE,
                                      .hpoint = 0,
                                      .timer_sel = LEDC_TIMER_0};

/*******************************************
黄灯闪烁：硬件错误
蓝灯闪烁：恢复出厂
红蓝交替闪烁：配置网络

网络错误：红灯常亮
绿灯呼吸：有人移动 （网络正常）
*******************************************/
static void Led_Task(void *arg)
{
    while (1)
    {
        //硬件错误
        if ((E2P_FLAG == false) || (ETH_FLAG == false))
        {
            Led_Off();
            vTaskDelay(500 / portTICK_RATE_MS);
            Led_R_On();
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        //恢复出厂
        else if (Set_defaul_flag == true)
        {
            Led_B_On();
            Led_R_On();
            Led_G_On();
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        //配网
        else if (Cnof_net_flag == true)
        {
            Led_Off();
            Led_B_On();
            vTaskDelay(500 / portTICK_RATE_MS);
            Led_Off();
            Led_R_On();
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        //初始化
        else if (INT_FLAG == true)
        {
            Led_Off();
            Led_G_On();
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        else
        {
            //开启LED指示
            if (cg_data_led == 1)
            {
                //网络错误
                if (Net_sta_flag == false)
                {
                    Led_Off();
                    Led_R_On();
                    vTaskDelay(100 / portTICK_RATE_MS);
                }
                else if (human_status == true)
                {
                    Led_Off();
                    Led_R_fade_On();
                    Led_R_fade_Off();
                }
                else
                {
                    Led_Off();
                    vTaskDelay(100 / portTICK_RATE_MS);
                }
            }
            else
            {
                Led_Off();
                vTaskDelay(100 / portTICK_RATE_MS);
            }
        }
    }
}

void Led_Init(void)
{
    gpio_config_t io_conf;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << GPIO_LED_G);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    io_conf.pin_bit_mask = (1 << GPIO_LED_R);
    gpio_config(&io_conf);
    io_conf.pin_bit_mask = (1 << GPIO_LED_B);
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

    xTaskCreate(Led_Task, "Led_Task", 4096, NULL, 2, &Led_Task_Handle);
}

void Led_G_Off(void)
{
    ledc_set_duty_and_update(ledc_channel.speed_mode, ledc_channel.channel, 1023, 0);
    // ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 1023);
    // ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

void Led_R_On(void)
{
    gpio_set_level(GPIO_LED_R, 0);
}

void Led_G_On(void)
{
    ledc_set_duty_and_update(ledc_channel.speed_mode, ledc_channel.channel, 0, 0);
    // ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
    // ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

void Led_B_On(void)
{
    gpio_set_level(GPIO_LED_B, 0);
}

void Led_Off(void)
{
    gpio_set_level(GPIO_LED_R, 1);
    Led_G_Off();
    gpio_set_level(GPIO_LED_B, 1);
}

void Led_R_fade_Off(void)
{
    ledc_set_fade_time_and_start(ledc_channel.speed_mode, ledc_channel.channel, 1023, LEDC_TEST_FADE_TIME, LEDC_FADE_WAIT_DONE);
}
void Led_R_fade_On(void)
{
    ledc_set_fade_time_and_start(ledc_channel.speed_mode, ledc_channel.channel, 0, LEDC_TEST_FADE_TIME, LEDC_FADE_WAIT_DONE);
}
