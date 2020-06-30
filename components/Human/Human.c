#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Http.h"
#include "Led.h"
#include "Json_parse.h"
#include "Smartconfig.h"

#include "Human.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define TAG "HUMAN"

bool HUM_FLAG = false;
bool human_status = false;
uint64_t One_Risi_Time = 0, Last_Time = 0, human_intr_num = 0;

// static void huamn_timer_cb(void *arg);
// esp_timer_handle_t human_timer_handle = 0; //定时器句柄
// esp_timer_create_args_t human_timer_arg = {
//     .callback = &huamn_timer_cb,
//     .arg = NULL,
//     .name = "HumanTimer"};

static SemaphoreHandle_t human_binary_handle = NULL;

static void human_gpio_intr_handler(void *arg)
{
    static uint8_t change_num = 0;
    /* 获取触发中断的gpio口 */
    uint32_t key_num = (uint32_t)arg;
    /* 从中断处理函数中发出消息到队列 */
    // Led_Status = LED_STA_SEND;
    if (key_num == GPIO_HUMAN)
    {
        // if (fn_dp > 0)
        // {
        //     human_intr_num++;
        // }

        if (fn_sen != 0)
        {
            xSemaphoreGive(human_binary_handle);
        }
        //传感器判断
        if (HUM_FLAG == false)
        {
            if (change_num > 1)
            {
                HUM_FLAG = true; //传感器OK
            }
            else
            {
                change_num++;
            }
        }
    }

    // xQueueSendFromISR(human_evt_queue, &key_num, NULL);
}

void Human_Init(void)
{
    human_binary_handle = xSemaphoreCreateBinary();
    gpio_config_t io_conf;
    //enable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1) << (GPIO_HUMAN)));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_set_intr_type(GPIO_HUMAN, GPIO_INTR_ANYEDGE); //触发
    gpio_isr_handler_add(GPIO_HUMAN, human_gpio_intr_handler, (void *)(GPIO_HUMAN));
    //create a queue to handle gpio event from isr
    // human_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // esp_timer_create(&human_timer_arg, &human_timer_handle);

    xTaskCreate(Human_Task, "Human_Task", 4096, NULL, 4, &Human_Handle);
}

void Human_Task(void *arg)
{
    human_status = false;
    INT_FLAG = true;
    vTaskDelay(30 * 1000 / portTICK_RATE_MS); //电路稳定时间，根据手册，最大30s
    INT_FLAG = false;
    xEventGroupSetBits(Net_sta_group, HUMAN_I_BIT);
    while (1)
    {
        // if (xQueueReceive(human_evt_queue, &io_num, (30 * 1000) / portTICK_PERIOD_MS)) //30s无中断，则判断无人
        if (xSemaphoreTake(human_binary_handle, (30 * 1000) / portTICK_PERIOD_MS))
        {
            // ESP_LOGI(TAG, "true isr\n");
            // esp_timer_start_once(human_timer_handle, fn_sen * 100 * 1000); //fn_sen*100MS 灵敏度
            if (gpio_get_level(GPIO_HUMAN) == 1)
            {
                Last_Time = esp_timer_get_time();
            }
            else
            {
                One_Risi_Time = esp_timer_get_time() - Last_Time;
                // ESP_LOGI(TAG, "One_Risi_Time:%lld", One_Risi_Time);
                if (One_Risi_Time > (100000 * fn_sen)) //fn_sen*100ms
                {
                    if (human_status == false)
                    {
                        human_status = true;
                        if (Binary_dp != NULL) //立即上传数据
                        {
                            vTaskNotifyGiveFromISR(Binary_dp, NULL);
                            ESP_LOGI(TAG, "Have human\n");
                        }
                    }
                    human_intr_num++;
                }
            }
        }
        else
        {
            if (human_status == true)
            {
                human_status = false;
                if (Binary_dp != NULL) //立即上传数据
                {
                    vTaskNotifyGiveFromISR(Binary_dp, NULL);
                    ESP_LOGI(TAG, "No human");
                }
            }
            if (gpio_get_level(GPIO_HUMAN) == 1)
            {
                ESP_LOGE(TAG, "human err!");
                HUM_FLAG = false; //传感器坏
            }
        }
    }
}