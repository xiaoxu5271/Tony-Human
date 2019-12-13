#ifndef __MQ_TT
#define __MQ_TT

#include "mqtt_client.h"

extern char mqtt_pwd[45];
extern char mqtt_usr[24];
extern char mqtt_topic[100];
extern uint8_t wifi_mqtt_status;
extern uint8_t MQTT_INIT_STA;

void initialise_mqtt(void);
void stop_wifi_mqtt(void);
void start_wifi_mqtt(void);

#endif