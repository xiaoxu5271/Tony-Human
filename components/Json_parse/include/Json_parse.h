#ifndef __Json_parse
#define __Json_parse
#include <stdio.h>
#include "esp_err.h"
#include "E2prom.h"

esp_err_t parse_objects_http_active(char *http_json_data);
esp_err_t parse_objects_bluetooth(char *blu_json_data);
esp_err_t parse_objects_mqtt(char *mqtt_json_data, bool sw_flag);
esp_err_t parse_objects_heart(char *json_data);
esp_err_t parse_objects_http_respond(char *http_json_data);
esp_err_t ParseTcpUartCmd(char *pcCmdBuffer, ...);

void Create_NET_Json(void);
uint16_t Create_Status_Json(char *status_buff, uint16_t buff_len, bool filed_flag);

#define NET_WIFI 0 //上网模式 wifi
#define NET_LAN 1  //上网模式 网线

#define NET_AUTO 2 //上网模式 自动

struct
{
    char mqtt_command_id[32];
    char mqtt_Rssi[8];
    char mqtt_tem[8];       //温度
    char mqtt_hum[8];       //湿度
    char mqtt_ota_url[128]; //OTA升级地址

} mqtt_json_s;

struct
{
    char wifi_ssid[32];
    char wifi_pwd[64];
} wifi_data;

typedef struct
{
    char creat_json_buff[512];
    uint16_t creat_json_len;
} creat_json;

/************metadata 参数***********/
extern uint32_t fn_dp;        //数据发送频率
extern uint32_t fn_th;        //温湿度频率
extern uint32_t fn_sen;       //人感灵敏度
extern uint32_t fn_sen_cycle; //人感灵敏度 周期，单位ms
extern uint32_t fn_sen_res;   //人感状态重置周期，单位ms
extern uint32_t fn_sen_sta;   //人感原始值统计周期，单位s
extern uint8_t cg_data_led;   //发送数据 LED状态 0关闭，1打开
extern uint8_t net_mode;      //上网模式选择 0：自动模式 1：lan模式 2：wifi模式
/************************************/

extern char SerialNum[SERISE_NUM_LEN];
extern char ProductId[PRODUCT_ID_LEN];
extern char ApiKey[API_KEY_LEN];
extern char ChannelId[CHANNEL_ID_LEN];
extern char USER_ID[USER_ID_LEN];
extern char WEB_SERVER[WEB_HOST_LEN];
extern char MQTT_SERVER[WEB_HOST_LEN];
extern char WEB_PORT[5];
extern char MQTT_PORT[5];
extern char BleName[50];
extern char SIM_APN[32];
extern char SIM_USER[32];
extern char SIM_PWD[32];

void Read_Metadate_E2p(void);
void Read_Product_E2p(void);

#endif