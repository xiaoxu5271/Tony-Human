#ifndef __S_C
#define __S_C

#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"

void init_wifi(void);
void start_user_wifi(void);
void stop_user_wifi(void);
void Net_Switch(void);
void Scan_Wifi(void);
void start_softap(void);
bool Check_Wifi(uint8_t *ssid, int8_t *rssi);
int Tcp_Send(int sock, char *Send_Buff);

extern uint8_t wifi_connect_sta; //wifi连接状态
// extern uint8_t wifi_work_sta;    //wifi开启状态
extern uint8_t start_AP;
extern uint16_t Net_ErrCode; //
extern bool WIFI_STA;

EventGroupHandle_t Net_sta_group;
#define CONNECTED_BIT (1 << 0)    //网络连接
#define ACTIVED_BIT (1 << 1)      //激活
#define MQTT_W_S_BIT (1 << 2)     //WIFI MQTT 启动
#define MQTT_E_S_BIT (1 << 3)     //LAN MQTT 启动
#define MQTT_W_C_BIT (1 << 4)     //WIFI MQTT 连接
#define MQTT_E_C_BIT (1 << 5)     //LAN MQTT 连接
#define WIFI_S_BIT (1 << 6)       //wifi启动状态
#define WIFI_C_BIT (1 << 7)       //wifi连接状态
#define HUMAN_I_BIT (1 << 8)      //人感初始化完成标志
#define ETH_Task_BIT (1 << 10)    //ETH 初始化任务状态
#define ETH_M_INIT_BIT (1 << 11)  //ETH MQTT初始化状态
#define ETH_M_TASK_BIT (1 << 12)  //ETH mqtt接收任务
#define Uart1_Task_BIT (1 << 13)  //Uart1 接收任务
#define MQTT_INITED_BIT (1 << 14) //MQTT初始化完成
#define ACTIVE_S_BIT (1 << 15)    //激活中
#define TIME_CAL_BIT (1 << 16)    //时间校准成功
#define BLE_RESP_BIT (1 << 17)    //蓝牙超时回复标志
#define WIFI_I_BIT (1 << 18)      //wifi初始化状态

#define TCP_PORT 5001

// static const int CONNECTED_BIT = BIT0;
// static const int AP_STACONNECTED_BIT = BIT0;
// extern TaskHandle_t my_tcp_connect_Handle;

//server
//AP热点模式的配置信息
#define SOFT_AP_SSID "CloudForce-SP1" //账号
#define SOFT_AP_PAS ""                //密码，可以为空
#define SOFT_AP_MAX_CONNECT 2         //最多的连接点

#endif