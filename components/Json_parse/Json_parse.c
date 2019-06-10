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
//#include "Motorctl.h"
//#include "Wind.h"
#include "Bluetooth.h"
#include "Human.h"
#include "sht30dis.h"
#include "ota.h"
#include "Led.h"
#include "w5500_driver.h"

//wifi_config_t wifi_config;

//#define DEBUG_0

unsigned long fn_dp = 0; //数据发送频率
unsigned long fn_th = 0; //温湿度频率
uint8_t cg_data_led = 1; //发送数据 LED状态 0：不闪烁 1：闪烁
uint8_t net_mode = 0;    //上网模式选择 0：自动模式 1：lan模式 2：wifi模式

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

static short Parse_metadata(char *ptrptr)
{
    bool fn_flag = 0;
    if (NULL == ptrptr)
    {
        return 0;
    }

    cJSON *pJsonJson = cJSON_Parse(ptrptr);
    if (NULL == pJsonJson)
    {
        cJSON_Delete(pJsonJson); //delete pJson

        return 0;
    }

    cJSON *pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_th"); //"fn_th"
    if (NULL != pSubSubSub)
    {

        if ((unsigned long)pSubSubSub->valueint != fn_th)
        {
            fn_flag = 1;
            fn_th = (unsigned long)pSubSubSub->valueint;
            printf("fn_th = %ld\n", fn_th);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_dp"); //"fn_dp"
    if (NULL != pSubSubSub)
    {

        if ((unsigned long)pSubSubSub->valueint != fn_dp)
        {
            fn_flag = 1;
            fn_dp = (unsigned long)pSubSubSub->valueint;
            printf("fn_dp = %ld\n", fn_dp);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "cg_data_led"); //"cg_data_led"
    if (NULL != pSubSubSub)
    {

        if ((uint8_t)pSubSubSub->valueint != cg_data_led)
        {
            cg_data_led = (uint8_t)pSubSubSub->valueint;
            printf("cg_data_led = %d\n", cg_data_led);
            if (cg_data_led == 0)
            {
                Turn_Off_LED();
            }
            else
            {
                Turn_ON_LED();
            }
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "net_mode"); //"net_mode"
    if (NULL != pSubSubSub)
    {

        if ((uint8_t)pSubSubSub->valueint != net_mode)
        {
            net_mode = (uint8_t)pSubSubSub->valueint;
            printf("net_mode = %d\n", net_mode);
        }
    }

    cJSON_Delete(pJsonJson);
    return 1;
}

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

//解析蓝牙数据包
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
        if (cjson_blu_data_parse_wifissid != NULL)
        {
            bzero(wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
            strcpy(wifi_data.wifi_ssid, cjson_blu_data_parse_wifissid->valuestring);
            printf("%s\r\n", cjson_blu_data_parse_wifissid->valuestring);
        }

        cjson_blu_data_parse_wifipwd = cJSON_GetObjectItem(cjson_blu_data_parse, "wifiPwd");
        if (cjson_blu_data_parse_wifipwd != NULL)
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
    // initialise_wifi(cjson_blu_data_parse_wifissid->valuestring, cjson_blu_data_parse_wifipwd->valuestring);
    initialise_wifi();
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
    cJSON *json_data_parse_channel_metadata = NULL;
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
            json_data_parse_channel_metadata = cJSON_GetObjectItem(json_data_parse_channel_value, "metadata");

            // printf("metadata: %s\n", json_data_parse_channel_metadata->valuestring);
            Parse_metadata(json_data_parse_channel_metadata->valuestring);
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
        // printf("result: %s\n", json_data_parse_value->valuestring);
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

                E2prom_Write(0x20, data_write3, 16);

                fflush(stdout); //使stdout清空，就会立刻输出所有在缓冲区的内容。
                esp_restart();  //芯片复位 函数位于esp_system.h
            }
        }

        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "metadata");
        if (json_data_parse_value != NULL)
        {
            // printf("metadata: %s\n", json_data_parse_value->valuestring);
            Parse_metadata(json_data_parse_value->valuestring);
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
        // printf("json_data_parse_value %s\n", json_print);
    }
    free(json_print);
    cJSON_Delete(json_data_parse);

    return 1;
}

//解析MQTT      指令
esp_err_t parse_objects_mqtt(char *mqtt_json_data)
{
    cJSON *json_data_parse = NULL;
    cJSON *json_data_string_parse = NULL;
    cJSON *json_data_command_id_parse = NULL;

    //OTA相关
    cJSON *json_data_action = NULL;
    cJSON *json_data_url = NULL;
    cJSON *json_data_vesion = NULL;

    json_data_parse = cJSON_Parse(mqtt_json_data);
    // printf("%s", cJSON_Print(json_data_parse));

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

        post_status = POST_NORMAL;
        json_data_string_parse = cJSON_Parse(json_data_string_parse->valuestring); //将command_string再次构建成json格式，以便二次解析
        if (json_data_string_parse != NULL)
        {

            // printf("MQTT-command_string  = %s\r\n", cJSON_Print(json_data_string_parse));

            //收到OTA相关指令
            if ((json_data_action = cJSON_GetObjectItem(json_data_string_parse, "action")) != NULL &&
                (json_data_vesion = cJSON_GetObjectItem(json_data_string_parse, "version")) != NULL &&
                (json_data_url = cJSON_GetObjectItem(json_data_string_parse, "url")) != NULL)
            {
                printf("OTA命令进入\r\n");
                //如果命令是OTA
                if (strcmp(json_data_action->valuestring, "ota") == 0)
                {
                    if (strcmp(json_data_vesion->valuestring, FIRMWARE) != 0) //判断版本号
                    {
                        strcpy(mqtt_json_s.mqtt_ota_url, json_data_url->valuestring);
                        E2prom_page_Write(ota_url_add, (uint8_t *)mqtt_json_s.mqtt_ota_url, 128);
                        printf("OTA_URL=%s\r\n OTA_VERSION=%s\r\n", mqtt_json_s.mqtt_ota_url, json_data_vesion->valuestring);
                        ota_start(); //启动OTA
                    }
                    else
                    {
                        printf("当前版本无需升级 \r\n");
                    }
                }
                else
                {
                    printf("Action非ota \r\n");
                }
            }
            else if ((json_data_action = cJSON_GetObjectItem(json_data_string_parse, "command")) != NULL)
            {
                printf("command CMD!\r\n");
                ParseTcpUartCmd(cJSON_Print(json_data_string_parse));
            }
            else
            {
                printf("非许可命令\r\n");
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

    return 1;
}

void create_http_json(creat_json *pCreat_json)
{
    printf("INTO CREATE_HTTP_JSON\r\n");
    //creat_json *pCreat_json = malloc(sizeof(creat_json));
    cJSON *root = cJSON_CreateObject();
    cJSON *item = cJSON_CreateObject();
    cJSON *next = cJSON_CreateObject();
    cJSON *fe_body = cJSON_CreateArray();
    //char status_creat_json_c[256];

    //printf("Server_Timer_SEND() %s", (char *)Server_Timer_SEND());
    //strncpy(http_json_c.http_time[20], Server_Timer_SEND(), 20);
    // printf("this http_json_c.http_time[20]  %s\r\n", http_json_c.http_time);
    strncpy(http_json_c.http_time, Server_Timer_SEND(), 24);
    wifi_ap_record_t wifidata;
    if (esp_wifi_sta_get_ap_info(&wifidata) == 0)
    {
        itoa(wifidata.rssi, mqtt_json_s.mqtt_Rssi, 10);
    }
    if (tem == 0 || hum == 0) //规避错误值
    {
        memset(mqtt_json_s.mqtt_tem, 0, sizeof(mqtt_json_s.mqtt_tem));
        memset(mqtt_json_s.mqtt_hum, 0, sizeof(mqtt_json_s.mqtt_hum));
    }
    else
    {
        sprintf(mqtt_json_s.mqtt_tem, "%4.2f", tem);
        sprintf(mqtt_json_s.mqtt_hum, "%4.2f", hum);
    }

    //printf("status_creat_json %s\r\n", status_creat_json);
    cJSON_AddItemToObject(root, "feeds", fe_body);
    cJSON_AddItemToArray(fe_body, item);
    cJSON_AddItemToObject(item, "created_at", cJSON_CreateString(http_json_c.http_time));
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

    char *cjson_printunformat;
    // cjson_printunformat = cJSON_PrintUnformatted(root); //将整个 json 转换成字符串 ，没有格式
    cjson_printunformat = cJSON_Print(root); //将整个 json 转换成字符串 ，有格式
    //printf("status_creat_json= %s\r\n", cjson_printunformat);

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

esp_err_t ParseTcpUartCmd(char *pcCmdBuffer)
{
    if (NULL == pcCmdBuffer) //null
    {
        return ESP_FAIL;
    }

    cJSON *pJson = cJSON_Parse(pcCmdBuffer); //parse json data
    if (NULL == pJson)
    {
        cJSON_Delete(pJson); //delete pJson

        return ESP_FAIL;
    }

    cJSON *pSub = cJSON_GetObjectItem(pJson, "Command"); //"Command"
    if (NULL != pSub)
    {
        if (!strcmp((char const *)pSub->valuestring, "SetupProduct")) //Command:SetupProduct
        {
            pSub = cJSON_GetObjectItem(pJson, "Password"); //"Password"
            if (NULL != pSub)
            {

                if (!strcmp((char const *)pSub->valuestring, "CloudForce"))
                {
                    //       E2prom_Write(PRODUCTURI_FLAG_ADDR, PRODUCT_URI, strlen(PRODUCT_URI), 1); //save product-uri flag

                    pSub = cJSON_GetObjectItem(pJson, "ProductID"); //"ProductID"
                    if (NULL != pSub)
                    {
                        printf("ProductID= %s\n", pSub->valuestring);
                        E2prom_Write(PRODUCT_ID_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring)); //save ProductID
                    }

                    pSub = cJSON_GetObjectItem(pJson, "SeriesNumber"); //"SeriesNumber"
                    if (NULL != pSub)
                    {
                        printf("SeriesNumber= %s\n", pSub->valuestring);
                        E2prom_Write(SERISE_NUM_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring)); //save SeriesNumber
                    }

                    pSub = cJSON_GetObjectItem(pJson, "Host"); //"Host"
                    if (NULL != pSub)
                    {

                        //E2prom_Write(HOST_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring), 1); //save host in at24c08
                    }

                    pSub = cJSON_GetObjectItem(pJson, "apn"); //"apn"
                    if (NULL != pSub)
                    {

                        //E2prom_Write(APN_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring), 1); //save apn
                    }

                    pSub = cJSON_GetObjectItem(pJson, "user"); //"user"
                    if (NULL != pSub)
                    {

                        //E2prom_Write(BEARER_USER_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring), 1); //save user
                    }

                    pSub = cJSON_GetObjectItem(pJson, "pwd"); //"pwd"
                    if (NULL != pSub)
                    {

                        // E2prom_Write(BEARER_PWD_ADDR, (uint8_t *)pSub->valuestring, strlen(pSub->valuestring), 1); //save pwd
                    }

                    //清空API-KEY存储，激活后获取
                    uint8_t data_write2[33] = "\0";
                    E2prom_Write(0x00, data_write2, 32);

                    //清空channelid，激活后获取
                    uint8_t data_write3[16] = "\0";
                    E2prom_Write(0x20, data_write3, 16);

                    uint8_t zerobuf[256] = "\0";
                    E2prom_BluWrite(0x00, (uint8_t *)zerobuf, 256); //清空蓝牙
                    printf("SetupProduct Successed !");
                    printf("{\"status\":0,\"code\": 0}");
                    vTaskDelay(3000 / portTICK_RATE_MS);
                    cJSON_Delete(pJson);
                    fflush(stdout); //使stdout清空，就会立刻输出所有在缓冲区的内容。
                    esp_restart();  //芯片复位 函数位于esp_system.h

                    return ESP_OK;
                }
            }
        }
        else if (!strcmp((char const *)pSub->valuestring, "SetupEthernet")) //Command:SetupEthernet
        {
            char *InpString;
            uint8_t set_net_buf[16];

            pSub = cJSON_GetObjectItem(pJson, "dhcp"); //"dhcp"
            if (NULL != pSub)
            {
                EE_byte_Write(ADDR_PAGE2, dhcp_mode_add, (uint8_t)pSub->valueint); //写入DHCP模式
                        }

            pSub = cJSON_GetObjectItem(pJson, "ip"); //"ip"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                set_net_buf[0] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[1] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[2] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[3] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "sn"); //"sn"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                set_net_buf[4] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[5] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[6] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[7] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "gw"); //"gw"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                set_net_buf[8] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[9] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[10] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[11] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "dns"); //"dns"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                set_net_buf[12] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[13] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[14] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                set_net_buf[15] = (uint8_t)strtoul(InpString, 0, 10);
            }
            E2prom_page_Write(NETINFO_add, set_net_buf, sizeof(set_net_buf));
            // for (uint8_t i = 0; i < 16; i++)
            // {
            //     printf("set_net_buf[%d]:%d\n", i, set_net_buf[i]);
            //     EE_byte_Write(ADDR_PAGE3, NETINFO_add + i, set_net_buf[i]);
            // }

            W5500_Network_Init();

            // E2prom_page_Read(NETINFO_add, (uint8_t *)netinfo_buff, sizeof(netinfo_buff));
            // printf("%02x \n", (unsigned int)netinfo_buff);
            cJSON_Delete(pJson); //delete pJson
            return 1;
        }
        else if (!strcmp((char const *)pSub->valuestring, "SetupWifi")) //Command:SetupWifi
        {
            pSub = cJSON_GetObjectItem(pJson, "SSID"); //"SSID"
            if (NULL != pSub)
            {
                bzero(wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
                strcpy(wifi_data.wifi_ssid, pSub->valuestring);
                printf("WIFI_SSID = %s\r\n", pSub->valuestring);
            }

            pSub = cJSON_GetObjectItem(pJson, "password"); //"password"
            if (NULL != pSub)
            {
                bzero(wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                strcpy(wifi_data.wifi_pwd, pSub->valuestring);
                printf("WIFI_PWD = %s\r\n", pSub->valuestring);
            }

            printf("{\"status\":0,\"code\": 0}");
            // initialise_wifi(wifi_data.wifi_ssid, wifi_data.wifi_pwd);
            initialise_wifi();
            cJSON_Delete(pJson); //delete pJson

            return 1;
        }
    }

    cJSON_Delete(pJson); //delete pJson

    return ESP_FAIL;
}
