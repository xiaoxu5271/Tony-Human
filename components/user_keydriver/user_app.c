
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
#include "tcp_bsp.h"
#include "Smartconfig.h"

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
                        ESP_LOGI("short_pressed_cb", "first press!!!\n");

                        break;
                case 2:
                        ESP_LOGI("short_pressed_cb", "double press!!!\n");
                        break;
                case 3:
                        ESP_LOGI("short_pressed_cb", "trible press!!!\n");
                        break;
                case 4:
                        ESP_LOGI("short_pressed_cb", "quatary press!!!\n");
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

                Task_key_num = 1;

                break;
        default:
                break;
        }
}

void user_key_cd_task(void)
{
        while (1)
        {
                switch (Task_key_num)
                {
                case 1:
                        Task_key_num = 0;
                        printf("AP START....\r\n");
                        wifi_init_softap();
                        //initialise_wifi("XIAOXU", "12345678");
                        //my_tcp_connect();
                        break;

                default:
                        break;
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
        xTaskCreate(user_key_cd_task, "user_key_cd_task", 8192, NULL, 1, NULL);
}