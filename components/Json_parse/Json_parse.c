#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include "esp_system.h"
#include "Json_parse.h"
#include "Nvs.h"
#include "ServerTimer.h"
#include "Http.h"

#include "esp_wifi.h"
#include "Smartconfig.h"
#include "E2prom.h"
#include "Http.h"
#include "Motorctl.h"
#include "Wind.h"
#include "Bluetooth.h"
#include "Human.h"
#include "sht30dis.h"

//wifi_config_t wifi_config;

//#define DEBUG_0

typedef enum
{
        HTTP_JSON_FORMAT_TINGERROR,
        HTTP_RESULT_UNSUCCESS,
        HTTP_OVER_PARSE,
        HTTP_WRITE_FLASH_OVER,
        CREAT_OBJECT_OVER,
} http_error_info;

extern uint32_t HTTP_STATUS;
char get_num[2];
char *key;
int n = 0, i;

int read_bluetooth(void)
{
        uint8_t bluetooth_sta[256];

        E2prom_BluRead(0x00, bluetooth_sta, 256);
        printf("bluetooth_sta=%s\n", bluetooth_sta);
        if (strlen((char *)bluetooth_sta) == 0) //未读到蓝牙配置信息
        {
                return 0;
        }
        uint8_t ret = parse_objects_bluetooth((char *)bluetooth_sta);
        if ((ret == BLU_PWD_REFUSE) || (ret == BLU_JSON_FORMAT_ERROR))
        {
                return 0;
        }
        return 1;
}

esp_err_t parse_objects_bluetooth(char *blu_json_data)
{
        cJSON *cjson_blu_data_parse = NULL;
        cJSON *cjson_blu_data_parse_wifissid = NULL;
        cJSON *cjson_blu_data_parse_wifipwd = NULL;
        cJSON *cjson_blu_data_parse_ob = NULL;
        cJSON *cjson_blu_data_parse_devicepwd = NULL;

        printf("start_ble_parse_json\r\n");
        if (blu_json_data[0] != '{')
        {
                printf("blu_json_data Json Formatting error\n");

                return 0;
        }

        //数据包包含NULL则直接返回error
        if (strstr(blu_json_data, "null") != NULL) //需要解析的字符串内含有null
        {
                printf("there is null in blu data\r\n");
                return BLU_PWD_REFUSE;
        }

        cjson_blu_data_parse = cJSON_Parse(blu_json_data);
        if (cjson_blu_data_parse == NULL) //如果数据包不为JSON则退出
        {
                printf("Json Formatting error2\n");
                cJSON_Delete(cjson_blu_data_parse);
                return BLU_JSON_FORMAT_ERROR;
        }
        else
        {
                cjson_blu_data_parse_devicepwd = cJSON_GetObjectItem(cjson_blu_data_parse, "devicePassword");
                //if(cjson_blu_data_parse_ob->valuedouble==0)
                if (cjson_blu_data_parse_devicepwd == NULL)
                {
                        printf("ble devpwd err1=%s\r\n", cjson_blu_data_parse_devicepwd->valuestring);
                        cJSON_Delete(cjson_blu_data_parse);
                        return BLU_PWD_REFUSE;
                }
                else if (strcmp(cjson_blu_data_parse_devicepwd->valuestring, ble_dev_pwd) != 0) //校验密码
                {
                        printf("ble devpwd err2=%s\r\n", cjson_blu_data_parse_devicepwd->valuestring);
                        cJSON_Delete(cjson_blu_data_parse);
                        return BLU_PWD_REFUSE;
                }
                else
                {
                        printf("ble devpwd ok=%s\r\n", cjson_blu_data_parse_devicepwd->valuestring);
                }

                cjson_blu_data_parse_wifissid = cJSON_GetObjectItem(cjson_blu_data_parse, "wifiSSID");
                if (cjson_blu_data_parse_wifissid == NULL)
                {
                        return BLU_NO_WIFI_SSID;
                }
                else
                {
                        bzero(wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
                        strcpy(wifi_data.wifi_ssid, cjson_blu_data_parse_wifissid->valuestring);
                        printf("%s\r\n", cjson_blu_data_parse_wifissid->valuestring);
                }

                cjson_blu_data_parse_wifipwd = cJSON_GetObjectItem(cjson_blu_data_parse, "wifiPwd");
                if (cjson_blu_data_parse_wifipwd == NULL)
                {
                        return BLU_NO_WIFI_PWD;
                }
                else
                {
                        bzero(wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                        strcpy(wifi_data.wifi_pwd, cjson_blu_data_parse_wifipwd->valuestring);
                        printf("%s\r\n", cjson_blu_data_parse_wifipwd->valuestring);
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "latitude")) != NULL)
                {
                        printf("latitude %f\r\n", cjson_blu_data_parse_ob->valuedouble);
                        ob_blu_json.lat = cjson_blu_data_parse_ob->valuedouble;
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "longitude")) != NULL)
                {
                        printf("longitude %f\r\n", cjson_blu_data_parse_ob->valuedouble);
                        ob_blu_json.lon = cjson_blu_data_parse_ob->valuedouble;
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "orientation")) != NULL)
                {
                        printf("orientation %f\r\n", cjson_blu_data_parse_ob->valuedouble);
                        ob_blu_json.orientation = cjson_blu_data_parse_ob->valuedouble;
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "s1")) != NULL)
                {
                        printf("s1 %s\r\n", cjson_blu_data_parse_ob->valuestring);
                        sscanf(cjson_blu_data_parse_ob->valuestring, "%2s", get_num);
                        ob_blu_json.T1_h = atoi(get_num);
                        ob_blu_json.T1_m = atoi((cjson_blu_data_parse_ob->valuestring) + 3);
                        printf("T1 %d %d \r\n", ob_blu_json.T1_h, ob_blu_json.T1_m);
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "e1")) != NULL)
                {
                        printf("e1 %s\r\n", cjson_blu_data_parse_ob->valuestring);
                        sscanf(cjson_blu_data_parse_ob->valuestring, "%2s", get_num);
                        ob_blu_json.T4_h = atoi(get_num);
                        ob_blu_json.T4_m = atoi((cjson_blu_data_parse_ob->valuestring) + 3);
                        printf("T4 %d %d \r\n", ob_blu_json.T4_h, ob_blu_json.T4_m);
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "s2")) != NULL)
                {
                        printf("s2 %s\r\n", cjson_blu_data_parse_ob->valuestring);
                        sscanf(cjson_blu_data_parse_ob->valuestring, "%2s", get_num);
                        ob_blu_json.T2_h = atoi(get_num);
                        ob_blu_json.T2_m = atoi((cjson_blu_data_parse_ob->valuestring) + 3);
                        printf("T2 %d %d \r\n", ob_blu_json.T2_h, ob_blu_json.T2_m);
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "e2")) != NULL)
                {
                        printf("e2 %s\r\n", cjson_blu_data_parse_ob->valuestring);
                        sscanf(cjson_blu_data_parse_ob->valuestring, "%2s", get_num);
                        ob_blu_json.T3_h = atoi(get_num);
                        ob_blu_json.T3_m = atoi((cjson_blu_data_parse_ob->valuestring) + 3);
                        printf("T3 %d %d \r\n", ob_blu_json.T3_h, ob_blu_json.T3_m);
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "serial")) != NULL)
                {
                        printf("serial %s\r\n", cjson_blu_data_parse_ob->valuestring);
                        n = strlen(cjson_blu_data_parse_ob->valuestring) / 2;
                        key = (char *)malloc(sizeof(char) * n);
                        for (i = 0; i < n; ++i)
                        {
                                sscanf(cjson_blu_data_parse_ob->valuestring + 2 * i, "%2hhX", key + i);
                        }
                        for (i = 0; i < n; ++i)
                        {
                                ob_blu_json.WallKeyId[i] = key[i];
                                printf("0x%02X ", key[i]);
                        }
                        free(key);
                        printf("\r\n");
                }
                if ((cjson_blu_data_parse_ob = cJSON_GetObjectItem(cjson_blu_data_parse, "switch")) != NULL)
                {
                        printf("switch %d\r\n", cjson_blu_data_parse_ob->valueint);
                        ob_blu_json.Switch = cjson_blu_data_parse_ob->valueint;
                }
        }
        //重新初始化WIFI
        initialise_wifi(cjson_blu_data_parse_wifissid->valuestring, cjson_blu_data_parse_wifipwd->valuestring);
        cJSON_Delete(cjson_blu_data_parse);
        return BLU_RESULT_SUCCESS;
}

//解析激活返回数据
esp_err_t parse_objects_http_active(char *http_json_data)
{
        cJSON *json_data_parse = NULL;
        cJSON *json_data_parse_value = NULL;
        cJSON *json_data_parse_time_value = NULL;
        cJSON *json_data_parse_channel_channel_write_key = NULL;
        cJSON *json_data_parse_channel_channel_id_value = NULL;
        //cJSON *json_data_parse_command_value = NULL;
        cJSON *json_data_parse_channel_value = NULL;
        //char *json_print;

        printf("start_parse_active_http_json\r\n");

        if (http_json_data[0] != '{')
        {
                printf("http_json_data Json Formatting error\n");

                return 0;
        }

        json_data_parse = cJSON_Parse(http_json_data);
        if (json_data_parse == NULL)
        {

                printf("Json Formatting error3\n");

                cJSON_Delete(json_data_parse);
                return 0;
        }
        else
        {

                json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "result");
                if (!(strcmp(json_data_parse_value->valuestring, "success")))
                {
                        printf("active:success\r\n");

                        json_data_parse_time_value = cJSON_GetObjectItem(json_data_parse, "server_time");
                        Server_Timer_GET(json_data_parse_time_value->valuestring);
                }
                else
                {
                        printf("active:error\r\n");
                        cJSON_Delete(json_data_parse);
                        return 0;
                }

                if (cJSON_GetObjectItem(json_data_parse, "channel") != NULL)
                {
                        json_data_parse_channel_value = cJSON_GetObjectItem(json_data_parse, "channel");

                        //printf("%s\r\n", cJSON_Print(json_data_parse_channel_value));

                        json_data_parse_channel_channel_write_key = cJSON_GetObjectItem(json_data_parse_channel_value, "write_key");
                        json_data_parse_channel_channel_id_value = cJSON_GetObjectItem(json_data_parse_channel_value, "channel_id");

                        //printf("api key=%s\r\n", json_data_parse_channel_channel_write_key->valuestring);
                        //printf("channel_id=%s\r\n", json_data_parse_channel_channel_id_value->valuestring);

                        //写入API-KEY
                        sprintf(ApiKey, "%s%c", json_data_parse_channel_channel_write_key->valuestring, '\0');
                        //printf("api key=%s\r\n", ApiKey);
                        E2prom_Write(0x00, (uint8_t *)ApiKey, 32);

                        //写入channelid
                        sprintf(ChannelId, "%s%c", json_data_parse_channel_channel_id_value->valuestring, '\0');
                        //printf("channel_id=%s\r\n", ChannelId);
                        E2prom_Write(0x20, (uint8_t *)ChannelId, strlen(ChannelId));
                }
        }
        cJSON_Delete(json_data_parse);
        return 1;
}

//解析http-post返回数据
esp_err_t parse_objects_http_respond(char *http_json_data)
{
        cJSON *json_data_parse = NULL;
        cJSON *json_data_parse_value = NULL;
        cJSON *json_data_parse_errorcode = NULL;

        if (http_json_data[0] != '{')
        {
                printf("http_respond_json_data Json Formatting error\n");
                return 0;
        }

        json_data_parse = cJSON_Parse(http_json_data);
        if (json_data_parse == NULL)
        {

                printf("Json Formatting error3\n");
                cJSON_Delete(json_data_parse);
                return 0;
        }
        else
        {

                json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "result");
                if (!(strcmp(json_data_parse_value->valuestring, "error")))
                {
                        json_data_parse_errorcode = cJSON_GetObjectItem(json_data_parse, "errorCode");
                        printf("post send error_code=%s\n", json_data_parse_errorcode->valuestring);
                        if (!(strcmp(json_data_parse_errorcode->valuestring, "invalid_channel_id"))) //设备空间ID被删除或API——KEY错误，需要重新激活
                        {
                                //清空API-KEY存储，激活后获取
                                uint8_t data_write2[33] = "\0";
                                E2prom_Write(0x00, data_write2, 32);

                                //清空channelid，激活后获取
                                uint8_t data_write3[16] = "\0";
                                ;
                                E2prom_Write(0x20, data_write3, 16);

                                fflush(stdout); //使stdout清空，就会立刻输出所有在缓冲区的内容。
                                esp_restart();  //芯片复位 函数位于esp_system.h
                        }
                }
        }
        cJSON_Delete(json_data_parse);
        return 1;
}

esp_err_t parse_Uart0(char *json_data)
{
        cJSON *json_data_parse = NULL;
        cJSON *json_data_parse_ProductID = NULL;
        cJSON *json_data_parse_SeriesNumber = NULL;

        //if(strstr(json_data,"{")==NULL)
        if (json_data[0] != '{')
        {
                printf("uart0 Json Formatting error1\n");
                return 0;
        }

        json_data_parse = cJSON_Parse(json_data);
        if (json_data_parse == NULL) //如果数据包不为JSON则退出
        {
                printf("uart0 Json Formatting error\n");
                cJSON_Delete(json_data_parse);

                return 0;
        }
        else
        {
                json_data_parse_ProductID = cJSON_GetObjectItem(json_data_parse, "ProductID");
                printf("ProductID= %s\n", json_data_parse_ProductID->valuestring);
                json_data_parse_SeriesNumber = cJSON_GetObjectItem(json_data_parse, "SeriesNumber");
                printf("SeriesNumber= %s\n", json_data_parse_SeriesNumber->valuestring);

                sprintf(ProductId, "%s%c", json_data_parse_ProductID->valuestring, '\0');
                E2prom_Write(0x40, (uint8_t *)ProductId, strlen(ProductId));

                sprintf(SerialNum, "%s%c", json_data_parse_SeriesNumber->valuestring, '\0');
                E2prom_Write(0x30, (uint8_t *)SerialNum, strlen(SerialNum));

                //清空API-KEY存储，激活后获取
                uint8_t data_write2[33] = "\0";
                E2prom_Write(0x00, data_write2, 32);

                //清空channelid，激活后获取
                uint8_t data_write3[16] = "\0";
                E2prom_Write(0x20, data_write3, 16);

                uint8_t zerobuf[256] = "\0";
                E2prom_BluWrite(0x00, (uint8_t *)zerobuf, 256); //清空蓝牙

                //E2prom_Read(0x30,(uint8_t *)SerialNum,16);
                //printf("read SerialNum=%s\n", SerialNum);

                //E2prom_Read(0x40,(uint8_t *)ProductId,32);
                //printf("read ProductId=%s\n", ProductId);

                printf("{\"status\":0,\"code\": 0}");
                cJSON_Delete(json_data_parse);
                fflush(stdout); //使stdout清空，就会立刻输出所有在缓冲区的内容。
                esp_restart();  //芯片复位 函数位于esp_system.h
                return 1;
        }
}

esp_err_t parse_objects_heart(char *json_data)
{
        cJSON *json_data_parse = NULL;
        cJSON *json_data_parse_value = NULL;
        char *json_print;
        json_data_parse = cJSON_Parse(json_data);

        if (json_data[0] != '{')
        {
                printf("heart Json Formatting error\n");

                return 0;
        }

        if (json_data_parse == NULL) //如果数据包不为JSON则退出
        {

                printf("Json Formatting error4\n");

                cJSON_Delete(json_data_parse);
                return 0;
        }
        else
        {
                json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "server_time");
                Server_Timer_GET(json_data_parse_value->valuestring);
                json_print = cJSON_Print(json_data_parse_value);
                printf("json_data_parse_value %s\n", json_print);
        }
        free(json_print);
        cJSON_Delete(json_data_parse);

        return 1;
}

esp_err_t parse_objects_mqtt(char *mqtt_json_data)
{
        cJSON *json_data_parse = NULL;
        cJSON *json_data_angle_parse = NULL;
        cJSON *json_data_height_parse = NULL;
        cJSON *json_data_mode_parse = NULL;
        cJSON *json_data_sun_condition_parse = NULL;
        cJSON *json_data_string_parse = NULL;
        cJSON *json_data_command_id_parse = NULL;
        cJSON *json_data_ctr_parse = NULL;

        json_data_parse = cJSON_Parse(mqtt_json_data);
        //printf("%s", cJSON_Print(json_data_parse));

        if (mqtt_json_data[0] != '{')
        {
                printf("mqtt_json_data Json Formatting error\n");

                return 0;
        }

        if (json_data_parse == NULL) //如果数据包不为JSON则退出
        {

                printf("Json Formatting error5\n");

                cJSON_Delete(json_data_parse);
                return 0;
        }

        json_data_string_parse = cJSON_GetObjectItem(json_data_parse, "command_string");
        if (json_data_string_parse != NULL)
        {
                json_data_command_id_parse = cJSON_GetObjectItem(json_data_parse, "command_id");
                strncpy(mqtt_json_s.mqtt_command_id, json_data_command_id_parse->valuestring, strlen(json_data_command_id_parse->valuestring));
                strncpy(mqtt_json_s.mqtt_string, json_data_string_parse->valuestring, strlen(json_data_string_parse->valuestring));

                //printf("%s\r\n", cJSON_Print(json_data_string_parse));
                json_data_string_parse = cJSON_Parse(json_data_string_parse->valuestring);
                if (json_data_string_parse != NULL)
                {
                        //printf("%s\r\n", cJSON_Print(json_data_string_parse));

                        //收到自动模式指令或单独发送解除风速、结霜报警指令
                        if ((json_data_mode_parse = cJSON_GetObjectItem(json_data_string_parse, "mode")) != NULL || (json_data_mode_parse = cJSON_GetObjectItem(json_data_string_parse, "frost_protection")) != NULL || (json_data_mode_parse = cJSON_GetObjectItem(json_data_string_parse, "wind_protection")) != NULL)
                        {
                                auto_ctl_count = 0;
                                if ((json_data_angle_parse = cJSON_GetObjectItem(json_data_string_parse, "angle")) != NULL)
                                {
                                        mqtt_json_s.mqtt_angle = json_data_angle_parse->valueint;
                                        printf("angle %d \r\n", mqtt_json_s.mqtt_angle);
                                }
                                if ((json_data_height_parse = cJSON_GetObjectItem(json_data_string_parse, "height")) != NULL)
                                {
                                        mqtt_json_s.mqtt_height = json_data_height_parse->valueint;
                                        printf("height %d \r\n", mqtt_json_s.mqtt_height);
                                }

                                if ((json_data_sun_condition_parse = cJSON_GetObjectItem(json_data_string_parse, "sun_condition")) != NULL)
                                {
                                        mqtt_json_s.mqtt_sun_condition = json_data_sun_condition_parse->valueint;
                                        printf("sun_condition %d \r\n", mqtt_json_s.mqtt_sun_condition);
                                }
                                //收到风特殊保护状态时，则立即进入保护状态
                                if ((json_data_sun_condition_parse = cJSON_GetObjectItem(json_data_string_parse, "wind_protection")) != NULL)
                                {
                                        mqtt_json_s.mqtt_wind_protection = json_data_sun_condition_parse->valueint;
                                        printf("mqtt_wind_protection %d \r\n", mqtt_json_s.mqtt_wind_protection);
                                }
                                //收到霜特殊保护状态时，则立即进入保护状态
                                if ((json_data_sun_condition_parse = cJSON_GetObjectItem(json_data_string_parse, "frost_protection")) != NULL)
                                {
                                        mqtt_json_s.mqtt_frost_protection = json_data_sun_condition_parse->valueint;
                                        printf("mqtt_frost_protection %d \r\n", mqtt_json_s.mqtt_frost_protection);
                                }

                                if ((mqtt_json_s.mqtt_wind_protection == 1) || (mqtt_json_s.mqtt_frost_protection == 1))
                                {
                                        protect_status = PROTECT_ON;  //开启平台保护状态，用于火灾解除时的判断
                                        if (work_status != WORK_FIRE) //在不是火警的情况下切保护
                                        {
                                                printf("WORK_PROTECT!!!\n");
                                                work_status = WORK_PROTECT;
                                        }
                                }
                                //当收到解除风速、霜保护时，切换为自动控制
                                else if ((mqtt_json_s.mqtt_wind_protection == 0) && (mqtt_json_s.mqtt_frost_protection == 0) && (mqtt_json_s.mqtt_fire_alarm == 0))
                                {
                                        protect_status = PROTECT_OFF; //关闭平台保护状态，用于火灾解除时的判断
                                        if (work_status == WORK_PROTECT)
                                        {
                                                printf("REMOVE_WORK_PROTECT!!!\n");
                                                work_status = WORK_AUTO;
                                                http_send_mes(POST_NORMAL);
                                        }
                                }

                                /*if ((json_data_sun_condition_parse = cJSON_GetObjectItem(json_data_string_parse, "fire_alarm")) != NULL)
                {
                    mqtt_json_s.mqtt_fire_alarm = json_data_sun_condition_parse->valueint;
                    printf("mqtt_fire_alarm %d \r\n", mqtt_json_s.mqtt_fire_alarm);
                }*/

                                if ((json_data_mode_parse = cJSON_GetObjectItem(json_data_string_parse, "mode")) != NULL)
                                {
                                        if (strncmp("auto", json_data_mode_parse->valuestring, strlen("auto")) == 0)
                                        {
                                                printf("!!!!!!!!!!RECV AUTO CTL VALUE!!!!!!!!!!!!!!!!\n");

                                                //判断当前如果是手动模式、墙壁开关控制模式时则记录该值，自动模式时则执行
                                                if ((work_status == WORK_HAND) || (work_status == WORK_WALLKEY))
                                                {
                                                        printf("now is hand_ctl,auto_height=%d,auto_angle=%d\n", mqtt_json_s.mqtt_height, mqtt_json_s.mqtt_angle);
                                                }
                                                //当前是自动状态或本地计算状态则切换自动状态
                                                else if ((work_status == WORK_AUTO) || (work_status == WORK_WAITLOCAL))
                                                {
                                                        work_status = WORK_AUTO;
                                                        strcpy(mqtt_json_s.mqtt_mode, "1");
                                                        Motor_AutoCtl((int)mqtt_json_s.mqtt_height, (int)mqtt_json_s.mqtt_angle);
                                                        //printf("send http\r\n");
                                                        http_send_mes(POST_NORMAL);
                                                }
                                                //当前是需要保护状态立即发送一个全收指令更新平台
                                                else if (work_status == WORK_PROTECT)
                                                {
                                                        strcpy(mqtt_json_s.mqtt_mode, "1");
                                                        http_send_mes(POST_ALLUP);
                                                        Motor_AutoCtl(0, 0);
                                                        //Motor_AutoCtl((int)mqtt_json_s.mqtt_height, (int)mqtt_json_s.mqtt_angle);
                                                }
                                        }
                                }
                        }

                        /********手动控制指令************************************************************************************************/
                        if ((json_data_ctr_parse = cJSON_GetObjectItem(json_data_string_parse, "r")) != NULL) //手动角调整
                        {
                                if ((work_status != WORK_FIRE) && (work_status != WORK_PROTECT))
                                {
                                        if ((json_data_angle_parse = cJSON_GetObjectItem(json_data_string_parse, "last")) != NULL)
                                        {
                                                mqtt_json_s.mqtt_last = json_data_angle_parse->valueint;
                                                printf("mqtt_last=%d\n", mqtt_json_s.mqtt_last);
                                        }
                                        work_status = WORK_HAND;
                                        strcpy(mqtt_json_s.mqtt_mode, "0"); //json上传模式改为0手动
                                        mqtt_json_s.mqtt_angle_adj = json_data_ctr_parse->valueint;
                                        if (mqtt_json_s.mqtt_angle_adj == 1)
                                        {
                                                //printf("send http\r\n");
                                                http_send_mes(POST_ANGLE_ADD);
                                                Motor_HandCtl_Angle(ADD);
                                        }
                                        else if (mqtt_json_s.mqtt_angle_adj == -1)
                                        {
                                                //printf("send http\r\n");
                                                http_send_mes(POST_ANGLE_SUB);
                                                Motor_HandCtl_Angle(SUB);
                                        }
                                        printf("mqtt_json_s.mqtt_angle_adj = %d \r\n", mqtt_json_s.mqtt_angle_adj);
                                }
                        }
                        if ((json_data_ctr_parse = cJSON_GetObjectItem(json_data_string_parse, "h")) != NULL) //手动高调整
                        {
                                if ((work_status != WORK_FIRE) && (work_status != WORK_PROTECT))
                                {
                                        if ((json_data_angle_parse = cJSON_GetObjectItem(json_data_string_parse, "last")) != NULL)
                                        {
                                                mqtt_json_s.mqtt_last = json_data_angle_parse->valueint;
                                                printf("mqtt_last=%d\n", mqtt_json_s.mqtt_last);
                                        }
                                        work_status = WORK_HAND;
                                        strcpy(mqtt_json_s.mqtt_mode, "0"); //json上传模式改为0手动
                                        mqtt_json_s.mqtt_height_adj = json_data_ctr_parse->valueint;
                                        if (mqtt_json_s.mqtt_height_adj == 1)
                                        {
                                                http_send_mes(POST_HEIGHT_ADD);
                                                Motor_HandCtl_Height(ADD);
                                        }
                                        else if (mqtt_json_s.mqtt_height_adj == -1)
                                        {
                                                http_send_mes(POST_HEIGHT_SUB);
                                                Motor_HandCtl_Height(SUB);
                                        }
                                        printf("mqtt_json_s.mqtt_height_adj = %d\r\n", mqtt_json_s.mqtt_height_adj);
                                }
                        }
                        if ((json_data_ctr_parse = cJSON_GetObjectItem(json_data_string_parse, "s")) != NULL) //手动全收全放
                        {
                                if ((work_status != WORK_FIRE) && (work_status != WORK_PROTECT))
                                {
                                        if ((json_data_angle_parse = cJSON_GetObjectItem(json_data_string_parse, "last")) != NULL)
                                        {
                                                mqtt_json_s.mqtt_last = json_data_angle_parse->valueint;
                                                printf("mqtt_last=%d\n", mqtt_json_s.mqtt_last);
                                        }
                                        work_status = WORK_HAND;
                                        strcpy(mqtt_json_s.mqtt_mode, "0"); //json上传模式改为0手动
                                        if (json_data_ctr_parse->valueint == 1)
                                        {
                                                http_send_mes(POST_ALLUP);
                                                Motor_AutoCtl(0, 0);
                                        }
                                        else if (json_data_ctr_parse->valueint == -1)
                                        {
                                                http_send_mes(POST_ALLDOWN);
                                                Motor_AutoCtl(100, 80);
                                        }
                                }
                        }
                        if ((json_data_ctr_parse = cJSON_GetObjectItem(json_data_string_parse, "c")) != NULL) //指令手动切自动
                        {
                                if (strcmp(json_data_ctr_parse->valuestring, "a") == 0) //收到控制台切换auto指令
                                {
                                        work_status = WORK_AUTO;
                                        strcpy(mqtt_json_s.mqtt_mode, "1");
                                        http_send_mes(POST_TARGET); //直接发送目标结果
                                        printf("hand to auto,auto_height=%d,auto_angle=%d\n", mqtt_json_s.mqtt_height, mqtt_json_s.mqtt_angle);
                                        if ((mqtt_json_s.mqtt_height == 0) && (mqtt_json_s.mqtt_angle == 0)) //目标是0.0直接到0.0
                                        {
                                                Motor_AutoCtl((int)mqtt_json_s.mqtt_height, (int)mqtt_json_s.mqtt_angle); //转自动控制
                                        }
                                        else
                                        {
                                                Motor_SetAllDown();                                                       //先清零
                                                Motor_AutoCtl((int)mqtt_json_s.mqtt_height, (int)mqtt_json_s.mqtt_angle); //转自动控制
                                        }
                                }
                                if (strcmp(json_data_ctr_parse->valuestring, "m") == 0) //收到控制台手动控制指令
                                {
                                        if ((work_status != WORK_FIRE) && (work_status != WORK_PROTECT))
                                        {
                                                if ((json_data_angle_parse = cJSON_GetObjectItem(json_data_string_parse, "last")) != NULL)
                                                {
                                                        mqtt_json_s.mqtt_last = json_data_angle_parse->valueint;
                                                        printf("mqtt_last=%d\n", mqtt_json_s.mqtt_last);
                                                }
                                                work_status = WORK_HAND;
                                                strcpy(mqtt_json_s.mqtt_mode, "0"); //json上传模式改为0手动
                                                http_send_mes(POST_NORMAL);
                                        }
                                }
                        }
                }
                else
                {
                        printf("Json Formatting error6\n");
                        cJSON_Delete(json_data_parse);
                        cJSON_Delete(json_data_string_parse);
                        return 0;
                }
                cJSON_Delete(json_data_parse);
                cJSON_Delete(json_data_string_parse);
        }
        return 1;
}

void create_http_json(uint8_t post_status, creat_json *pCreat_json)
{
        printf("INTO CREATE_HTTP_JSON\r\n");
        //creat_json *pCreat_json = malloc(sizeof(creat_json));
        cJSON *root = cJSON_CreateObject();
        cJSON *item = cJSON_CreateObject();
        cJSON *next = cJSON_CreateObject();
        cJSON *fe_body = cJSON_CreateArray();
        //char status_creat_json_c[256];
        char status_creat_json[128];
        char status_cj[4];

        //printf("Server_Timer_SEND() %s", (char *)Server_Timer_SEND());
        //strncpy(http_json_c.http_time[20], Server_Timer_SEND(), 20);
        // printf("this http_json_c.http_time[20]  %s\r\n", http_json_c.http_time);
        strncpy(http_json_c.http_time, Server_Timer_SEND(), 24);
        wifi_ap_record_t wifidata;
        if (esp_wifi_sta_get_ap_info(&wifidata) == 0)
        {
                itoa(wifidata.rssi, mqtt_json_s.mqtt_Rssi, 10);
        }
        int8_t mqtt_height1 = -1;
        int8_t mqtt_angle1 = -1;
        Motor_Value_Query(&mqtt_height1, &mqtt_angle1);
        printf("Motor_Value_Query,mqtt_angle1=%d,mqtt_height1=%d\r\n", mqtt_angle1, mqtt_height1);

        if (post_status == POST_ANGLE_ADD)
        {
                if (mqtt_angle1 < 80)
                {
                        mqtt_angle1 += 10;
                }
        }
        else if (post_status == POST_ANGLE_SUB)
        {
                if (mqtt_angle1 > 0)
                {
                        mqtt_angle1 -= 10;
                }
        }
        else if (post_status == POST_HEIGHT_ADD)
        {
                if (mqtt_height1 < 100)
                {
                        mqtt_height1 += 10;
                        mqtt_angle1 = 80;
                }
        }
        else if (post_status == POST_HEIGHT_SUB)
        {
                if (mqtt_height1 > 0)
                {
                        mqtt_height1 -= 10;
                        mqtt_angle1 = 0;
                }
        }
        else if (post_status == POST_ALLDOWN)
        {
                mqtt_height1 = 100;
                mqtt_angle1 = 80;
        }
        else if (post_status == POST_ALLUP)
        {
                mqtt_height1 = 0;
                mqtt_angle1 = 0;
        }
        else if (post_status == POST_TARGET)
        {
                mqtt_height1 = mqtt_json_s.mqtt_height;
                mqtt_angle1 = mqtt_json_s.mqtt_angle;
        }

        itoa(mqtt_height1, mqtt_json_s.mqtt_height_char, 10);
        itoa(mqtt_angle1, mqtt_json_s.mqtt_angle_char, 10);
        itoa(mqtt_json_s.mqtt_wind_protection, mqtt_json_s.mqtt_wind_protection_char, 10);
        itoa(mqtt_json_s.mqtt_sun_condition, mqtt_json_s.mqtt_sun_condition_char, 10);
        itoa(mqtt_json_s.mqtt_frost_protection, mqtt_json_s.mqtt_frost_protection_char, 10);
        itoa(mqtt_json_s.mqtt_fire_alarm, mqtt_json_s.mqtt_fire_alarm_char, 10);
        // sprintf(status_creat_json, "%s%s%s%s%s%s%s%s%s%s%s", "{\"height\":", mqtt_height1 + ",", "\"angle\":", mqtt_angle1 + ",", "\"auto\":0,", "\"wind_protection\":", ",", "\"sun_condition\":", ",", "\"frost_protection\":", "}");
        sprintf(status_cj, "%s", "\"");
        sprintf(status_creat_json, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "{",
                status_cj, "height", status_cj, ":", mqtt_json_s.mqtt_height_char, ",",
                status_cj, "angel", status_cj, ":", mqtt_json_s.mqtt_angle_char, ",",
                status_cj, "auto", status_cj, ":", mqtt_json_s.mqtt_mode, ",",
                status_cj, "wind_protection", status_cj, ":", mqtt_json_s.mqtt_wind_protection_char, ",",
                status_cj, "sun_condition", status_cj, ":", mqtt_json_s.mqtt_sun_condition_char, ",",
                status_cj, "frost_protection", status_cj, ":", mqtt_json_s.mqtt_frost_protection_char, ",",
                status_cj, "fire_alarm", status_cj, ":", mqtt_json_s.mqtt_fire_alarm_char, "}");

        sprintf(mqtt_json_s.mqtt_tem, "%4.2f", tem);
        sprintf(mqtt_json_s.mqtt_hum, "%4.2f", hum);

        /*if(post_status==POST_NOCOMMAND)
    {
        cJSON_AddItemToObject(root, "feeds", fe_body);
        cJSON_AddItemToArray(fe_body, item);
        cJSON_AddItemToObject(item, "created_at", cJSON_CreateString(http_json_c.http_time));
        cJSON_AddItemToObject(item, "field1", cJSON_CreateString(mqtt_json_s.mqtt_height_char)); //高度
        cJSON_AddItemToObject(item, "field2", cJSON_CreateString(mqtt_json_s.mqtt_angle_char));  //角度
        cJSON_AddItemToObject(item, "field3", cJSON_CreateString(mqtt_json_s.mqtt_mode));        //模式
        cJSON_AddItemToObject(item, "field4", cJSON_CreateString(wind_Read_c));                  //风速
        cJSON_AddItemToObject(item, "field5", cJSON_CreateString(mqtt_json_s.mqtt_Rssi));        //WIFI RSSI       
    }
    else*/
        {
                //printf("status_creat_json %s\r\n", status_creat_json);
                cJSON_AddItemToObject(root, "feeds", fe_body);
                cJSON_AddItemToArray(fe_body, item);
                cJSON_AddItemToObject(item, "created_at", cJSON_CreateString(http_json_c.http_time));
                cJSON_AddItemToObject(item, "status", cJSON_CreateString(status_creat_json));
                cJSON_AddItemToArray(fe_body, next);
                cJSON_AddItemToObject(next, "created_at", cJSON_CreateString(http_json_c.http_time));
                cJSON_AddItemToObject(next, "field1", cJSON_CreateString(mqtt_json_s.mqtt_tem)); //温度
                cJSON_AddItemToObject(next, "field2", cJSON_CreateString(mqtt_json_s.mqtt_hum)); //湿度
                //cJSON_AddItemToObject(next, "field3", cJSON_CreateString(mqtt_json_s.mqtt_mode));        //模式
                if (human_status == NOHUMAN)
                {
                        cJSON_AddItemToObject(next, "field3", cJSON_CreateString("0"));
                }
                else if (human_status == HAVEHUMAN)

                {
                        cJSON_AddItemToObject(next, "field3", cJSON_CreateString("1"));
                }

                cJSON_AddItemToObject(next, "field5", cJSON_CreateString(mqtt_json_s.mqtt_Rssi)); //WIFI RSSI
        }

        char *cjson_printunformat;
        cjson_printunformat = cJSON_PrintUnformatted(root); //将整个 json 转换成字符串 ，没有格式
        //pCreat_json->creat_json_b = cJSON_PrintUnformatted(root);
        //pCreat_json->creat_json_c = strlen(cJSON_PrintUnformatted(root));

        pCreat_json->creat_json_c = strlen(cjson_printunformat); //  creat_json_c 是整个json 所占的长度
        //pCreat_json->creat_json_b=cjson_printunformat;
        //pCreat_json->creat_json_b=malloc(pCreat_json->creat_json_c);
        bzero(pCreat_json->creat_json_b, sizeof(pCreat_json->creat_json_b)); //  creat_json_b 是整个json 包
        memcpy(pCreat_json->creat_json_b, cjson_printunformat, pCreat_json->creat_json_c);
        //printf("http_json=%s\n",pCreat_json->creat_json_b);
        free(cjson_printunformat);
        cJSON_Delete(root);
        //return pCreat_json;
}
