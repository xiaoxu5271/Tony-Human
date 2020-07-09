#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include "esp_system.h"
#include "esp_log.h"

#include "Json_parse.h"
#include "Nvs.h"
#include "ServerTimer.h"

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
#include "tcp_bsp.h"
#include "my_base64.h"
#include "Human.h"

#define TAG "Json-parese"

uint32_t fn_dp = 0;           //数据发送频率
uint32_t fn_th = 0;           //温湿度频率
uint32_t fn_sen = 950;        //人感灵敏度 阈值，单位ms
uint32_t fn_sen_cycle = 1000; //人感灵敏度 周期，单位ms
uint32_t fn_sen_res = 10000;  //人感状态重置周期，单位ms
uint32_t fn_sen_sta = 300;    //人感原始值统计周期，单位s
uint8_t cg_data_led = 1;      //发送数据 LED状态 0：不闪烁 1：闪烁
uint8_t net_mode = 0;         //上网模式选择 0：自动模式 1：lan模式 2：wifi模式

char SerialNum[SERISE_NUM_LEN] = {0};
char ProductId[PRODUCT_ID_LEN] = {0};
char ApiKey[API_KEY_LEN] = {0};
char ChannelId[CHANNEL_ID_LEN] = {0};
char USER_ID[USER_ID_LEN] = {0};
char WEB_SERVER[WEB_HOST_LEN] = {0};
char WEB_PORT[5] = {0};
char MQTT_SERVER[WEB_HOST_LEN] = {0};
char MQTT_PORT[5] = {0};
char BleName[50] = {0};
char SIM_APN[32] = {0};
char SIM_USER[32] = {0};
char SIM_PWD[32] = {0};

typedef enum
{
    HTTP_JSON_FORMAT_TINGERROR,
    HTTP_RESULT_UNSUCCESS,
    HTTP_OVER_PARSE,
    HTTP_WRITE_FLASH_OVER,
    CREAT_OBJECT_OVER,
} http_error_info;

char get_num[2];
char *key;
int n = 0, i;

static short Parse_metadata(char *ptrptr)
{
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

    // cJSON *pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_th"); //"fn_th"
    // if (NULL != pSubSubSub)
    // {
    //     if ((uint32_t)pSubSubSub->valueint != fn_th)
    //     {
    //         fn_th = (uint32_t)pSubSubSub->valueint;
    //         printf("fn_th = %d\n", fn_th);
    //     }
    // }

    cJSON *pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_dp"); //"fn_dp"
    if (NULL != pSubSubSub)
    {

        if ((uint32_t)pSubSubSub->valueint != fn_dp)
        {
            fn_dp = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_DP_ADD, fn_dp, 4);
            printf("fn_dp = %d\n", fn_dp);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_sen"); //"fn_sen"
    if (NULL != pSubSubSub)
    {

        if ((uint32_t)pSubSubSub->valueint != fn_sen)
        {
            fn_sen = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_SEN_ADD, fn_sen, 4);
            printf("fn_sen = %d\n", fn_sen);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "cg_data_led"); //"cg_data_led"
    if (NULL != pSubSubSub)
    {

        if ((uint8_t)pSubSubSub->valueint != cg_data_led)
        {
            cg_data_led = (uint8_t)pSubSubSub->valueint;
            E2P_WriteLenByte(CG_DATA_LED_ADD, cg_data_led, 1);
            printf("cg_data_led = %d\n", cg_data_led);
        }
    }

    // pSubSubSub = cJSON_GetObjectItem(pJsonJson, "net_mode"); //"net_mode"
    // if (NULL != pSubSubSub)
    // {
    //     if ((uint8_t)pSubSubSub->valueint != net_mode)
    //     {
    //         net_mode = (uint8_t)pSubSubSub->valueint;
    //         EE_byte_Write(ADDR_PAGE2, net_mode_add, net_mode); //写入net_mode
    //         printf("net_mode = %d\n", net_mode);
    //     }
    // }
    cJSON_Delete(pJsonJson);
    return 1;
}

//解析蓝牙数据包
int32_t parse_objects_bluetooth(char *blu_json_data)
{
    cJSON *cjson_blu_data_parse = NULL;
    cJSON *cjson_blu_data_parse_command = NULL;
    printf("start_ble_parse_json：\r\n%s\n", blu_json_data);

    char *resp_val = NULL;
    resp_val = strstr(blu_json_data, "{");
    if (resp_val == NULL)
    {
        ESP_LOGE(TAG, "DATA NO JSON");
        return 0;
    }
    cjson_blu_data_parse = cJSON_Parse(resp_val);

    if (cjson_blu_data_parse == NULL) //如果数据包不为JSON则退出
    {
        ESP_LOGE(TAG, "%d,%s", __LINE__, blu_json_data);
        cJSON_Delete(cjson_blu_data_parse);
        return BLU_JSON_FORMAT_ERROR;
    }
    else
    {
        cjson_blu_data_parse_command = cJSON_GetObjectItem(cjson_blu_data_parse, "command");
        ESP_LOGI(TAG, "command=%s\r\n", cjson_blu_data_parse_command->valuestring);
        char *Json_temp;
        Json_temp = cJSON_PrintUnformatted(cjson_blu_data_parse);
        ParseTcpUartCmd(Json_temp);
        free(Json_temp);
    }
    cJSON_Delete(cjson_blu_data_parse);
    return 1;
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
    cJSON *json_data_parse_channel_user_id = NULL;
    //char *json_print;

    // printf("start_parse_active_http_json\r\n");

    char *resp_val = NULL;
    resp_val = strstr(http_json_data, "{\"result\":\"success\",");
    if (resp_val == NULL)
    {
        ESP_LOGE("JSON", "DATA NO JSON");
        return 0;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL)
    {
        printf("Json Formatting error3\n");

        cJSON_Delete(json_data_parse);
        return 0;
    }
    else
    {
        if (cJSON_GetObjectItem(json_data_parse, "channel") != NULL)
        {
            json_data_parse_channel_value = cJSON_GetObjectItem(json_data_parse, "channel");

            //printf("%s\r\n", cJSON_Print(json_data_parse_channel_value));

            json_data_parse_channel_channel_write_key = cJSON_GetObjectItem(json_data_parse_channel_value, "write_key");
            json_data_parse_channel_channel_id_value = cJSON_GetObjectItem(json_data_parse_channel_value, "channel_id");
            json_data_parse_channel_metadata = cJSON_GetObjectItem(json_data_parse_channel_value, "metadata");
            json_data_parse_channel_user_id = cJSON_GetObjectItem(json_data_parse_channel_value, "user_id");

            // printf("metadata: %s\n", json_data_parse_channel_metadata->valuestring);
            Parse_metadata(json_data_parse_channel_metadata->valuestring);
            //printf("api key=%s\r\n", json_data_parse_channel_channel_write_key->valuestring);
            //printf("channel_id=%s\r\n", json_data_parse_channel_channel_id_value->valuestring);

            //写入API-KEY
            sprintf(ApiKey, "%s%c", json_data_parse_channel_channel_write_key->valuestring, '\0');
            E2P_Write(API_KEY_ADD, (uint8_t *)ApiKey, API_KEY_LEN);
            //写入channelid
            sprintf(ChannelId, "%s%c", json_data_parse_channel_channel_id_value->valuestring, '\0');
            E2P_Write(CHANNEL_ID_ADD, (uint8_t *)ChannelId, CHANNEL_ID_LEN);
            //写入user_id
            sprintf(USER_ID, "%s%c", json_data_parse_channel_user_id->valuestring, '\0');
            E2P_Write(USER_ID_ADD, (uint8_t *)USER_ID, USER_ID_LEN);
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
    // cJSON *json_data_parse_errorcode = NULL;

    char *resp_val = NULL;
    resp_val = strstr(http_json_data, "{\"result\":");
    if (resp_val == NULL)
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        return 0;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL)
    {

        // printf("Json Formatting error3\n");
        ESP_LOGE(TAG, "%s", http_json_data);
        cJSON_Delete(json_data_parse);
        return 0;
    }
    else
    {
        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "metadata");
        if (json_data_parse_value != NULL)
        {
            // printf("metadata: %s\n", json_data_parse_value->valuestring);
            Parse_metadata(json_data_parse_value->valuestring);
        }

        // json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "sensors"); //sensors
        // if (NULL != json_data_parse_value)
        // {
        //     Parse_fields_num(json_data_parse_value->valuestring); //parse sensors
        // }

        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "command"); //
        if (NULL != json_data_parse_value)
        {
            char *mqtt_json;
            mqtt_json = cJSON_PrintUnformatted(json_data_parse_value);
            ESP_LOGI(TAG, "%s", mqtt_json);
            parse_objects_mqtt(mqtt_json, false); //parse mqtt
            free(mqtt_json);
        }
    }

    cJSON_Delete(json_data_parse);
    return 1;
}

esp_err_t parse_objects_heart(char *json_data)
{
    cJSON *json_data_parse = NULL;
    cJSON *json_data_parse_value = NULL;

    char *resp_val = NULL;
    resp_val = strstr(json_data, "{\"result\":\"success\",");
    if (resp_val == NULL)
    {
        ESP_LOGE("JSON", "DATA NO JSON");
        return 0;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL) //如果数据包不为JSON则退出
    {

        printf("Json Formatting error4\n");

        cJSON_Delete(json_data_parse);
        return 0;
    }
    else
    {
        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "result");
        if (!(strcmp(json_data_parse_value->valuestring, "success")))
        {
            json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "server_time");
            Server_Timer_GET(json_data_parse_value->valuestring);
        }
        else
        {
            printf("active:error\r\n");
            cJSON_Delete(json_data_parse);
            return 0;
        }
    }
    cJSON_Delete(json_data_parse);
    return 1;
}

//解析MQTT指令
esp_err_t parse_objects_mqtt(char *mqtt_json_data, bool sw_flag)
{
    cJSON *json_data_parse = NULL;
    char *resp_val = NULL;
    resp_val = strstr(mqtt_json_data, "{\"command_id\":");
    if (resp_val == NULL)
    {
        // ESP_LOGE("JSON", "DATA NO JSON");
        return 0;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL) //如果数据包不为JSON则退出
    {
        printf("Json Formatting error5\n");
        cJSON_Delete(json_data_parse);
        return 0;
    }

    cJSON *pSubSubSub = cJSON_GetObjectItem(json_data_parse, "command_id"); //
    if (pSubSubSub != NULL)
    {
        memset(mqtt_json_s.mqtt_command_id, 0, sizeof(mqtt_json_s.mqtt_command_id));
        strncpy(mqtt_json_s.mqtt_command_id, pSubSubSub->valuestring, strlen(pSubSubSub->valuestring));
        post_status = POST_NORMAL;
        // if (Binary_dp != NULL)
        // {
        //     xTaskNotifyGive(Binary_dp);
        // }
    }

    pSubSubSub = cJSON_GetObjectItem(json_data_parse, "command_string"); //
    if (pSubSubSub != NULL)
    {
        // ESP_LOGI(TAG, "command_string=%s", pSubSubSub->valuestring);
        ParseTcpUartCmd(pSubSubSub->valuestring);

        cJSON *json_data_parse_1 = cJSON_Parse(pSubSubSub->valuestring);
        if (json_data_parse_1 != NULL)
        {
            pSubSubSub = cJSON_GetObjectItem(json_data_parse_1, "action"); //
            if (pSubSubSub != NULL)
            {
                if (strcmp(pSubSubSub->valuestring, "ota") == 0)
                {
                    pSubSubSub = cJSON_GetObjectItem(json_data_parse_1, "url"); //
                    if (pSubSubSub != NULL)
                    {
                        strcpy(mqtt_json_s.mqtt_ota_url, pSubSubSub->valuestring);
                        ESP_LOGI(TAG, "OTA_URL=%s", mqtt_json_s.mqtt_ota_url);

                        // pSubSubSub = cJSON_GetObjectItem(json_data_parse_1, "size"); //
                        // if (pSubSubSub != NULL)
                        // {
                        //     mqtt_json_s.mqtt_file_size = (uint32_t)pSubSubSub->valuedouble;
                        //     ESP_LOGI(TAG, "OTA_SIZE=%d", mqtt_json_s.mqtt_file_size);

                        pSubSubSub = cJSON_GetObjectItem(json_data_parse_1, "version"); //
                        if (pSubSubSub != NULL)
                        {
                            if (strcmp(pSubSubSub->valuestring, FIRMWARE) != 0) //与当前 版本号 对比
                            {
                                ESP_LOGI(TAG, "OTA_VERSION=%s", pSubSubSub->valuestring);
                                ota_start(); //启动OTA
                            }
                        }
                        // }
                    }
                }
                else if (strcmp(pSubSubSub->valuestring, "command") == 0 && sw_flag == true)
                {
                    pSubSubSub = cJSON_GetObjectItem(json_data_parse_1, "set_state");
                    if (pSubSubSub != NULL)
                    {
                        // Switch_Relay(pSubSubSub->valueint);
                    }
                }
            }
        }
        cJSON_Delete(json_data_parse_1);
    }
    cJSON_Delete(json_data_parse);

    return 1;
}

uint16_t Create_Status_Json(char *status_buff, bool filed_flag)
{
    uint8_t mac_sys[6] = {0};
    char *ssid64_buff;
    esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC

    if (net_mode == NET_WIFI)
    {
        ssid64_buff = (char *)malloc(64);
        memset(ssid64_buff, 0, 64);
        base64_encode(wifi_data.wifi_ssid, strlen(wifi_data.wifi_ssid), ssid64_buff, 64);

        sprintf(status_buff, "],\"status\":\"mac=%02x:%02x:%02x:%02x:%02x:%02x\",\"ssid_base64\":\"%s\"}",
                mac_sys[0],
                mac_sys[1],
                mac_sys[2],
                mac_sys[3],
                mac_sys[4],
                mac_sys[5],
                ssid64_buff);
        free(ssid64_buff);
    }
    else
    {
        // sprintf(status_buff, "],\"status\":\"mac=%02x:%02x:%02x:%02x:%02x:%02x,ICCID=%s\"}",
        //         mac_sys[0],
        //         mac_sys[1],
        //         mac_sys[2],
        //         mac_sys[3],
        //         mac_sys[4],
        //         mac_sys[5],
        //         ICCID);
    }

    return strlen(status_buff);
}

void create_http_json(creat_json *pCreat_json, uint8_t flag)
{
    //creat_json *pCreat_json = malloc(sizeof(creat_json));
    cJSON *root = cJSON_CreateObject();
    // cJSON *item = cJSON_CreateObject();
    cJSON *next = cJSON_CreateObject();
    cJSON *fe_body = cJSON_CreateArray();
    uint8_t mac_sys[6] = {0};
    char mac_buff[100] = {0};
    char ssid64_buff[64] = {0};

    char *time_buff = (char *)malloc(24);
    Server_Timer_SEND(time_buff);
    wifi_ap_record_t wifidata;

    //printf("status_creat_json %s\r\n", status_creat_json);
    cJSON_AddItemToObject(root, "feeds", fe_body);
    // cJSON_AddItemToArray(fe_body, item);
    // cJSON_AddItemToObject(item, "created_at", cJSON_CreateString(http_json_c.http_time));
    cJSON_AddItemToArray(fe_body, next);
    cJSON_AddItemToObject(next, "created_at", cJSON_CreateString(time_buff));
    // cJSON_AddItemToObject(next, "field2", cJSON_CreateString(mqtt_json_s.mqtt_tem)); //温度
    // cJSON_AddItemToObject(next, "field3", cJSON_CreateString(mqtt_json_s.mqtt_hum)); //湿度

    if (human_status == false)
    {
        cJSON_AddItemToObject(next, "field1", cJSON_CreateString("0"));
    }
    else if (human_status == true)
    {
        cJSON_AddItemToObject(next, "field1", cJSON_CreateString("1"));
    }
    //构建附属信息
    if (flag == 1)
    {
        if (net_mode == NET_WIFI) //wifi
        {
            if (esp_wifi_sta_get_ap_info(&wifidata) == 0)
            {
                itoa(wifidata.rssi, mqtt_json_s.mqtt_Rssi, 10);
            }
            esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC，
            sprintf(mac_buff,
                    "mac=%02x:%02x:%02x:%02x:%02x:%02x",
                    mac_sys[0],
                    mac_sys[1],
                    mac_sys[2],
                    mac_sys[3],
                    mac_sys[4],
                    mac_sys[5]);
            base64_encode(wifi_data.wifi_ssid, strlen(wifi_data.wifi_ssid), ssid64_buff, sizeof(ssid64_buff));
            cJSON_AddItemToObject(next, "field2", cJSON_CreateString(mqtt_json_s.mqtt_Rssi)); //WIFI RSSI
            cJSON_AddItemToObject(root, "status", cJSON_CreateString(mac_buff));
            cJSON_AddItemToObject(root, "ssid_base64", cJSON_CreateString(ssid64_buff));
        }
        else //以太网
        {
            esp_read_mac(mac_sys, 3); //获取芯片内部默认出厂MAC，
            sprintf(mac_buff,
                    "mac=%02x:%02x:%02x:%02x:%02x:%02x",
                    mac_sys[0],
                    mac_sys[1],
                    mac_sys[2],
                    mac_sys[3],
                    mac_sys[4],
                    mac_sys[5]);
            base64_encode(wifi_data.wifi_ssid, strlen(wifi_data.wifi_ssid), ssid64_buff, sizeof(ssid64_buff));
            cJSON_AddItemToObject(root, "status", cJSON_CreateString(mac_buff));
        }

        cJSON_AddItemToObject(next, "field3", cJSON_CreateNumber(human_intr_num)); //
        human_intr_num = 0;
    }

    char *cjson_printunformat;
    // cjson_printunformat = cJSON_PrintUnformatted(root); //将整个 json 转换成字符串 ，没有格式
    cjson_printunformat = cJSON_PrintUnformatted(root); //将整个 json 转换成字符串 ，有格式
    //printf("status_creat_json= %s\r\n", cjson_printunformat);

    pCreat_json->creat_json_len = strlen(cjson_printunformat); //  creat_json_c 是整个json 所占的长度
    //pCreat_json->creat_json_b=cjson_printunformat;
    //pCreat_json->creat_json_b=malloc(pCreat_json->creat_json_c);
    bzero(pCreat_json->creat_json_buff, sizeof(pCreat_json->creat_json_buff)); //  creat_json_b 是整个json 包
    memcpy(pCreat_json->creat_json_buff, cjson_printunformat, pCreat_json->creat_json_len);
    //printf("http_json=%s\n",pCreat_json->creat_json_b);
    free(cjson_printunformat);
    free(time_buff);
    cJSON_Delete(root);
    //return pCreat_json;
}

esp_err_t ParseTcpUartCmd(char *pcCmdBuffer)
{
    char send_buf[128] = {0};
    sprintf(send_buf, "{\"status\":0,\"code\": 0}");
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
        //{"Command":"SetupProduct","Password":"CloudForce","ProductID":"ubibot-ms1","SeriesNumber":"A0005HUM1","Host":"api.ubibot.cn","Port":"80","MqttHost":"mqtt.ubibot.cn","MqttPort":"1883"}
        if (!strcmp((char const *)pSub->valuestring, "SetupProduct")) //Command:SetupProduct
        {
            pSub = cJSON_GetObjectItem(pJson, "Password"); //"Password"
            if (NULL != pSub)
            {
                if (!strcmp((char const *)pSub->valuestring, "CloudForce"))
                {
                    // E2prom_set_defaul(false);
                    pSub = cJSON_GetObjectItem(pJson, "ProductID"); //"ProductID"
                    if (NULL != pSub)
                    {
                        printf("ProductID= %s, len= %d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(PRODUCT_ID_ADDR, (uint8_t *)pSub->valuestring, PRODUCT_ID_LEN); //save ProductID
                    }

                    pSub = cJSON_GetObjectItem(pJson, "SeriesNumber"); //"SeriesNumber"
                    if (NULL != pSub)
                    {
                        printf("SeriesNumber= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(SERISE_NUM_ADDR, (uint8_t *)pSub->valuestring, SERISE_NUM_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "Host"); //"Host"
                    if (NULL != pSub)
                    {
                        printf("Host= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(WEB_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "Port"); //"WEB PORT"
                    if (NULL != pSub)
                    {

                        printf("Port= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(WEB_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "MqttHost"); //"mqtt Host"
                    if (NULL != pSub)
                    {
                        printf("MqttHost= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(MQTT_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "MqttPort"); //"Host"
                    if (NULL != pSub)
                    {
                        printf("MqttPort= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(MQTT_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
                    }

                    printf("{\"status\":0,\"code\": 0}");

                    vTaskDelay(3000 / portTICK_RATE_MS);
                    cJSON_Delete(pJson);
                    esp_restart(); //芯片复位 函数位于esp_system.h

                    return ESP_OK;
                }
                else
                {
                    //密码错误
                    printf("{\"status\":1,\"code\": 101}\r\n");
                }
            }
        }

        //{"command":"SetupHost","Host":"api.ubibot.io","Port":"80","MqttHost":"mqtt.ubibot.io","MqttPort":"1883"}
        else if (!strcmp((char const *)pSub->valuestring, "SetupHost")) //Command:SetupWifi
        {
            pSub = cJSON_GetObjectItem(pJson, "Host"); //"Host"
            if (NULL != pSub)
            {
                ESP_LOGI(TAG, "Host= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(WEB_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save SeriesNumber
            }

            pSub = cJSON_GetObjectItem(pJson, "Port"); //"WEB PORT"
            if (NULL != pSub)
            {
                printf("Port= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(WEB_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
            }

            pSub = cJSON_GetObjectItem(pJson, "MqttHost"); //"mqtt Host"
            if (NULL != pSub)
            {
                printf("MqttHost= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(MQTT_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
            }

            pSub = cJSON_GetObjectItem(pJson, "MqttPort"); //"Host"
            if (NULL != pSub)
            {
                printf("MqttPort= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(MQTT_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
            }

            printf("{\"status\":0,\"code\": 0}");

            vTaskDelay(3000 / portTICK_RATE_MS);
            cJSON_Delete(pJson);
            esp_restart(); //芯片复位 函数位于esp_system.h

            return ESP_OK;
        }

        else if (!strcmp((char const *)pSub->valuestring, "SetupEthernet")) //Command:SetupEthernet
        {
            char *InpString;

            pSub = cJSON_GetObjectItem(pJson, "dhcp"); //"dhcp"
            if (NULL != pSub)
            {
                E2P_WriteOneByte(ETHERNET_DHCP_ADD, (uint8_t)pSub->valueint); //写入DHCP模式
                user_dhcp_mode = (uint8_t)pSub->valueint;
            }

            pSub = cJSON_GetObjectItem(pJson, "ip"); //"ip"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                netinfo_buff[0] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[1] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[2] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[3] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "mask"); //"sn"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                netinfo_buff[4] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[5] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[6] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[7] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "gw"); //"gw"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                netinfo_buff[8] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[9] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[10] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[11] = (uint8_t)strtoul(InpString, 0, 10);
            }

            pSub = cJSON_GetObjectItem(pJson, "dns1"); //"dns"
            if (NULL != pSub)
            {
                InpString = strtok(pSub->valuestring, ".");
                netinfo_buff[12] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[13] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[14] = (uint8_t)strtoul(InpString, 0, 10);

                InpString = strtok(NULL, ".");
                netinfo_buff[15] = (uint8_t)strtoul(InpString, 0, 10);
            }

            esp_log_buffer_char(TAG, netinfo_buff, 16);

            E2P_Write(ETHERNET_IP_ADD, netinfo_buff, sizeof(netinfo_buff));

            E2P_WriteOneByte(NET_MODE_ADD, NET_LAN); //写入net_mode

            net_mode = NET_LAN;
            Net_Switch();

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
                E2P_Write(WIFI_SSID_ADD, (uint8_t *)wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
                printf("WIFI_SSID = %s\r\n", pSub->valuestring);
                net_mode = NET_WIFI;
                E2P_WriteOneByte(NET_MODE_ADD, net_mode); //写入net_mode
            }

            pSub = cJSON_GetObjectItem(pJson, "password"); //"password"
            if (NULL != pSub)
            {
                bzero(wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                strcpy(wifi_data.wifi_pwd, pSub->valuestring);
                E2P_Write(WIFI_PASSWORD_ADD, (uint8_t *)wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                printf("WIFI_PWD = %s\r\n", pSub->valuestring);
            }
            Net_Switch();
        }
        else if (!strcmp((char const *)pSub->valuestring, "ReadProduct")) //Command:ReadProduct
        {
            char mac_buff[18];
            char *json_temp;
            uint8_t mac_sys[6] = {0};
            cJSON *root = cJSON_CreateObject();

            esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC，
            sprintf(mac_buff,
                    "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac_sys[0],
                    mac_sys[1],
                    mac_sys[2],
                    mac_sys[3],
                    mac_sys[4],
                    mac_sys[5]);

            cJSON_AddItemToObject(root, "ProductID", cJSON_CreateString(ProductId));
            cJSON_AddItemToObject(root, "SeriesNumber", cJSON_CreateString(SerialNum));
            cJSON_AddItemToObject(root, "Host", cJSON_CreateString(WEB_SERVER));
            cJSON_AddItemToObject(root, "CHANNEL_ID", cJSON_CreateString(ChannelId));
            cJSON_AddItemToObject(root, "USER_ID", cJSON_CreateString(USER_ID));
            cJSON_AddItemToObject(root, "MAC", cJSON_CreateString(mac_buff));
            cJSON_AddItemToObject(root, "firmware", cJSON_CreateString(FIRMWARE));

            // sprintf(json_temp, "%s", cJSON_PrintUnformatted(root));
            json_temp = cJSON_PrintUnformatted(root);
            printf("%s\n", json_temp);
            cJSON_Delete(root); //delete pJson
            free(json_temp);
        }
        //{"command":"CheckSensors"}
        else if (!strcmp((char const *)pSub->valuestring, "CheckSensors")) //Command:ReadProduct
        {
            cJSON *root = cJSON_CreateObject();
            char *json_temp;

            bool result_flag = true;

            if ((E2P_FLAG & ETH_FLAG & HUM_FLAG) != true)
            {
                result_flag = false;
            }

            //构建返回结果
            if (result_flag == true)
            {
                cJSON_AddStringToObject(root, "result", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "result", "ERROR");
            }
            //e2prom
            if (E2P_FLAG == true)
            {
                cJSON_AddStringToObject(root, "e2prom", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "e2prom", "ERROR");
            }
            //W5500
            if (ETH_FLAG == true)
            {
                cJSON_AddStringToObject(root, "W5500", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "W5500", "ERROR");
            }
            //HUMAN
            if (HUM_FLAG == true)
            {
                cJSON_AddStringToObject(root, "Human", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "Human", "ERROR");
            }

            json_temp = cJSON_PrintUnformatted(root);
            printf("%s\n", json_temp);
            cJSON_Delete(root); //delete pJson
            free(json_temp);
        }
        //{"command":"ScanWifiList"}
        else if (!strcmp((char const *)pSub->valuestring, "ScanWifiList"))
        {
            Scan_Wifi();
        }
        //{"command":"reboot"}
        else if (!strcmp((char const *)pSub->valuestring, "reboot"))
        {
            esp_restart();
        }
        //{"command":"AllReset"}
        else if (!strcmp((char const *)pSub->valuestring, "AllReset"))
        {
            E2prom_set_defaul(false);
        }
    }

    cJSON_Delete(pJson); //delete pJson

    return ESP_FAIL;
}

//读取EEPROM中的metadata
void Read_Metadate_E2p(void)
{

    fn_dp = E2P_ReadLenByte(FN_DP_ADD, 4);          //数据发送频率
    fn_sen = E2P_ReadLenByte(FN_SEN_ADD, 4);        //人感灵敏度
    cg_data_led = E2P_ReadOneByte(CG_DATA_LED_ADD); //发送数据 LED状态 0关闭，1打开
    net_mode = E2P_ReadOneByte(NET_MODE_ADD);       //上网模式选择 0：自动模式 1：lan模式 2：wifi模式

    printf("E2P USAGE:%d\n", E2P_USAGED);

    printf("fn_dp:%d\n", fn_dp);
    printf("cg_data_led:%d\n", cg_data_led);
    printf("net_mode:%d\n", net_mode);
}

void Read_Product_E2p(void)
{
    printf("FIRMWARE=%s\n", FIRMWARE);
    E2P_Read(SERISE_NUM_ADDR, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    printf("SerialNum=%s\n", SerialNum);
    E2P_Read(PRODUCT_ID_ADDR, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    printf("ProductId=%s\n", ProductId);
    E2P_Read(WEB_HOST_ADD, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    printf("WEB_SERVER=%s\n", WEB_SERVER);
    E2P_Read(WEB_PORT_ADD, (uint8_t *)WEB_PORT, 5);
    printf("WEB_PORT=%s\n", WEB_PORT);
    E2P_Read(MQTT_HOST_ADD, (uint8_t *)MQTT_SERVER, WEB_HOST_LEN);
    printf("MQTT_SERVER=%s\n", MQTT_SERVER);
    E2P_Read(MQTT_PORT_ADD, (uint8_t *)MQTT_PORT, 5);
    printf("MQTT_PORT=%s\n", MQTT_PORT);
    E2P_Read(CHANNEL_ID_ADD, (uint8_t *)ChannelId, CHANNEL_ID_LEN);
    printf("ChannelId=%s\n", ChannelId);
    E2P_Read(USER_ID_ADD, (uint8_t *)USER_ID, USER_ID_LEN);
    printf("USER_ID=%s\n", USER_ID);
    E2P_Read(API_KEY_ADD, (uint8_t *)ApiKey, API_KEY_LEN);
    printf("ApiKey=%s\n", ApiKey);
    E2P_Read(WIFI_SSID_ADD, (uint8_t *)wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
    E2P_Read(WIFI_PASSWORD_ADD, (uint8_t *)wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
    printf("wifi ssid=%s,wifi password=%s\n", wifi_data.wifi_ssid, wifi_data.wifi_pwd);

    E2P_Read(ETHERNET_IP_ADD, netinfo_buff, 16);
    esp_log_buffer_char(TAG, netinfo_buff, 16);
}
