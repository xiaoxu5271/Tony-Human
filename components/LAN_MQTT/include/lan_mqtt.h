#ifndef __LAN_MQTT_H
#define __LAN_MQTT_H

// #include "My_Mqtt.h"
// #include "Bluetooth.h"

#if 1

// #define MQTT_PORT 1883
// #define MQTT_USER mqtt_usr    //
// #define MQTT_PASS mqtt_pwd    //
// #define MQTT_Topic mqtt_topic //
#define KEEPLIVE_TIME 6000                                //100为1s
#define MQTT_CLIEND_ID "d8034f7509c44389b30194d4c373f09c" //ID

#else
#define MQTT_SOCKET 2
#define MQTT_PORT 1883
#define MQTT_CLIEND_ID "d8034f7509c44389b30194d4c373f09c"    //ID
#define MQTT_DOMAIN "api.ubibot.cn"                          //MQTT
#define MQTT_USER "c_id=4777"                                //
#define MQTT_PASS "api_key=f2c931e60aec1a39338b4523c417eb54" //
#define MQTT_Topic "/product/28343913545840b3b9b42c568e78e243/channel/4777/control"

#endif

typedef void (*mqtt_connect_callback)(void);

void lan_mqtt_task(void *pvParameter);
void lan_mqtt_init(void);
void start_lan_mqtt(void);
void stop_lan_mqtt(void);
uint8_t mqtt_publish(char *Topic, char *msg, uint16_t len);

#endif /*__USER_MQTT_H*/
