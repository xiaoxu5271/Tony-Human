#ifndef __Json_parse
#define __Json_parse
#include <stdio.h>
#include "esp_err.h"

esp_err_t parse_objects_http_active(char *http_json_data);
esp_err_t parse_objects_bluetooth(char *blu_json_data);
esp_err_t parse_objects_mqtt(char *json_data);
esp_err_t parse_objects_heart(char *json_data);
esp_err_t parse_Uart0(char *json_data);
esp_err_t parse_objects_http_respond(char *http_json_data);
esp_err_t ParseTcpUartCmd(char *pcCmdBuffer);

esp_err_t creat_object(void);

#define WORK_INIT 0X00       //初始化
#define WORK_AUTO 0x01       //平台自动模式
#define WORK_HAND 0x02       //网页版手动模式
#define WORK_HANDTOAUTO 0x03 //用于自动回复时执行一次自动控制指令
#define WORK_LOCAL 0x04      //本地计算控制模式
#define WORK_WAITLOCAL 0x05  //本地计算等待模式（用于状态机空闲状态）
#define WORK_WALLKEY 0X06    //本地墙壁开关控制模式
#define WORK_PROTECT 0X07    //风速和结霜保护
#define WORK_FIREINIT 0X08   //开机就火灾
#define WORK_FIRE 0x09       //火灾保护状态

#define NET_AUTO 0 //上网模式 自动
#define NET_LAN 1  //上网模式 网线
#define NET_WIFI 2 //上网模式 wifi

#define PROTECT_ON 0X01  //当前有风速等平台保护状态
#define PROTECT_OFF 0X00 //当前无风速等平台保护状态

#define MAX_AUTO_CTL_TIME (5 * 60) //5min
//#define MAX_WALLKEY_TIME    (4*60*60)  //4h
#define MAX_WALLKEY_TIME (5 * 60) //5min

struct
{
    int8_t mqtt_angle;
    char mqtt_angle_char[8];
    int8_t mqtt_height;
    char mqtt_height_char[8];
    char mqtt_mode[8];

    int8_t mqtt_angle_adj;  //用于手动控制解析改变角度取值-1/+1
    int8_t mqtt_height_adj; //用于手动控制解析改变高度取值-1/+1

    int mqtt_last;

    int mqtt_sun_condition;
    char mqtt_sun_condition_char[2];
    int mqtt_wind_protection;
    char mqtt_wind_protection_char[2];
    int mqtt_frost_protection;
    char mqtt_frost_protection_char[2];
    int mqtt_fire_alarm;
    char mqtt_fire_alarm_char[2];
    char mqtt_stage[8];
    char mqtt_command_id[32];
    char mqtt_string[256];
    char mqtt_Rssi[8];
    char mqtt_tem[8];       //温度
    char mqtt_hum[8];       //湿度
    char mqtt_ota_url[128]; //OTA升级地址

} mqtt_json_s;

struct
{
    char wifi_ssid[36];
    char wifi_pwd[36];
} wifi_data;

struct
{
    int http_angle;
    int http_height;
    int http_mode;
    int http_sun_condition;
    char http_time[24];
} http_json_c;

typedef struct
{
    char creat_json_b[512];
    int creat_json_c;
} creat_json;

typedef struct
{
    float lon;
    float lat;
    float orientation;
    int T1_h;
    int T1_m;
    int T2_h;
    int T2_m;
    int T3_h;
    int T3_m;
    int T4_h;
    int T4_m;
    uint8_t WallKeyId[4];
    int8_t Switch;
} object_bluetooth_json;

struct
{
    float lon;
    float lat;
    float orientation;
    int T1_h;
    int T1_m;
    int T2_h;
    int T2_m;
    int T3_h;
    int T3_m;
    int T4_h;
    int T4_m;
    uint8_t WallKeyId[4];
    int8_t Switch;
} ob_blu_json;

int read_bluetooth(void);
//creat_json *create_http_json(uint8_t post_status);
void create_http_json(uint8_t post_status, creat_json *pCreat_json);

uint8_t work_status;    //当前工作状态
uint8_t protect_status; //保护状态，用于火灾和风速混合保护的切换

/************metadata 参数***********/
extern unsigned long fn_dp; //数据发送频率
extern unsigned long fn_th; //温湿度频率
extern uint8_t cg_data_led; //发送数据 LED状态
extern uint8_t net_mode;    //上网模式选择 0：自动模式 1：lan模式 2：wifi模式
/************************************/

int auto_ctl_count; //自动控制指令计数，收到平台的自动控制指令后该变量清零，在定时器中每1s+1，加到180S（3min）后，进入本地计算

#endif