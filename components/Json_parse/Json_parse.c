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
// #include "sht30dis.h"
#include "ota.h"
#include "Led.h"
#include "w5500_driver.h"
// #include "tcp_bsp.h"
#include "my_base64.h"
#include "Human.h"
#include "sound_level.h"

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

    cJSON *pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_dp"); //"fn_dp"
    if (NULL != pSubSubSub)
    {
        if ((uint32_t)pSubSubSub->valueint != fn_dp)
        {
            fn_dp = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_DP_ADD, fn_dp, 4);
            ESP_LOGI(TAG, "fn_dp = %d\r\n", fn_dp);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_sen"); //"fn_sen"
    if (NULL != pSubSubSub)
    {

        if ((uint32_t)pSubSubSub->valueint != fn_sen && (uint32_t)pSubSubSub->valueint > 0)
        {
            fn_sen = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_SEN_ADD, fn_sen, 4);
            ESP_LOGI(TAG, "fn_sen = %d\n", fn_sen);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_sen_cycle"); //"fn_sen"
    if (NULL != pSubSubSub)
    {
        if ((uint32_t)pSubSubSub->valueint != fn_sen_cycle && (uint32_t)pSubSubSub->valueint > 0)
        {
            fn_sen_cycle = (uint32_t)pSubSubSub->valueint;
            Change_Fn_sen(fn_sen_cycle);
            E2P_WriteLenByte(FN_SEN_CYCLE_ADD, fn_sen_cycle, 4);
            ESP_LOGI(TAG, "fn_sen_cycle = %d\n", fn_sen_cycle);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_sen_res"); //"fn_sen"
    if (NULL != pSubSubSub)
    {
        if ((uint32_t)pSubSubSub->valueint != fn_sen_res && (uint32_t)pSubSubSub->valueint > 0)
        {
            fn_sen_res = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_SEN_RES_ADD, fn_sen_res, 4);
            ESP_LOGI(TAG, "fn_sen_res = %d\n", fn_sen_res);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "fn_sen_sta"); //"fn_sen"
    if (NULL != pSubSubSub)
    {
        if ((uint32_t)pSubSubSub->valueint != fn_sen_sta && (uint32_t)pSubSubSub->valueint > 0)
        {
            fn_sen_sta = (uint32_t)pSubSubSub->valueint;
            E2P_WriteLenByte(FN_SEN_STA_ADD, fn_sen_sta, 4);
            ESP_LOGI(TAG, "fn_sen_sta = %d\n", fn_sen_sta);
        }
    }

    pSubSubSub = cJSON_GetObjectItem(pJsonJson, "cg_data_led"); //"cg_data_led"
    if (NULL != pSubSubSub)
    {

        if ((uint8_t)pSubSubSub->valueint != cg_data_led)
        {
            cg_data_led = (uint8_t)pSubSubSub->valueint;
            E2P_WriteLenByte(CG_DATA_LED_ADD, cg_data_led, 1);
            ESP_LOGI(TAG, "cg_data_led = %d\n", cg_data_led);
        }
    }

    // pSubSubSub = cJSON_GetObjectItem(pJsonJson, "net_mode"); //"net_mode"
    // if (NULL != pSubSubSub)
    // {
    //     if ((uint8_t)pSubSubSub->valueint != net_mode)
    //     {
    //         net_mode = (uint8_t)pSubSubSub->valueint;
    //         EE_byte_Write(ADDR_PAGE2, net_mode_add, net_mode); //写入net_mode
    //          ESP_LOGI(TAG, "net_mode = %d\n", net_mode);
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
    ESP_LOGI(TAG, "start_ble_parse_json：\r\n%s\n", blu_json_data);

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

    // ESP_LOGI(TAG,"start_parse_active_http_json\r\n");
    char *resp_val = NULL;
    resp_val = strstr(http_json_data, "{\"result\":\"success\",");
    if (resp_val == NULL)
    {
        ESP_LOGE("JSON", "DATA NO JSON");
        return ESP_FAIL;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL)
    {
        // ESP_LOGI(TAG,"Json Formatting error3\n");
        ESP_LOGE(TAG, "%s", http_json_data);
        cJSON_Delete(json_data_parse);
        return ESP_FAIL;
    }
    else
    {
        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "result");
        if (!(strcmp(json_data_parse_value->valuestring, "success")))
        {
            ESP_LOGI(TAG, "active:success\r\n");

            json_data_parse_time_value = cJSON_GetObjectItem(json_data_parse, "server_time");
            Server_Timer_GET(json_data_parse_time_value->valuestring);
        }
        else
        {
            ESP_LOGE(TAG, "active:error\r\n");
            cJSON_Delete(json_data_parse);
            return ESP_FAIL;
        }

        if (cJSON_GetObjectItem(json_data_parse, "channel") != NULL)
        {
            json_data_parse_channel_value = cJSON_GetObjectItem(json_data_parse, "channel");

            //ESP_LOGI(TAG,"%s\r\n", cJSON_Print(json_data_parse_channel_value));

            json_data_parse_channel_channel_write_key = cJSON_GetObjectItem(json_data_parse_channel_value, "write_key");
            json_data_parse_channel_channel_id_value = cJSON_GetObjectItem(json_data_parse_channel_value, "channel_id");
            json_data_parse_channel_metadata = cJSON_GetObjectItem(json_data_parse_channel_value, "metadata");
            json_data_parse_channel_user_id = cJSON_GetObjectItem(json_data_parse_channel_value, "user_id");

            Parse_metadata(json_data_parse_channel_metadata->valuestring);

            //写入API-KEY
            if (strcmp(ApiKey, json_data_parse_channel_channel_write_key->valuestring) != 0)
            {
                strcpy(ApiKey, json_data_parse_channel_channel_write_key->valuestring);
                ESP_LOGI(TAG, "api_key:%s\n", ApiKey);
                // E2P_Write(API_KEY_ADD, (uint8_t *)ApiKey, API_KEY_LEN);
            }

            //写入channelid
            if (strcmp(ChannelId, json_data_parse_channel_channel_id_value->valuestring) != 0)
            {
                strcpy(ChannelId, json_data_parse_channel_channel_id_value->valuestring);
                ESP_LOGI(TAG, "ChannelId:%s\n", ChannelId);
                // E2P_Write(CHANNEL_ID_ADD, (uint8_t *)ChannelId, CHANNEL_ID_LEN);
            }

            //写入user_id
            if (strcmp(USER_ID, json_data_parse_channel_user_id->valuestring) != 0)
            {
                strcpy(USER_ID, json_data_parse_channel_user_id->valuestring);
                ESP_LOGI(TAG, "USER_ID:%s\n", USER_ID);
                // E2P_Write(USER_ID_ADD, (uint8_t *)USER_ID, USER_ID_LEN);
            }
        }
    }
    cJSON_Delete(json_data_parse);
    return ESP_OK;
}

//解析http-post返回数据
esp_err_t parse_objects_http_respond(char *http_json_data)
{
    cJSON *json_data_parse = NULL;
    cJSON *json_data_parse_value = NULL;
    // cJSON *json_data_parse_errorcode = NULL;

    char *resp_val = NULL;
    resp_val = strstr(http_json_data, "{\"result\":\"success\",\"server_time\":");
    if (resp_val == NULL)
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        return ESP_FAIL;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL)
    {

        // ESP_LOGI(TAG,"Json Formatting error3\n");
        ESP_LOGE(TAG, "%s", http_json_data);
        cJSON_Delete(json_data_parse);
        return ESP_FAIL;
    }
    else
    {
        json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "metadata");
        if (json_data_parse_value != NULL)
        {
            // ESP_LOGI(TAG,"metadata: %s\n", json_data_parse_value->valuestring);
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
            if (mqtt_json != NULL)
            {
                ESP_LOGI(TAG, "%s", mqtt_json);
                parse_objects_mqtt(mqtt_json, false); //parse mqtt
                cJSON_free(mqtt_json);
            }
        }

        // json_data_parse_value = cJSON_GetObjectItem(json_data_parse, "cali"); //cali
        // if (NULL != json_data_parse_value)
        // {
        //     ESP_LOGI(TAG, "cali: %s\n", json_data_parse_value->valuestring);
        //     // Parse_cali_dat(json_data_parse_value->valuestring); //parse cali
        // }
    }

    cJSON_Delete(json_data_parse);
    return ESP_OK;
}

esp_err_t parse_objects_heart(char *json_data)
{
    cJSON *json_data_parse = NULL;
    cJSON *json_data_parse_value = NULL;

    char *resp_val = NULL;
    resp_val = strstr(json_data, "{\"result\":\"success\",\"server_time\":");
    if (resp_val == NULL)
    {
        ESP_LOGE("JSON", "DATA NO JSON");
        return ESP_FAIL;
    }
    json_data_parse = cJSON_Parse(resp_val);

    if (json_data_parse == NULL) //如果数据包不为JSON则退出
    {

        ESP_LOGE(TAG, "Json Formatting error4\n");

        cJSON_Delete(json_data_parse);
        return ESP_FAIL;
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
            ESP_LOGE(TAG, "active:error\r\n");
            cJSON_Delete(json_data_parse);
            return ESP_FAIL;
        }
    }
    cJSON_Delete(json_data_parse);
    return ESP_OK;
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
        ESP_LOGI(TAG, "Json Formatting error5\n");
        cJSON_Delete(json_data_parse);
        return 0;
    }

    cJSON *pSubSubSub = cJSON_GetObjectItem(json_data_parse, "command_id"); //
    if (pSubSubSub != NULL)
    {
        memset(mqtt_json_s.mqtt_command_id, 0, sizeof(mqtt_json_s.mqtt_command_id));
        strncpy(mqtt_json_s.mqtt_command_id, pSubSubSub->valuestring, strlen(pSubSubSub->valuestring));
        post_status = POST_NORMAL;
        if (Binary_dp != NULL)
        {
            xTaskNotifyGive(Binary_dp);
        }
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

// uint16_t Create_Status_Json(char *status_buff, bool filed_flag)
// {
//     uint8_t mac_sys[6] = {0};
//     char *ssid64_buff;
//     esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC

//     if (net_mode == NET_WIFI)
//     {
//         ssid64_buff = (char *)malloc(64);
//         memset(ssid64_buff, 0, 64);
//         base64_encode(wifi_data.wifi_ssid, strlen(wifi_data.wifi_ssid), ssid64_buff, 64);

//         snprintf(status_buff, sizeof(status_buff), "],\"status\":\"mac=%02x:%02x:%02x:%02x:%02x:%02x\",\"ssid_base64\":\"%s\"}",
//                  mac_sys[0],
//                  mac_sys[1],
//                  mac_sys[2],
//                  mac_sys[3],
//                  mac_sys[4],
//                  mac_sys[5],
//                  ssid64_buff);
//         free(ssid64_buff);
//     }
//     else
//     {
//         // sprintf(status_buff, "],\"status\":\"mac=%02x:%02x:%02x:%02x:%02x:%02x,ICCID=%s\"}",
//         //         mac_sys[0],
//         //         mac_sys[1],
//         //         mac_sys[2],
//         //         mac_sys[3],
//         //         mac_sys[4],
//         //         mac_sys[5],
//         //         ICCID);
//     }

//     return strlen(status_buff);
// }

void Create_NET_Json(void)
{
    if ((xEventGroupGetBits(Net_sta_group) & TIME_CAL_BIT) == TIME_CAL_BIT)
    {
        char *OutBuffer;
        char *time_buff;
        uint16_t len = 0;
        cJSON *pJsonRoot;
        wifi_ap_record_t wifidata_t;
        time_buff = (char *)malloc(24);
        Server_Timer_SEND(time_buff);
        pJsonRoot = cJSON_CreateObject();
        cJSON_AddStringToObject(pJsonRoot, "created_at", (const char *)time_buff);

        cJSON_AddItemToObject(pJsonRoot, "field1", cJSON_CreateNumber(human_status));
        if ((xEventGroupGetBits(Net_sta_group) & ACTIVED_BIT) == ACTIVED_BIT)
        {
            if (net_mode == NET_WIFI)
            {
                esp_wifi_sta_get_ap_info(&wifidata_t);
                cJSON_AddItemToObject(pJsonRoot, "field2", cJSON_CreateNumber(wifidata_t.rssi));
            }
        }

        OutBuffer = cJSON_PrintUnformatted(pJsonRoot); //cJSON_Print(Root)
        if (OutBuffer != NULL)
        {
            len = strlen(OutBuffer);
            ESP_LOGI(TAG, "len:%d\n%s\n", len, OutBuffer);

            if (xSemaphoreTake(Cache_muxtex, 10 / portTICK_RATE_MS) == pdTRUE)
            {
                DataSave((uint8_t *)OutBuffer, len);
                xSemaphoreGive(Cache_muxtex);
            }
            else
            {
                ESP_LOGE(TAG, "%d,Cache_muxtex", __LINE__);
            }

            cJSON_free(OutBuffer);
        }
        cJSON_Delete(pJsonRoot); //delete cjson root
        free(time_buff);
    }
}

uint16_t Create_Status_Json(char *status_buff, uint16_t buff_len, bool filed_flag)
{
    uint8_t mac_sys[6] = {0};
    char *ssid64_buff;
    esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC
    ssid64_buff = (char *)malloc(64);
    memset(ssid64_buff, 0, 64);
    base64_encode(wifi_data.wifi_ssid, strlen(wifi_data.wifi_ssid), ssid64_buff, 64);

    snprintf(status_buff, buff_len, "],\"status\":\"mac=%02X:%02X:%02X:%02X:%02X:%02X\"",
             mac_sys[0],
             mac_sys[1],
             mac_sys[2],
             mac_sys[3],
             mac_sys[4],
             mac_sys[5]);
    if (net_mode == NET_WIFI)
    {
        snprintf(status_buff + strlen(status_buff), buff_len - strlen(status_buff), ",\"ssid_base64\":\"%s\"", ssid64_buff);
    }

    snprintf(status_buff + strlen(status_buff), buff_len - strlen(status_buff), "}");

    free(ssid64_buff);

    return strlen(status_buff);
}

esp_err_t ParseTcpUartCmd(char *pcCmdBuffer, ...)
{
    char *Ret_OK = "{\"status\":0,\"code\": 0}\r\n";
    char *Ret_Fail = "{\"status\":1,\"code\":1}\r\n";

    va_list argp;
    /*argp指向传入的第一个可选参数，msg是最后一个确定的参数*/
    va_start(argp, pcCmdBuffer);
    int sock = 0;
    int tcp_flag;
    tcp_flag = va_arg(argp, int);
    if (tcp_flag == 1)
    {
        sock = va_arg(argp, int);
        ESP_LOGI(TAG, "sock: %d\n", sock);
    }
    va_end(argp);

    esp_err_t ret = ESP_FAIL;
    if (NULL == pcCmdBuffer) //null
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        ret = ESP_FAIL;
        return ret;
    }

    cJSON *pJson = cJSON_Parse(pcCmdBuffer); //parse json data
    if (NULL == pJson)
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        cJSON_Delete(pJson); //delete pJson
        ret = ESP_FAIL;
        return ret;
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
                        ESP_LOGI(TAG, "ProductID= %s, len= %d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(PRODUCT_ID_ADDR, (uint8_t *)pSub->valuestring, PRODUCT_ID_LEN); //save ProductID
                        // E2P_Write(PRODUCT_ID_ADDR + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, PRODUCT_ID_LEN); //save ProductID
                    }

                    pSub = cJSON_GetObjectItem(pJson, "SeriesNumber"); //"SeriesNumber"
                    if (NULL != pSub)
                    {
                        ESP_LOGI(TAG, "SeriesNumber= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(SERISE_NUM_ADDR, (uint8_t *)pSub->valuestring, SERISE_NUM_LEN); //save
                        // E2P_Write(SERISE_NUM_ADDR + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, SERISE_NUM_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "Host"); //"Host"
                    if (NULL != pSub)
                    {
                        ESP_LOGI(TAG, "Host= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(WEB_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                        // E2P_Write(WEB_HOST_ADD + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "Port"); //"WEB PORT"
                    if (NULL != pSub)
                    {

                        ESP_LOGI(TAG, "Port= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(WEB_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
                        // E2P_Write(WEB_PORT_ADD + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, 5); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "MqttHost"); //"mqtt Host"
                    if (NULL != pSub)
                    {
                        ESP_LOGI(TAG, "MqttHost= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(MQTT_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                        // E2P_Write(MQTT_HOST_ADD + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
                    }

                    pSub = cJSON_GetObjectItem(pJson, "MqttPort"); //"Host"
                    if (NULL != pSub)
                    {
                        ESP_LOGI(TAG, "MqttPort= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                        E2P_Write(MQTT_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
                        // E2P_Write(MQTT_PORT_ADD + E2P_SIZE / 2, (uint8_t *)pSub->valuestring, 5); //save
                    }
                    if (tcp_flag == 1)
                    {
                        Tcp_Send(sock, Ret_OK);
                    }
                    printf("%s", Ret_OK);

                    vTaskDelay(3000 / portTICK_RATE_MS);
                    cJSON_Delete(pJson);
                    esp_restart(); //芯片复位 函数位于esp_system.h

                    ret = ESP_OK;
                }
                else
                {
                    ret = ESP_FAIL;
                    //密码错误
                    if (tcp_flag == 1)
                    {
                        Tcp_Send(sock, Ret_Fail);
                    }
                    printf("%s", Ret_Fail);
                }
            }
        }

        else if (!strcmp((char const *)pSub->valuestring, "SetupWifi")) //Command:SetupWifi
        {
            pSub = cJSON_GetObjectItem(pJson, "SSID"); //"SSID"
            if (NULL != pSub)
            {
                bzero(wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
                strcpy(wifi_data.wifi_ssid, pSub->valuestring);
                E2P_Write(WIFI_SSID_ADD, (uint8_t *)wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
                ESP_LOGI(TAG, "WIFI_SSID = %s\r\n", pSub->valuestring);
                net_mode = NET_WIFI;
                E2P_WriteOneByte(NET_MODE_ADD, net_mode); //写入net_mode
            }

            pSub = cJSON_GetObjectItem(pJson, "password"); //"password"
            if (NULL != pSub)
            {
                bzero(wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                strcpy(wifi_data.wifi_pwd, pSub->valuestring);
                E2P_Write(WIFI_PASSWORD_ADD, (uint8_t *)wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
                ESP_LOGI(TAG, "WIFI_PWD = %s\r\n", pSub->valuestring);
            }
            if (tcp_flag == 1)
            {
                Tcp_Send(sock, Ret_OK);
                //AP配置，先关闭蓝牙
                ble_app_stop();
            }
            printf("%s", Ret_OK);
            vTaskDelay(1000 / portTICK_RATE_MS);
            Net_Switch();

            //重置网络

            ret = ESP_OK;
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
                ESP_LOGI(TAG, "Port= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(WEB_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
            }

            pSub = cJSON_GetObjectItem(pJson, "MqttHost"); //"mqtt Host"
            if (NULL != pSub)
            {
                ESP_LOGI(TAG, "MqttHost= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(MQTT_HOST_ADD, (uint8_t *)pSub->valuestring, WEB_HOST_LEN); //save
            }

            pSub = cJSON_GetObjectItem(pJson, "MqttPort"); //"Host"
            if (NULL != pSub)
            {
                ESP_LOGI(TAG, "MqttPort= %s, len=%d\n", pSub->valuestring, strlen(pSub->valuestring));
                E2P_Write(MQTT_PORT_ADD, (uint8_t *)pSub->valuestring, 5); //save
            }

            if (tcp_flag == 1)
            {
                Tcp_Send(sock, Ret_OK);
            }
            printf("%s", Ret_OK);

            vTaskDelay(3000 / portTICK_RATE_MS);
            cJSON_Delete(pJson);
            esp_restart(); //芯片复位 函数位于esp_system.h

            ret = ESP_OK;
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

            if (tcp_flag == 1)
            {
                Tcp_Send(sock, Ret_OK);
                //AP配置，先关闭蓝牙
                ble_app_stop();
            }
            printf("%s", Ret_OK);
            vTaskDelay(1000 / portTICK_RATE_MS);
            //重置网络
            Net_Switch();

            ret = ESP_OK;
        }

        else if (!strcmp((char const *)pSub->valuestring, "ReadProduct")) //Command:ReadProduct
        {
            char mac_buff[18];
            char *json_temp;
            uint8_t mac_sys[6] = {0};
            cJSON *root = cJSON_CreateObject();

            esp_read_mac(mac_sys, 0); //获取芯片内部默认出厂MAC，
            snprintf(mac_buff,
                     sizeof(mac_buff),
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
            cJSON_AddItemToObject(root, "Port", cJSON_CreateString(WEB_PORT));
            cJSON_AddItemToObject(root, "MqttHost", cJSON_CreateString(MQTT_SERVER));
            cJSON_AddItemToObject(root, "MqttPort", cJSON_CreateString(MQTT_PORT));
            cJSON_AddItemToObject(root, "CHANNEL_ID", cJSON_CreateString(ChannelId));
            cJSON_AddItemToObject(root, "USER_ID", cJSON_CreateString(USER_ID));
            cJSON_AddItemToObject(root, "MAC", cJSON_CreateString(mac_buff));
            cJSON_AddItemToObject(root, "firmware", cJSON_CreateString(FIRMWARE));

            json_temp = cJSON_PrintUnformatted(root);
            if (json_temp != NULL)
            {
                if (tcp_flag == 1)
                {
                    Tcp_Send(sock, json_temp);
                }
                printf("%s\r\n", json_temp);
                cJSON_free(json_temp);
            }
            cJSON_Delete(root); //delete pJson
            ret = ESP_OK;
        }

        //{"command":"CheckEthernet"}
        else if (!strcmp((char const *)pSub->valuestring, "CheckEthernet")) //Command:ReadProduct
        {
            cJSON *root = cJSON_CreateObject();
            char *json_temp;

            if (ETH_FLAG == true)
            {
                cJSON_AddStringToObject(root, "result", "OK");
                cJSON_AddStringToObject(root, "module", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "result", "ERROR");
                cJSON_AddStringToObject(root, "module", "ERROR");
            }

            json_temp = cJSON_PrintUnformatted(root);
            if (json_temp != NULL)
            {
                if (tcp_flag == 1)
                {
                    Tcp_Send(sock, json_temp);
                }
                printf("%s\r\n", json_temp);
                cJSON_free(json_temp);
            }
            cJSON_Delete(root); //delete pJson
            ret = ESP_OK;
        }
        //{"command":"CheckSensors"}
        else if (!strcmp((char const *)pSub->valuestring, "CheckSensors"))
        {
            cJSON *root = cJSON_CreateObject();
            char *json_temp;
            char wifi_ssid[33] = {0};
            int8_t wifi_rssi;

            bool result_flag = true;
            bool WIFI_flag = false;

            // xTaskNotifyGive(Binary_485_th);
            // xTaskNotifyGive(Binary_ext);
            // xTaskNotifyGive(Binary_energy);

            if (Check_Wifi((uint8_t *)wifi_ssid, &wifi_rssi))
            {
                WIFI_flag = true;
            }
            else
            {
                result_flag = false;
                WIFI_flag = false;
            }

            if ((E2P_FLAG & HUM_FLAG) != true)
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

            if (WIFI_flag == true)
            {
                cJSON_AddStringToObject(root, "ssid", wifi_ssid);
                cJSON_AddNumberToObject(root, "rssi", wifi_rssi);
            }
            else
            {
                cJSON_AddStringToObject(root, "ssid", "NULL");
                cJSON_AddNumberToObject(root, "rssi", 0);
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

            if (HUM_FLAG == true)
            {
                cJSON_AddStringToObject(root, "Human", "OK");
            }
            else
            {
                cJSON_AddStringToObject(root, "Human", "ERROR");
            }

            json_temp = cJSON_PrintUnformatted(root);
            if (json_temp != NULL)
            {
                if (tcp_flag == 1)
                {
                    Tcp_Send(sock, json_temp);
                }
                printf("%s\r\n", json_temp);
                cJSON_free(json_temp);
            }
            cJSON_Delete(root); //delete pJson
            ret = ESP_OK;
        }
        //{"command":"ScanWifiList"}
        else if (!strcmp((char const *)pSub->valuestring, "ScanWifiList"))
        {
            Scan_Wifi();
            ret = ESP_OK;
        }
        //{"command":"reboot"}
        else if (!strcmp((char const *)pSub->valuestring, "reboot"))
        {
            esp_restart();
            ret = ESP_OK;
        }
        //{"command":"AllReset"}
        else if (!strcmp((char const *)pSub->valuestring, "AllReset"))
        {
            // E2prom_set_defaul(false);
            E2prom_set_0XFF();
            ret = ESP_OK;
        }
    }

    cJSON_Delete(pJson); //delete pJson

    return ret;
}

//读取EEPROM中的metadata
void Read_Metadate_E2p(void)
{

    fn_dp = E2P_ReadLenByte(FN_DP_ADD, 4);               //数据发送频率
    fn_sen = E2P_ReadLenByte(FN_SEN_ADD, 4);             //人感灵敏度
    fn_sen_cycle = E2P_ReadLenByte(FN_SEN_CYCLE_ADD, 4); //
    fn_sen_res = E2P_ReadLenByte(FN_SEN_RES_ADD, 4);     //
    fn_sen_sta = E2P_ReadLenByte(FN_SEN_STA_ADD, 4);     //
    cg_data_led = E2P_ReadOneByte(CG_DATA_LED_ADD);      //发送数据 LED状态 0关闭，1打开
    net_mode = E2P_ReadOneByte(NET_MODE_ADD);            //上网模式选择 0：自动模式 1：lan模式 2：wifi模式

    if (fn_sen == 0)
    {
        fn_sen = 100;
    }
    if (fn_sen_cycle == 0)
    {
        fn_sen_cycle = 1000;
    }
    if (fn_sen_res == 0)
    {
        fn_sen_res = 30000;
    }
    if (fn_sen_sta == 0)
    {
        fn_sen_sta = 300;
    }

    ESP_LOGI(TAG, "E2P USAGE:%d\n", E2P_USAGED);

    ESP_LOGI(TAG, "fn_dp:%d\n", fn_dp);
    ESP_LOGI(TAG, "fn_sen:%d\n", fn_sen);
    ESP_LOGI(TAG, "fn_sen_cycle:%d\n", fn_sen_cycle);
    ESP_LOGI(TAG, "fn_sen_res:%d\n", fn_sen_res);
    ESP_LOGI(TAG, "fn_sen_sta:%d\n", fn_sen_sta);
    ESP_LOGI(TAG, "cg_data_led:%d\n", cg_data_led);
    ESP_LOGI(TAG, "net_mode:%d\n", net_mode);
}

void Read_Product_E2p(void)
{
    ESP_LOGI(TAG, "FIRMWARE=%s\n", FIRMWARE);
    E2P_Read(SERISE_NUM_ADDR, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    ESP_LOGI(TAG, "SerialNum=%s\n", SerialNum);
    E2P_Read(PRODUCT_ID_ADDR, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    ESP_LOGI(TAG, "ProductId=%s\n", ProductId);
    E2P_Read(WEB_HOST_ADD, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    ESP_LOGI(TAG, "WEB_SERVER=%s\n", WEB_SERVER);
    E2P_Read(WEB_PORT_ADD, (uint8_t *)WEB_PORT, 5);
    ESP_LOGI(TAG, "WEB_PORT=%s\n", WEB_PORT);
    E2P_Read(MQTT_HOST_ADD, (uint8_t *)MQTT_SERVER, WEB_HOST_LEN);
    ESP_LOGI(TAG, "MQTT_SERVER=%s\n", MQTT_SERVER);
    E2P_Read(MQTT_PORT_ADD, (uint8_t *)MQTT_PORT, 5);
    ESP_LOGI(TAG, "MQTT_PORT=%s\n", MQTT_PORT);
    E2P_Read(CHANNEL_ID_ADD, (uint8_t *)ChannelId, CHANNEL_ID_LEN);
    ESP_LOGI(TAG, "ChannelId=%s\n", ChannelId);
    E2P_Read(USER_ID_ADD, (uint8_t *)USER_ID, USER_ID_LEN);
    ESP_LOGI(TAG, "USER_ID=%s\n", USER_ID);
    E2P_Read(API_KEY_ADD, (uint8_t *)ApiKey, API_KEY_LEN);
    ESP_LOGI(TAG, "ApiKey=%s\n", ApiKey);
    E2P_Read(WIFI_SSID_ADD, (uint8_t *)wifi_data.wifi_ssid, sizeof(wifi_data.wifi_ssid));
    E2P_Read(WIFI_PASSWORD_ADD, (uint8_t *)wifi_data.wifi_pwd, sizeof(wifi_data.wifi_pwd));
    ESP_LOGI(TAG, "wifi ssid=%s,wifi password=%s\n", wifi_data.wifi_ssid, wifi_data.wifi_pwd);

    E2P_Read(ETHERNET_IP_ADD, netinfo_buff, 16);
    esp_log_buffer_char(TAG, netinfo_buff, 16);
}
