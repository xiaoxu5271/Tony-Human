#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Human.h"
#include "Http.h"
#include "Led.h"

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
}

void Humanapp(void)
{
        int human_gpio_value;

        human_gpio_value = gpio_get_level(GPIO_HUMAN); //读取人感电平
        ESP_LOGI(" ", "human_gpio_value=%d\n", human_gpio_value);
        if (human_gpio_value == 1) //传感器报有人
        {

                havehuman_count++;
                printf("havehuman_count=%d\n", havehuman_count);
        }

        //需要把数据发送到平台
        if (need_send == 1)
        {
                http_send_mes(POST_NORMAL);
                need_send = 0;
        }
}

void Human_Task(void *arg)
{
        while (1)
        {
                Humanapp();

                vTaskDelay(200 / portTICK_RATE_MS);
        }
}