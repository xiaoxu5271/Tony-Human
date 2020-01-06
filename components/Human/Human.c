#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Human.h"
#include "Http.h"
#include "Led.h"

#define TAG "HUNAN"

void timer_periodic_cb(void *arg);

esp_timer_handle_t timer_periodic_handle = 0; //定时器句柄

esp_timer_create_args_t timer_periodic_arg = {
    .callback =
        &timer_periodic_cb,
    .arg = NULL,
    .name = "PeriodicTimer"};

void timer_periodic_cb(void *arg) //200ms中断一次
{
    static uint64_t timer_count = 0;
    static uint64_t nohuman_timer_count = 0;
    timer_count++;
    nohuman_timer_count++;

    if (human_status == HAVEHUMAN) //有人时，1s内右2个1则转为有人
    {
        if (timer_count >= 10) //2s
        {
            timer_count = 0;
            if (havehuman_count >= 4)
            {
                human_status = HAVEHUMAN;
                printf("human_status1=%d\n", human_status);

                //need_send=1;

                havehuman_count = 0;
                nohuman_timer_count = 0;
            }
            else
            {
                havehuman_count = 0;
            }
        }
    }

    if (human_status == NOHUMAN) //无人时，2s内右6个1则转为有人
    {
        if (timer_count >= 10) //2s
        {
            timer_count = 0;
            if (havehuman_count >= 6)
            {
                if (human_status == NOHUMAN) //如当前是无人，立即上传有人
                {
                    if (Binary_Http_Send != NULL)
                    {
                        xSemaphoreGive(Binary_Http_Send);
                    }
                }
                human_status = HAVEHUMAN;
                printf("human_status2=%d\n", human_status);
                havehuman_count = 0;
                nohuman_timer_count = 0;
            }
            else
            {
                havehuman_count = 0;
            }
        }
    }

    if (nohuman_timer_count >= 4500) //900s 15min
    {
        human_status = NOHUMAN;
        nohuman_timer_count = 0;
        printf("human_status=%d\n", human_status);
    }
}

void Human_Init(void)
{
    //配置继电器输出管脚
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = 1 << GPIO_HUMAN;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // esp_err_t err = esp_timer_create(&timer_periodic_arg, &timer_periodic_handle);
    // err = esp_timer_start_periodic(timer_periodic_handle, 200000); //创建定时器，单位us，定时200ms
    // if (err != ESP_OK)
    // {
    //     printf("timer periodic create err code:%d\n", err);
    // }
}

void Humanapp(void)
{
    int human_gpio_value;

    human_gpio_value = gpio_get_level(GPIO_HUMAN); //读取人感电平
    ESP_LOGD(TAG, "human_gpio_value=%d\n", human_gpio_value);
    if (human_gpio_value == 1) //传感器报有人
    {

        havehuman_count++;
        ESP_LOGD(TAG, "havehuman_count=%d\n", havehuman_count);
    }
}

void Human_Task(void *arg)
{
    uint64_t timer_count = 0;
    uint64_t nohuman_timer_count = 0;
    while (1)
    {
        Humanapp();
        timer_count++;
        nohuman_timer_count++;

        if (timer_count >= 10)
        {
            timer_count = 0;
            if (havehuman_count >= 6)
            {
                havehuman_count = 0;
                if (human_status == NOHUMAN) //  突变
                {
                    if (Binary_Http_Send != NULL) //立即上传数据
                    {
                        xSemaphoreGive(Binary_Http_Send);
                    }
                }
                human_status = HAVEHUMAN;
            }
            else
            {
                havehuman_count = 0;
                // if (human_status == HAVEHUMAN) //  突变
                // {
                //     if (Binary_Http_Send != NULL) //立即上传数据
                //     {
                //         xSemaphoreGive(Binary_Http_Send);
                //     }
                // }
                human_status = NOHUMAN;
            }
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}