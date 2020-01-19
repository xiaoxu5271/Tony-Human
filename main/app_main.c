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
    nvs_flash_init(); //初始化flash
    work_status = WORK_INIT;
    mqtt_json_s.mqtt_height = -1;
    mqtt_json_s.mqtt_angle = -1;

    //Led_On();
    E2prom_Init();
    //Fire_Init();
    Uart0_Init();
    Human_Init();
    Led_Init();
    user_app_key_init();
    xTaskCreate(Uart0_Task, "Uart0_Task", 4096, NULL, 9, NULL);
    /*step1 判断是否有序列号和product id****/
    E2prom_Read(SERISE_NUM_ADDR, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    printf("SerialNum=%s\n", SerialNum);

    E2prom_Read(PRODUCT_ID_ADDR, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    printf("ProductId=%s\n", ProductId);

    E2prom_Read(WEB_HOST_ADD, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    printf("Host=%s\n", WEB_SERVER);

    printf("FIRMWARE=%s\n", FIRMWARE);

    if ((SerialNum[0] == 0xff) && (SerialNum[1] == 0xff)) //新的eeprom，先清零
    {
        printf("new eeprom\n");
        E2prom_empty_all();

        EE_byte_Write(ADDR_PAGE2, need_update_add, 0);     //存放OTA升级需求参数
        EE_byte_Write(ADDR_PAGE2, update_fail_num_add, 0); //存放OTA升级重试次数
        EE_byte_Write(ADDR_PAGE2, net_mode_add, NET_AUTO); //写入net_mode
        EE_byte_Write(ADDR_PAGE2, dhcp_mode_add, 1);       //写入DHCP模式，默认开启
    }
    EE_byte_Read(ADDR_PAGE2, net_mode_add, &net_mode); //读取网络模式
    printf("net mode is %d!\n", net_mode);

    if ((strlen(SerialNum) == 0) || (strlen(ProductId) == 0) || (strlen(WEB_SERVER) == 0)) //未获取到序列号或productid，未烧写序列号
    {
        printf("no SerialNum or product id!\n");
        while (1)
        {
            //故障灯
            Led_Status = LED_STA_NOSER;
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    strncpy(ble_dev_pwd, SerialNum + 3, 4);
    printf("ble_dev_pwd=%s\n", ble_dev_pwd);

    ble_app_init();
    init_wifi();
    w5500_user_int();

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, -1); //等待网络连接

    initialise_http();
    initialise_mqtt();

    // EE_byte_Read(ADDR_PAGE2, need_update_add, &need_update);
    // EE_byte_Read(ADDR_PAGE2, update_fail_num_add, &update_fail_num);
    // printf(" nead_update: %d       update_fail_num: %d\n", need_update, update_fail_num);
    // if (need_update == 1)
    // {
    //     printf("需要升级，已失败次数 %d \n", update_fail_num);
    //     ota_start();
    // }
    // else
    // {
    //     printf("无需升级\n");
    // }
}
