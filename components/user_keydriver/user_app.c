
// ===========================
// 头文件包含
// ===========================
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "user_key.h"
#include <esp_log.h>
// #include "tcp_bsp.h"
#include "Smartconfig.h"
#include "My_Mqtt.h"
#include "w5500_driver.h"
#include "ota.h"
#include "Bluetooth.h"
#include "E2prom.h"
#include "Json_parse.h"
#include "Led.h"

#define TAG "USER_KET"

uint8_t Task_key_num = 0;

/* 填充需要配置的按键个数以及对应的相关参数 */
static key_config_t gs_m_key_config[BOARD_BUTTON_COUNT] =
    {
        {BOARD_BUTTON, APP_KEY_ACTIVE_LOW, 0, LONG_PRESSED_TIMER},
};

/** 
 * 用户的短按处理函数
 * @param[in]   key_num                 :短按按键对应GPIO口
 * @param[in]   short_pressed_counts    :短按按键对应GPIO口按下的次数,这里用不上
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
void short_pressed_cb(uint8_t key_num, uint8_t *short_pressed_counts)
{
    switch (key_num)
    {
    case BOARD_BUTTON:
        switch (*short_pressed_counts)
        {
        case 1:
            // ESP_LOGI("short_pressed_cb", "first press!!!\n");
            Task_key_num = 1;
            break;
        case 2:
            // ESP_LOGI("short_pressed_cb", "double press!!!\n");
            break;
        case 3:
            // ESP_LOGI("short_pressed_cb", "trible press!!!\n");
            if (Cnof_net_flag == true)
            {
                Set_defaul_flag = true;
                E2prom_set_defaul(true);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                esp_restart();
            }
            break;
        case 4:
            // ESP_LOGI("short_pressed_cb", "quatary press!!!\n");
            break;
            // case ....:
            // break;

        default:
            break;
        }
        *short_pressed_counts = 0;
        break;

    default:
        break;
    }
}

/** 
 * 用户的长按处理函数
 * @param[in]   key_num                 :短按按键对应GPIO口
 * @param[in]   long_pressed_counts     :按键对应GPIO口按下的次数,这里用不上
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
void long_pressed_cb(uint8_t key_num, uint8_t *long_pressed_counts)
{
    switch (key_num)
    {
    case BOARD_BUTTON:
        ESP_LOGI("long_pressed_cb", "long press!!!\n");
        if (Cnof_net_flag)
        {
            ble_app_stop();
            Net_Switch();
        }
        else
        {
            ble_app_start();
            start_softap();
        }

        break;
    default:
        break;
    }
}

static void vTask_view_Work(void *pvParameters)
{
    uint8_t pcWriteBuffer[2048];

    while (1)
    {
        if (Task_key_num == 2)
        {
            Task_key_num = 0;

            ESP_LOGI(TAG, "free Heap:%d\n", esp_get_free_heap_size());
            /* K1键按下 打印任务执行情况 */

            ESP_LOGI(TAG, "=======================================================\r\n");
            ESP_LOGI(TAG, "任务名           任务状态   优先级      剩余栈   任务序号\r\n");
            vTaskList((char *)&pcWriteBuffer);
            ESP_LOGI(TAG, "%s\r\n", pcWriteBuffer);

            ESP_LOGI(TAG, "\r\n任务名            运行计数              使用率\r\n");
            vTaskGetRunTimeStats((char *)&pcWriteBuffer);
            ESP_LOGI(TAG, "%s\r\n", pcWriteBuffer);

            /* 其他的键值不处理 */
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

/** 
 * 用户的按键初始化函数
 * @param[in]   null 
 * @retval      null
 * @par         修改日志 
 *               Ver0.0.1:
                     Helon_Chan, 2018/06/16, 初始化版本\n 
 */
void user_app_key_init(void)
{
    int32_t err_code;
    err_code = user_key_init(gs_m_key_config, BOARD_BUTTON_COUNT, DECOUNE_TIMER, long_pressed_cb, short_pressed_cb);
    ESP_LOGI("user_app_key_init", "user_key_init is %d\n", err_code);
    // xTaskCreate(vTask_view_Work, "vTask_view_Work", 10240, NULL, 5, NULL);
}