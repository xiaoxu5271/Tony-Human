#ifndef __H_P
#define __H_P

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#define FIRMWARE "HUM1-0.2.43"

#define POST_NORMAL 0X00
#define POST_HEIGHT_ADD 0X01

#define POST_HEIGHT_SUB 0X02
#define POST_ANGLE_ADD 0X03
#define POST_ANGLE_SUB 0X04
#define POST_ALLDOWN 0X05
#define POST_ALLUP 0X06
#define POST_TARGET 0X07    //HTTP直接上传目标值，用于手动切自动的状态上传
#define POST_NOCOMMAND 0X08 //HTTP只上传平台，无commnd id

#define HTTP_STA_SERIAL_NUMBER 0x00
#define HTTP_KEY_GET 0x01
#define HTTP_KEY_NOT_GET 0x02

#define NOHUMAN 0x00
#define HAVEHUMAN 0x01

#define HTTP_RECV_BUFF_LEN 2048
#define MAX_DATA_BUFFRE_LEN 1024 * 10

typedef enum
{
    NET_OK = 0,
    NET_DIS,
    NET_READ,
    NET_410,
    NET_400,
    NET_402,
} Net_Err;

SemaphoreHandle_t xMutex_Http_Send;
SemaphoreHandle_t Cache_muxtex;

//需要发送的二值信号量
extern TaskHandle_t Binary_Heart_Send;
extern TaskHandle_t Binary_dp;
extern TaskHandle_t Binary_intr;
extern TaskHandle_t Active_Task_Handle;
esp_timer_handle_t http_timer_suspend_p;

extern uint8_t post_status;
extern uint8_t Last_Led_Status;

extern uint8_t Databuffer[MAX_DATA_BUFFRE_LEN];
extern uint32_t Databuffer_len;

void initialise_http(void);
void Start_Active(void);
void DataSave(uint8_t *sava_buff, uint16_t Buff_len);

#endif