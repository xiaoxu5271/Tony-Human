#ifndef __MQ_TT
#define __MQ_TT

#include "mqtt_client.h"

void initialise_mqtt(void);
void Start_W_Mqtt(void);
void Stop_W_Mqtt(void);
void W_Mqtt_Publish(char *msg);
// extern esp_mqtt_client_handle_t client;

QueueHandle_t Send_Mqtt_Queue;
#define MQTT_BUFF_LEN 521
typedef struct
{
    char buff[MQTT_BUFF_LEN]; //
    uint16_t buff_len;        //
} Mqtt_Msg;

extern bool MQTT_W_STA;
extern bool MQTT_E_STA;

char topic_s[100];
char topic_p[100];
char mqtt_pwd[42];
char mqtt_usr[23];
char mqtt_uri[64];

#endif