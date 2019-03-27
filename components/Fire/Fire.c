#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Fire.h"
#include "Http.h"
#include "Json_parse.h"
#include "Motorctl.h"


static const char *TAG = "Fire";


void Fire_interrupt_callBack(void* arg); 
static xQueueHandle gpio_evt_queue = NULL; //定义一个队列返回变量


void Fire_interrupt_callBack(void* arg) 
{
    uint32_t io_num;
    while (1) 
    {
        //不断读取gpio队列，读取完后将删除队列
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) //队列阻塞等待
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            //有火是0，没火是1
            ESP_LOGW(TAG, "FIRE_interrupt,gpio[%d]=%d\n", io_num,gpio_get_level(io_num));
            if(gpio_get_level(io_num)==0) //火灾
            {
                printf("on fire!\n");
                mqtt_json_s.mqtt_fire_alarm=1;
                http_send_mes(POST_ALLUP);  
                work_status=WORK_FIRE;
                Motor_AutoCtl(0,0);
            }
            else//火灾解除
            {
                mqtt_json_s.mqtt_fire_alarm=0;
                printf("fire close\n");
                if(work_status==WORK_FIRE)
                {
                    if(protect_status==PROTECT_ON)
                    {
                        work_status=WORK_PROTECT;
                    }
                    else if(protect_status==PROTECT_OFF)
                    {
                        work_status=WORK_HANDTOAUTO;//切自动
                    }
                    
                }
            }
        }
       
    }
}


void IRAM_ATTR gpio_isr_handler(void* arg) 
{
    //把中断消息插入到队列的后面，将gpio的io参数传递到队列中
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
   
}

void Fire_Init(void)
{
    //配置GPIO，下降沿和上升沿触发中断
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = 1 << GPIO_XF;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_set_intr_type(GPIO_XF, GPIO_INTR_ANYEDGE);
    
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    //注册中断服务分配资源号0
    gpio_install_isr_service(0);
    //设置GPIO的中断回调函数
    gpio_isr_handler_add(GPIO_XF, gpio_isr_handler,(void*) GPIO_XF);

    //建立任务
    xTaskCreate(Fire_interrupt_callBack //任务函数
            , "Fire_interrupt_callBack" //任务名字
            , 8192  //任务堆栈大小
            , NULL  //传递给任务函数的参数
            , 10   //任务优先级
            , NULL); //任務句柄


}


