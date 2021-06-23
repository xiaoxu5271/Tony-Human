#include <stdio.h>
#include "cJSON.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Http.h"
#include "Led.h"
#include "Json_parse.h"
#include "Smartconfig.h"
#include "ServerTimer.h"

#include "Human.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define TAG "HUMAN"

TaskHandle_t Human_Handle = NULL;
TaskHandle_t Binary_sta = NULL;
QueueHandle_t human_evt_queue = NULL;

bool HUM_FLAG = false;
bool human_status = false;
bool Timer2_flag = false;

uint64_t One_Risi_Time = 0, Last_Time = 0, human_intr_num = 0;

static void huamn_timer_cb(void *arg);
esp_timer_handle_t human_timer_handle = 0; //定时器句柄
esp_timer_create_args_t human_timer_arg = {
    .callback = &huamn_timer_cb,
    .arg = NULL,
    .name = "HumanTimer"};

static void huamn_timer2_cb(void *arg);
esp_timer_handle_t human_timer2_handle = 0; //定时器句柄
esp_timer_create_args_t human_timer2_arg = {
    .callback = &huamn_timer2_cb,
    .arg = NULL,
    .name = "HumanTimer2"};

static void huamn_timer_cb(void *arg)
{

    uint8_t Event_num = TIMER_CB;
    if (human_evt_queue != NULL)
        xQueueSendFromISR(human_evt_queue, &Event_num, NULL);
}
//用于判断无人
static void huamn_timer2_cb(void *arg)
{
    uint8_t Event_num = TIMER2_CB;
    if (human_evt_queue != NULL)
        xQueueSendFromISR(human_evt_queue, &Event_num, NULL);
}

static void human_gpio_intr_handler(void *arg)
{
    static uint8_t change_num = 0;
    /* 获取触发中断的gpio口 */
    uint32_t key_num = (uint32_t)arg;
    uint8_t Event_num = GPIO_CB;
    /* 从中断处理函数中发出消息到队列 */
    // Led_Status = LED_STA_SEND;
    if (key_num == GPIO_HUMAN)
    {
        if (fn_sen != 0)
        {
            if (human_evt_queue != NULL)
                xQueueSendFromISR(human_evt_queue, &Event_num, NULL);
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

void Human_Task(void *arg)
{
    human_status = false;
    INT_FLAG = true;
    vTaskDelay(10 * 1000 / portTICK_RATE_MS); //电路稳定时间，根据手册，最大30s
    INT_FLAG = false;
    xEventGroupSetBits(Net_sta_group, HUMAN_I_BIT);
    uint8_t human_evt = 0;
    while (1)
    {
        // ulTaskNotifyTake(pdTRUE, -1);
        if (xQueueReceive(human_evt_queue, (void *)&human_evt, (portTickType)portMAX_DELAY))
        {
            switch (human_evt)
            {
                //IO中断
            case GPIO_CB:
                if (gpio_get_level(GPIO_HUMAN) == 1)
                {
                    Last_Time = esp_timer_get_time();
                }
                else
                {
                    if (Last_Time > 0)
                    {
                        One_Risi_Time += (esp_timer_get_time() - Last_Time);
                    }
                }
                // ESP_LOGI(TAG, "One_Risi_Time:%lld", One_Risi_Time);
                break;

                //有人判定周期定时器
            case TIMER_CB:
                //如果进入下周期时，传感器仍上升沿，则统计上周期上升时间
                if (gpio_get_level(GPIO_HUMAN) == 1)
                {
                    if (Last_Time > 0)
                    {
                        One_Risi_Time += esp_timer_get_time() - Last_Time;
                    }
                    Last_Time = esp_timer_get_time();
                }

                printf("One_Risi_Time=%lld\r\n", One_Risi_Time);

                if (One_Risi_Time > (uint64_t)(1000 * fn_sen)) //fn_sen*100ms
                {
                    // ESP_LOGI(TAG, "One_Risi_Time:%lld,fn_sen:%lld", One_Risi_Time, (uint64_t)(1000 * fn_sen));
                    if (Timer2_flag == true)
                    {
                        esp_timer_stop(human_timer2_handle);
                        Timer2_flag = false;
                    }

                    if (human_status == false)
                    {
                        human_status = true;
                        ESP_LOGI(TAG, "Have human\n");
                        if (Binary_sta != NULL)
                        {
                            xTaskNotifyGive(Binary_sta);
                        }
                    }
                }

                //累加统计上升时间
                human_intr_num += (uint64_t)(One_Risi_Time / 1000);
                printf("human_intr_num=%lld\r\n", human_intr_num);

                //清空本次统计上升时间
                One_Risi_Time = 0;

                if (Timer2_flag == false)
                {
                    Timer2_flag = true;
                    if (fn_sen_res != 0)
                    {
                        esp_timer_start_once(human_timer2_handle, fn_sen_res * 1000);
                    }
                }
                break;

                //恢复无人判定周期定时器
            case TIMER2_CB:
                if (human_status == true)
                {
                    human_status = false;
                    ESP_LOGI(TAG, "No human");
                }
                Timer2_flag = false;
                break;

            default:
                break;
            }
        }
    }
}

void Create_human_intr_task(void *arg)
{
    char *OutBuffer;
    char *time_buff;
    // uint8_t *SaveBuffer;
    uint16_t len = 0;
    cJSON *pJsonRoot;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, -1);

        if ((xEventGroupGetBits(Net_sta_group) & TIME_CAL_BIT) == TIME_CAL_BIT)
        {
            time_buff = (char *)malloc(24);
            Server_Timer_SEND(time_buff);
            pJsonRoot = cJSON_CreateObject();
            cJSON_AddStringToObject(pJsonRoot, "created_at", (const char *)time_buff);
            cJSON_AddItemToObject(pJsonRoot, "field3", cJSON_CreateNumber(human_intr_num));
            //清空统计
            human_intr_num = 0;

            OutBuffer = cJSON_PrintUnformatted(pJsonRoot); //cJSON_Print(Root)
            if (OutBuffer != NULL)
            {
                len = strlen(OutBuffer);
                ESP_LOGI(TAG, "len:%d\n%s\n", len, OutBuffer);
                xSemaphoreTake(Cache_muxtex, -1);
                DataSave((uint8_t *)OutBuffer, len);
                xSemaphoreGive(Cache_muxtex);
                cJSON_free(OutBuffer);
            }
            cJSON_Delete(pJsonRoot); //delete cjson root

            free(time_buff);
        }
    }
}

void Create_human_sta_task(void *arg)
{
    char *OutBuffer;
    char *time_buff;
    // uint8_t *SaveBuffer;
    uint16_t len = 0;
    cJSON *pJsonRoot;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, -1);

        if ((xEventGroupGetBits(Net_sta_group) & TIME_CAL_BIT) == TIME_CAL_BIT)
        {
            time_buff = (char *)malloc(24);
            Server_Timer_SEND(time_buff);
            pJsonRoot = cJSON_CreateObject();
            cJSON_AddStringToObject(pJsonRoot, "created_at", (const char *)time_buff);
            cJSON_AddItemToObject(pJsonRoot, "field1", cJSON_CreateNumber(human_status));

            OutBuffer = cJSON_PrintUnformatted(pJsonRoot); //cJSON_Print(Root)
            if (OutBuffer != NULL)
            {
                len = strlen(OutBuffer);
                ESP_LOGI(TAG, "len:%d\n%s\n", len, OutBuffer);
                xSemaphoreTake(Cache_muxtex, -1);
                DataSave((uint8_t *)OutBuffer, len);
                xSemaphoreGive(Cache_muxtex);
                cJSON_free(OutBuffer);
            }
            cJSON_Delete(pJsonRoot); //delete cjson root

            free(time_buff);
        }
    }
}

void Human_Init(void)
{
    human_evt_queue = xQueueCreate(20, sizeof(uint8_t));
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

    esp_timer_create(&human_timer_arg, &human_timer_handle);
    esp_timer_create(&human_timer2_arg, &human_timer2_handle);
    if (fn_sen_cycle == 0)
    {
        fn_sen_cycle = 500;
    }
    esp_timer_start_periodic(human_timer_handle, fn_sen_cycle * 1000);
    xTaskCreate(Human_Task, "Human_Task", 4096, NULL, 4, &Human_Handle);
    xTaskCreate(Create_human_sta_task, "Create_human_sta_task", 4096, NULL, 3, &Binary_sta);
    xTaskCreate(Create_human_intr_task, "Create_human_intr_task", 4096, NULL, 3, &Binary_intr);
}

void Change_Fn_sen(uint32_t Fn_sen)
{
    esp_timer_stop(human_timer_handle);
    esp_timer_start_periodic(human_timer_handle, Fn_sen * 1000);
}