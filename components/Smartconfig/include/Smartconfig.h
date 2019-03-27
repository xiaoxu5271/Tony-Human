#ifndef __S_C
#define __S_C

#include "freertos/event_groups.h"

void smartconfig_example_task(void *parm);
void initialise_wifi(char *wifi_ssid, char *wifi_password);
void init_wifi(void);
void reconnect_wifi_usr(void);
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

EventGroupHandle_t wifi_event_group;

#define WIFISTATUS_CONNET       0X01
#define WIFISTATUS_DISCONNET    0X00
uint8_t WifiStatus;


#endif