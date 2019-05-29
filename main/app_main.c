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
                    need_send = 1;
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

static void Uart0_Task(void *arg)
{
    while (1)
    {
        Uart0_read();

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

/*
  EEPROM PAGE0 
    0x00 APIkey(32byte)
    0x20 chnnel_id(4byte)
    0x30 Serial_No(16byte)
    0x40 Protuct_id(32byte)
  EEPROM PAGE1
    0X00 bluesave  (256byte)
*/

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
    //   strcpy(SerialNum,"AAA0003HUM1");
    //   strcpy(ProductId,"28343913545840b3b9b42c568e78e243");

    xTaskCreate(Uart0_Task, "Uart0_Task", 4096, NULL, 10, NULL);

    /*step1 判断是否有序列号和product id****/
    E2prom_Read(0x30, (uint8_t *)SerialNum, 16);
    printf("SerialNum=%s\n", SerialNum);

    E2prom_Read(0x40, (uint8_t *)ProductId, 32);
    printf("ProductId=%s\n", ProductId);

    // EE_byte_Write(ADDR_PAGE2, need_update_add, 0);     //存放OTA升级需求参数
    // EE_byte_Write(ADDR_PAGE2, update_fail_num_add, 0); //存放OTA升级重试次数
    // char zero_data[256];
    // bzero(zero_data, sizeof(zero_data));
    // E2prom_page_Write(0x00, (uint8_t *)zero_data, 256); //清空蓝牙

    if ((SerialNum[0] == 0xff) && (SerialNum[1] == 0xff)) //新的eeprom，先清零
    {
        printf("new eeprom\n");
        char zero_data[256];
        bzero(zero_data, sizeof(zero_data));
        E2prom_Write(0x00, (uint8_t *)zero_data, 256);
        E2prom_BluWrite(0x00, (uint8_t *)zero_data, 256);   //清空蓝牙
        E2prom_page_Write(0x00, (uint8_t *)zero_data, 256); //清空蓝牙

        E2prom_Read(0x30, (uint8_t *)SerialNum, 16);
        printf("SerialNum=%s\n", SerialNum);

        E2prom_Read(0x40, (uint8_t *)ProductId, 32);
        printf("ProductId=%s\n", ProductId);

        EE_byte_Write(ADDR_PAGE2, need_update_add, 0);     //存放OTA升级需求参数
        EE_byte_Write(ADDR_PAGE2, update_fail_num_add, 0); //存放OTA升级重试次数
    }

    if ((strlen(SerialNum) == 0) || (strlen(ProductId) == 0)) //未获取到序列号或productid，未烧写序列号
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

    ble_app_start();
    init_wifi();
    w5500_user_int();

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY); //等待网络连接、

    vTaskDelay(5000 / portTICK_RATE_MS); //延时5s，开机滤掉抖动状态
    human_status = NOHUMAN;
    /*******************************timer 1s init**********************************************/
    esp_err_t err = esp_timer_create(&timer_periodic_arg, &timer_periodic_handle);
    err = esp_timer_start_periodic(timer_periodic_handle, 200000); //创建定时器，单位us，定时200ms
    if (err != ESP_OK)
    {
        printf("timer periodic create err code:%d\n", err);
    }

    xTaskCreate(Human_Task, "Human_Task", 8192, NULL, 5, &Human_Handle);
    xTaskCreate(Sht30_Task, "Sht30_Task", 8192, NULL, 5, &Sht30_Handle);

    initialise_http();
    initialise_mqtt();

    EE_byte_Read(ADDR_PAGE2, need_update_add, &need_update);
    EE_byte_Read(ADDR_PAGE2, update_fail_num_add, &update_fail_num);
    printf(" nead_update: %d       update_fail_num: %d\n", need_update, update_fail_num);
    if (need_update == 1)
    {
        printf("需要升级，已失败次数 %d \n", update_fail_num);
        ota_start();
    }
    else
    {
        printf("无需升级\n");
    }
}
