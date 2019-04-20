#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "Switch.h"
#include "Http.h"
#include "Json_parse.h"
#include "Led.h"
#include "Bluetooth.h"

#define GPIO_KEY (GPIO_NUM_12)

static const char *TAG = "switch";
uint64_t key_timer_count = 0;

//定时器中断相关
void timer_switch_cb(void *arg);

esp_timer_handle_t timer_switch_handle = 0; //定时器句柄

esp_timer_create_args_t timer_switch_arg = {
    .callback =
        &timer_switch_cb,
    .arg = NULL,
    .name = "SwitchTimer"};

void timer_switch_cb(void *arg) //10ms中断一次
{

        key_timer_count++;
        ESP_LOGW(TAG, "按下计时 %llu\n", key_timer_count);

        if (key_timer_count >= 300) //检测长按超过1S
        {
                ESP_LOGW(TAG, "key long down!\n");
                Led_B_On();
                key_timer_count = 0;
                esp_timer_stop(timer_switch_handle);
        }
}

void Switch_interrupt_callBack(void *arg);
static xQueueHandle gpio_evt_queue = NULL; //定义一个队列返回变量

void Switch_interrupt_callBack(void *arg)
{
        uint32_t io_num;
        while (1)
        {
                //不断读取gpio队列，读取完后将删除队列
                if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) //队列阻塞等待
                {
                        // vTaskDelay(5 / portTICK_RATE_MS);
                        //ESP_LOGW(TAG, "key_interrupt,gpio[%d]=%d\n", io_num, gpio_get_level(io_num));
                        if (gpio_get_level(io_num) == 0) //按下
                        {
                                //ESP_LOGW(TAG, "key down!\n");
                                esp_err_t err = esp_timer_start_periodic(timer_switch_handle, 10000); //创建定时器，单位us，定时10ms
                                if (err != ESP_OK)
                                {
                                        printf("timer switch start err code:%d\n", err);
                                }

                                //http_send_mes();
                        }
                        else //抬起
                        {
                                esp_timer_stop(timer_switch_handle);
                                if (key_timer_count > 2) //短按
                                {
                                        key_timer_count = 0;
                                        ESP_LOGW(TAG, "key short down!\n");
                                }
                        }
                }
        }
}

void IRAM_ATTR gpio_isr_handler(void *arg)
{
        //把中断消息插入到队列的后面，将gpio的io参数传递到队列中
        uint32_t gpio_num = (uint32_t)arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void Switch_Init(void)
{
        //配置GPIO，边沿触发中断
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        io_conf.pin_bit_mask = 1ULL << GPIO_KEY;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);

        gpio_set_intr_type(GPIO_KEY, GPIO_INTR_ANYEDGE);

        gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
        //注册中断服务分配资源号0
        gpio_install_isr_service(0);
        //设置GPIO的中断回调函数
        gpio_isr_handler_add(GPIO_KEY, gpio_isr_handler, (void *)GPIO_KEY);

        //建立任务
        xTaskCreate(Switch_interrupt_callBack //任务函数
                    ,
                    "Switch_interrupt_callBack" //任务名字
                    ,
                    8192 //任务堆栈大小
                    ,
                    NULL //传递给任务函数的参数
                    ,
                    10 //任务优先级
                    ,
                    NULL); //任務句柄

        esp_err_t err = esp_timer_create(&timer_switch_arg, &timer_switch_handle); //创建定时器任务,但不开启
        if (err != ESP_OK)
        {
                printf("timer switch create err code:%d\n", err);
        }
}
