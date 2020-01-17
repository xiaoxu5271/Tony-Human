#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Human.h"
#include "Http.h"
#include "Led.h"
#include "Json_parse.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define TAG "HUMAN"

uint8_t human_chack = 0;

static void huamn_timer_cb(void *arg);
esp_timer_handle_t human_timer_handle = 0; //定时器句柄
esp_timer_create_args_t human_timer_arg = {
    .callback =
        &huamn_timer_cb,
    .arg = NULL,
    .name = "HumanTimer"};

static xQueueHandle human_evt_queue = NULL;
static SemaphoreHandle_t human_binary_handle = NULL;

static void
human_gpio_intr_handler(void *arg)
{
    static uint8_t change_num = 0;
    /* 获取触发中断的gpio口 */
    uint32_t key_num = (uint32_t)arg;
    /* 从中断处理函数中发出消息到队列 */
    // Led_Status = LED_STA_SEND;
    if (key_num == GPIO_HUMAN)
    {
        if (fn_sen != 0)
        {
            xSemaphoreGive(human_binary_handle);
        }
        //传感器判断
        if (human_chack == 0)
        {
            if (change_num > 5)
            {
                human_chack = 1; //传感器OK
            }
            else
            {
                change_num++;
            }
        }
    }

    // xQueueSendFromISR(human_evt_queue, &key_num, NULL);
}

void huamn_timer_cb(void *arg)
{
    if (gpio_get_level(GPIO_HUMAN) == 1)
    {
        if (human_status == NOHUMAN)
        {
            human_status = HAVEHUMAN;
            if (Binary_Http_Send != NULL) //立即上传数据
            {
                xSemaphoreGive(Binary_Http_Send);
            }
        }
        ESP_LOGI(TAG, "HAVEHUMAN !\n");
    }
}

void Human_Init(void)
{
    gpio_config_t io_conf;
    //enable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1) << (GPIO_HUMAN)));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_set_intr_type(GPIO_HUMAN, GPIO_INTR_POSEDGE); //上升沿触发
    gpio_isr_handler_add(GPIO_HUMAN, human_gpio_intr_handler, (void *)(GPIO_HUMAN));
    //create a queue to handle gpio event from isr
    // human_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    human_binary_handle = xSemaphoreCreateBinary();

    esp_timer_create(&human_timer_arg, &human_timer_handle);

    xTaskCreate(Human_Task, "Human_Task", 4096, NULL, 4, &Human_Handle);
}

void Human_Task(void *arg)
{
    uint32_t io_num;
    human_status = NOHUMAN;
    vTaskDelay(30 * 1000 / portTICK_RATE_MS); //电路稳定时间，根据手册，最大30s
    while (1)
    {
        // if (xQueueReceive(human_evt_queue, &io_num, (30 * 1000) / portTICK_PERIOD_MS)) //30s无中断，则判断无人
        if (xSemaphoreTake(human_binary_handle, (30 * 1000) / portTICK_PERIOD_MS))
        {
            // ESP_LOGI(TAG, "HAVEHUMAN isr\n");
            esp_timer_start_once(human_timer_handle, fn_sen * 100 * 1000); //fn_sen*100MS 灵敏度
        }
        else
        {
            human_status = NOHUMAN;
            ESP_LOGI(TAG, "NOHUMAN\n");
        }
    }
}