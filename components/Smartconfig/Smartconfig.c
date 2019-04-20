#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"

#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
/*  user include */
#include "Smartconfig.h"
#include "esp_log.h"
#include "Led.h"
#include "tcp_bsp.h"

wifi_config_t s_staconf;

enum wifi_connect_sta
{
        connect_Y = 1,
        connect_N = 2,
};
uint8_t wifi_con_sta = 0;
uint8_t start_AP = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:
                //esp_wifi_connect();
                break;

        case SYSTEM_EVENT_STA_GOT_IP:
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                Led_Status = LED_STA_AUTO; //联网工作
                break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
                Led_Status = LED_STA_FIRE; //断网
                ESP_LOGI(TAG, "断网");
                //esp_timer_start_periodic(timer_wifi_dis, 1000000); //创建定时器，单位us，定时1s
                //wifi_init_apsta();         //断网后，进入AP+STA配网模式，同时继续等待原有WIFI重新连接
                if (start_AP == 1)
                {
                        start_AP = 0;
                        esp_wifi_connect();
                }

                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                break;

        case SYSTEM_EVENT_AP_STACONNECTED: //AP模式-有STA连接成功
                //作为ap，有sta连接
                ESP_LOGI(TAG, "station:" MACSTR " join,AID=%d\n",
                         MAC2STR(event->event_info.sta_connected.mac),
                         event->event_info.sta_connected.aid);
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                break;

        case SYSTEM_EVENT_AP_STADISCONNECTED: //AP模式-有STA断线
                ESP_LOGI(TAG, "station:" MACSTR "leave,AID=%d\n",
                         MAC2STR(event->event_info.sta_disconnected.mac),
                         event->event_info.sta_disconnected.aid);

                g_rxtx_need_restart = true;
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                break;

        default:
                break;
        }
        return ESP_OK;
}

void initialise_wifi(char *wifi_ssid, char *wifi_password)
{
        printf("WIFI Reconnect,SSID=%s,PWD=%s\r\n", wifi_ssid, wifi_password);

        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &s_staconf));
        if (s_staconf.sta.ssid[0] == '\0') //判断当前系统中是否有WIFI信息,
        {
                strcpy(s_staconf.sta.ssid, wifi_ssid);
                strcpy(s_staconf.sta.password, wifi_password);

                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &s_staconf));
                ESP_ERROR_CHECK(esp_wifi_start());
                esp_wifi_connect();
        }
        else //有,清空,重新写入
        {
                ESP_ERROR_CHECK(esp_wifi_stop());
                memset(&s_staconf.sta, 0, sizeof(s_staconf));
                //printf("WIFI CHANGE\r\n");
                strcpy(s_staconf.sta.ssid, wifi_ssid);
                strcpy(s_staconf.sta.password, wifi_password);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &s_staconf));
                ESP_ERROR_CHECK(esp_wifi_start());
                esp_wifi_connect();
        }
}

void reconnect_wifi_usr(void)
{
        printf("WIFI Reconnect\r\n");
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &s_staconf));

        ESP_ERROR_CHECK(esp_wifi_stop());
        memset(&s_staconf.sta, 0, sizeof(s_staconf));

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &s_staconf));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
}

void init_wifi(void) //
{
        tcpip_adapter_init();
        wifi_event_group = xEventGroupCreate();
        memset(&s_staconf.sta, 0, sizeof(s_staconf));
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); //实验，测试解决wifi中断问题
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &s_staconf));
        if (s_staconf.sta.ssid[0] != '\0')
        {
                printf("wifi_init_sta finished.");
                printf("connect to ap SSID:%s password:%s\r\n",
                       s_staconf.sta.ssid, s_staconf.sta.password);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &s_staconf));
                ESP_ERROR_CHECK(esp_wifi_start());
                esp_wifi_connect();
                //Led_Status = LED_STA_TOUCH;
        }
        else
        {
                printf("Waiting for SetupWifi ....\r\n");
                wifi_init_softap();
                my_tcp_connect();
        }
}

/*
* WIFI作为AP的初始化
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
                    hx-zsj, 2018/08/06, 初始化版本\n 
*/
void wifi_init_softap(void)
{
        start_AP = 1;
        ESP_ERROR_CHECK(esp_wifi_stop());
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = SOFT_AP_SSID,
                .password = SOFT_AP_PAS,
                .ssid_len = 0,
                .max_connection = SOFT_AP_MAX_CONNECT,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK}};
        if (strlen(SOFT_AP_PAS) == 0)
        {
                wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "SoftAP set finish,SSID:%s password:%s \n",
                 wifi_config.ap.ssid, wifi_config.ap.password);

        xTaskCreate(&my_tcp_connect_task, "my_tcp_connect_task", 4096, NULL, 5, NULL);
}

/*
* WIFI作为AP+STA的初始化
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:

*/
void wifi_init_apsta(void)
{
        wifi_event_group = xEventGroupCreate();
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        wifi_config_t sta_wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &sta_wifi_config));

        wifi_config_t ap_wifi_config =
            {
                .ap = {
                    .ssid = "esp32_ap",
                    .password = "",
                    //     .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                    .max_connection = 1,
                },
            };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
}
