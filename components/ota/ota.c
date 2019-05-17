#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "Smartconfig.h"
#include "Json_parse.h"
#include "Http.h"
#include "Human.h"
#include "sht30dis.h"
#include "Mqtt.h"
#include "E2prom.h"

#include "ota.h"

static const char *TAG = "ota";
TaskHandle_t ota_handle = NULL;
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
        switch (evt->event_id)
        {
        case HTTP_EVENT_ERROR:
                ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
                break;
        case HTTP_EVENT_ON_CONNECTED:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
                break;
        case HTTP_EVENT_HEADER_SENT:
                ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
                break;
        case HTTP_EVENT_ON_HEADER:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
                break;
        case HTTP_EVENT_ON_DATA:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                break;
        case HTTP_EVENT_ON_FINISH:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
                break;
        case HTTP_EVENT_DISCONNECTED:
                ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
                break;
        }
        return ESP_OK;
}

void ota_task(void *arg)
{
        ESP_LOGI(TAG, "Starting OTA...");
        esp_timer_stop(http_timer_suspend_p);
        vTaskSuspend(httpHandle);
        vTaskSuspend(Sht30_Handle);
        vTaskSuspend(Human_Handle);
        stop_wifi_mqtt();

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to ota Server....");
        E2prom_Ota_Read(0x00, (uint8_t *)mqtt_json_s.mqtt_ota_url, 128);
        ESP_LOGI(TAG, "OTA-URL=[%s]\r\n", mqtt_json_s.mqtt_ota_url);

        esp_http_client_config_t config = {
            .url = mqtt_json_s.mqtt_ota_url,
            .timeout_ms = 10000,
            //     .cert_pem = (char *)server_cert_pem_start, //如果使用http地址则需要屏蔽此条代码
            .event_handler = _http_event_handler,
        };

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK)
        {
                ESP_LOGI(TAG, "Update Successed\r\n ");

                if (need_update != 0)
                {
                        need_update = 0;
                        EE_byte_Write(ADDR_PAGE2, need_update_add, need_update); //存放OTA升级需求参数
                }
                if (update_fail_num != 0)
                {
                        update_fail_num = 0;
                        EE_byte_Write(ADDR_PAGE2, update_fail_num_add, update_fail_num); //存放OTA升级重试次数
                }

                vTaskDelay(1000 / portTICK_RATE_MS); //延时
                esp_restart();
        }
        else
        {
                update_fail_num++;
                if (update_fail_num > 3)
                {
                        need_update = 0;
                        update_fail_num = 0;
                        EE_byte_Write(ADDR_PAGE2, need_update_add, need_update);         //存放OTA升级需求参数
                        EE_byte_Write(ADDR_PAGE2, update_fail_num_add, update_fail_num); //存放OTA升级重试次数

                        ESP_LOGI(TAG, "多次升级后仍失败，请检查网络或者更换路由器解决\r\n ");
                        esp_timer_start_periodic(http_timer_suspend_p, 1000 * 1000 * 10);
                        xTaskResumeFromISR(Sht30_Handle);
                        xTaskResumeFromISR(Human_Handle);
                        start_wifi_mqtt();
                }
                else
                {
                        need_update = 1;
                        EE_byte_Write(ADDR_PAGE2, need_update_add, need_update);         //存放OTA升级需求参数
                        EE_byte_Write(ADDR_PAGE2, update_fail_num_add, update_fail_num); //存放OTA升级重试次数
                        ESP_LOGE(TAG, " Update Failed, Retry after reboot.  Failed  %d times!  \r\n", update_fail_num);

                        vTaskDelay(1000 / portTICK_RATE_MS); //延时
                        esp_restart();
                }
        }

        vTaskDelete(NULL); //删除自身任务
}

void ota_start(void) //建立OTA升级任务，目的是为了让此函数被调用后尽快执行完毕
{
        xTaskCreate(ota_task, "ota_task", 8192, NULL, 2, ota_handle);
}