#ifndef __S_C
#define __S_C

#include "freertos/event_groups.h"
#include "freertos/task.h"

void smartconfig_example_task(void *parm);
void initialise_wifi(char *wifi_ssid, char *wifi_password);
void init_wifi(void);
void wifi_init_softap(void);
void wifi_init_apsta(void);
void reconnect_wifi_usr(void);

static const int CONNECTED_BIT = BIT0;
extern TaskHandle_t my_tcp_connect_Handle;
extern EventGroupHandle_t wifi_event_group;

#define WIFISTATUS_CONNET 0X01
#define WIFISTATUS_DISCONNET 0X00

//server
//AP热点模式的配置信息
#define SOFT_AP_SSID "HX-TCP-SERVER" //账号
#define SOFT_AP_PAS ""               //密码，可以为空
#define SOFT_AP_MAX_CONNECT 1        //最多的连接点

//client
//STA模式配置信息,即要连上的路由器的账号密码
#define GATEWAY_SSID "Massky_AP"  //账号
#define GATEWAY_PAS "ztl62066206" //密码

uint8_t WifiStatus;

#endif