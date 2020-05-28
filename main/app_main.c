#include <stdio.h>
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "Smartconfig.h"
#include "w5500_driver.h"
#include "Http.h"
#include "Nvs.h"
#include "Mqtt.h"
#include "Bluetooth.h"
#include "Json_parse.h"

#include "Human.h"
#include "Uart0.h"

#include "Led.h"
#include "E2prom.h"

#include "RtcUsr.h"

#include "sht30dis.h"
#include "user_app.h"
#include "ota.h"

static void Uart0_Task(void *arg)
{
    while (1)
    {
        Uart0_read();
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    Led_Init();
    E2prom_Init();
    Uart0_Init();
    Human_Init();
    w5500_user_int();

    user_app_key_init();
    xTaskCreate(Uart0_Task, "Uart0_Task", 4096, NULL, 9, NULL);
    printf("FIRMWARE=%s\n", FIRMWARE);

    E2prom_Read(SERISE_NUM_ADDR, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    printf("SerialNum=%s\n", SerialNum);
    E2prom_Read(PRODUCT_ID_ADDR, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    printf("ProductId=%s\n", ProductId);
    E2prom_Read(WEB_HOST_ADD, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    printf("Host=%s\n", WEB_SERVER);
    EE_byte_Read(ADDR_PAGE2, net_mode_add, &net_mode); //读取网络模式
    printf("net mode is %d!\n", net_mode);

    strncpy(ble_dev_pwd, SerialNum + 3, 4);
    printf("ble_dev_pwd=%s\n", ble_dev_pwd);

    ble_app_init();
    init_wifi();

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, -1); //等待网络连接

    initialise_http();
    initialise_mqtt();

    if ((strlen(SerialNum) == 0) || (strlen(ProductId) == 0) || (strlen(WEB_SERVER) == 0)) //未获取到序列号或productid，未烧写序列号
    {
        printf("no SerialNum or product id!\n");
        while (1)
        {
            //故障灯
            // Led_Status = LED_STA_NOSER;
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }
}
