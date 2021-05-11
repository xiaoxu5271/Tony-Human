#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"
// #include "platform.h"

#include "w5500_socket.h"
// #include "MQTTConnect.h"
#include "MQTTPacket.h"
#include "w5500_driver.h"
#include "Json_parse.h"
#include "Smartconfig.h"
#include "Http.h"
#include "My_Mqtt.h"
#include "E2prom.h"

#include "lan_mqtt.h"
#define TAG "LAN_MQTT"

#define MQTT_LEN 2048
#define RECV_LEN 2048

TaskHandle_t lan_mqtt_Handle = NULL;
MQTTPacket_connectData user_MQTTPacket_ConnectData;

static int transport_getdata(uint8_t *buf, int count)
{
    return lan_recv(MQTT_SOCKET, buf, count);
}

uint8_t mqtt_remote_ip[4];
/*mqtt连接函数
	参数：空
	返回：1：连接成功，0连接失败
*/
static uint8_t mqtt_connect()
{
    uint8_t *mqtt_buff = (uint8_t *)malloc(MQTT_LEN);
    uint8_t SessionFlg, ConnAckFlg;
    uint8_t count = 0;
    int32_t ret;
    int rc;
    while (1)
    {
        rc = MQTTSerialize_connect(mqtt_buff, MQTT_LEN, &user_MQTTPacket_ConnectData);
        if (rc == 0)
        {
            free(mqtt_buff);
            ESP_LOGE(TAG, "MQTT> serialize connect packet error: %d.\r\n", rc);
            return 0;
        }
        ESP_LOGI(TAG, "MQTT> serialize packet length:%d,%s", rc, mqtt_buff);
        ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
        if (ret != rc)
        {
            free(mqtt_buff);
            ESP_LOGE(TAG, "MQTT> send error:%d.rc:%d.\r\n", ret, rc);
            return 0;
        }
        //等待应答信号
        do
        {
            while (MQTTPacket_read(mqtt_buff, MQTT_LEN, transport_getdata) != CONNACK)
            {
                vTaskDelay(500 / portTICK_RATE_MS);
                count++;
                if (count > 10)
                {
                    count = 0;
                    ESP_LOGE(TAG, "MQTT> connect timerout.\r\n");
                    free(mqtt_buff);
                    return 0;
                }
            }
        } while ((MQTTDeserialize_connack(&SessionFlg, &ConnAckFlg, mqtt_buff, MQTT_LEN) != 1 || ConnAckFlg != 0));
        free(mqtt_buff);
        return 1;
    }
}

/*mqtt发布主题
	参数：主题
	返回：1:成功,0:失败
*/
uint8_t mqtt_publish(char *Topic, char *msg, uint16_t len)
{
    uint8_t *mqtt_buff = (uint8_t *)malloc(MQTT_LEN);
    int msglen = strlen(msg);
    int32_t rc, ret;
    int qos = 1; //设置成1 可以正常收发
    uint16_t count = 0;
    MQTTString topicString = MQTTString_initializer;

    topicString.cstring = Topic;
    rc = MQTTSerialize_publish(mqtt_buff, MQTT_LEN, 0, qos, 0, 0, topicString, (unsigned char *)msg, msglen);
    if (rc < 0)
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        free(mqtt_buff);
        return 0;
    }

    ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
    if (ret != rc)
    {
        ESP_LOGE(TAG, "%d", __LINE__);
        free(mqtt_buff);
        return 0;
    }

    //阻塞等待接收数据 5s超时
    while (getSn_RX_RSR(MQTT_SOCKET) == 0)
    {
        count++;
        if (count > 500)
        {
            count = 0;
            ESP_LOGE(TAG, "MQTT> publish recv error.\r\n");
            free(mqtt_buff);
            return -2;
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    if ((ret = MQTTPacket_read(mqtt_buff, MQTT_LEN, transport_getdata)) != PUBACK)
    {
        ESP_LOGE(TAG, "publish recv %d", ret);
        free(mqtt_buff);
        return -3;
    }
    free(mqtt_buff);
    return 1;
}

/*mqtt订阅主题
	参数：主题
	返回：1:成功,0:失败
*/

/**************************************************/
static uint8_t mqtt_subscribe(char *Topic)
{
    uint8_t *mqtt_buff = (uint8_t *)malloc(MQTT_LEN);
    int msgid = 1;
    int req_qos = 0;
    int granted_qos;
    int subcount;
    unsigned short submsgid;
    int32_t rc, ret;
    uint8_t count = 0;

    MQTTString topicString = MQTTString_initializer;

    topicString.cstring = (char *)Topic;

    rc = MQTTSerialize_subscribe(mqtt_buff, MQTT_LEN, 0, msgid, 1, &topicString, &req_qos); //构建订阅报文

    if (rc < 0)
    {
        ESP_LOGE(TAG, "MQTT> serialize subscribe error.\r\n");
        free(mqtt_buff);
        return 0;
    }

    ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
    if (ret != rc)
    {
        ESP_LOGE(TAG, "MQTT> send subscribe error.\r\n");
    }
    do
    {
        while (MQTTPacket_read(mqtt_buff, MQTT_LEN, transport_getdata) != SUBACK)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            if (++count > 30)
            {
                count = 0;
                ESP_LOGE(TAG, "MQTT> wait suback timerout.\r\n");
                free(mqtt_buff);
                return 0;
            }
        }
        ESP_LOGD(TAG, "MQTT> MQTTPacket read SUBACK.\r\n");
        rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, mqtt_buff, MQTT_LEN);
    } while (granted_qos != 0);
    free(mqtt_buff);
    return 1;
}

/*
	MQTT 发送ping报文
	参数：空
	返回：1：发送成功，0：发送失败
*/
static int8_t mqtt_ping()
{
    uint8_t *mqtt_buff = (uint8_t *)malloc(MQTT_LEN);
    int rc;
    int32_t ret;
    uint32_t count = 0;

    rc = MQTTSerialize_pingreq(mqtt_buff, MQTT_LEN);
    ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);

    // printf("rc = %d, ret = %d \n", rc, ret);
    if (rc != ret)
    {
        ESP_LOGE(TAG, "MQTT> ping send error.\r\n");
        free(mqtt_buff);
        return -1;
    }

    //阻塞等待接收数据 1s超时
    while (getSn_RX_RSR(MQTT_SOCKET) == 0)
    {
        count++;
        if (count > 500)
        {
            count = 0;
            ESP_LOGE(TAG, "MQTT> ping recv error.\r\n");
            free(mqtt_buff);
            return -2;
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    if (MQTTPacket_read(mqtt_buff, MQTT_LEN, transport_getdata) != PINGRESP)
    {
        ESP_LOGE(TAG, "MQTT> ping recv not PINGRESP.\r\n");
        free(mqtt_buff);
        return -3;
    }
    free(mqtt_buff);
    return 1;
}

// void  lan_mqtt_process_receive()
// {

// }

/*mqtt运行任务，当tcp成功建立链接时，恢复该任务，否则是挂起状态
当LINK断开时，挂起该任务
该任务建立在TCP成功连接的基础上：连接mqtt服务器->订阅相关主题->
返回：正常情况阻塞任务，异常返回错误类型
*/

static int8_t mqtt()
{
    uint8_t *mqtt_buff = (uint8_t *)malloc(MQTT_LEN);
    uint8_t *msg_rev_buf = (uint8_t *)malloc(RECV_LEN);
    uint8_t *payload_in;
    int ret;
    uint8_t dup, retained;
    uint16_t mssageid;
    int qos, payloadlen_in;
    MQTTString topoc;
    uint32_t ping_timeout = 0;

    creat_json Http_Post_Buff;

    if (mqtt_connect()) //连接服务器
    {
        ESP_LOGI(TAG, "MQTT> connected.\r\n");
        memset(mqtt_buff, 0, 2048);
        MQTT_E_STA = true;
        if (mqtt_subscribe(topic_s)) //订阅
        {
            ESP_LOGI(TAG, "MQTT> subscribe success.\r\n");
            while (1) //开始接收数据，阻塞任务
            {
                //延时阻塞接收数据
                ret = getSn_RX_RSR(MQTT_SOCKET);

                //判断是否收到数据
                if (ret > 0)
                {
                    memset(mqtt_buff, 0, MQTT_LEN);
                    ret = MQTTPacket_read(mqtt_buff, MQTT_LEN, transport_getdata);
                    if (ret == PUBLISH)
                    {
                        MQTTDeserialize_publish(&dup, &qos, &retained, &mssageid, &topoc,
                                                &payload_in, &payloadlen_in, mqtt_buff, MQTT_LEN);

                        strcpy((char *)msg_rev_buf, (const char *)payload_in);
                        ESP_LOGI(TAG, "message arrived %d: %s\n\r", payloadlen_in, (char *)msg_rev_buf);
                        parse_objects_mqtt((char *)msg_rev_buf, false); //收到平台MQTT数据并解析

                        // HighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                        // printf("MQTT-TASK HighWaterMark=%d \n", HighWaterMark);
                        //心跳计时清零
                        ping_timeout = 0;
                    }
                    else if (ret == -1)
                    {
                        ESP_LOGE(TAG, "MQTTPacket_read ERR : %d\n", ret);
                        free(msg_rev_buf);
                        free(mqtt_buff);
                        return -1;
                    }
                }
                //定时发送心跳包，单位S
                else if (ping_timeout > KEEPLIVE_TIME)
                {
                    ping_timeout = 0;
                    //发送心跳包
                    if ((ret = mqtt_ping()) < 0)
                    {
                        ESP_LOGE(TAG, "MQTT> ping error code:%d.\r\n", ret);
                        free(msg_rev_buf);
                        free(mqtt_buff);
                        return 0;
                    }
                }
                else
                {
                    if (xQueueReceive(Send_Mqtt_Queue, &Http_Post_Buff, 0) == pdPASS)
                    {
                        ESP_LOGI(TAG, "%d", __LINE__);
                        mqtt_publish(topic_p, Http_Post_Buff.creat_json_buff, Http_Post_Buff.creat_json_len);
                    }
                }

                ping_timeout++;
                vTaskDelay(10 / portTICK_RATE_MS);
            }
        }
        else
        {
            ESP_LOGE(TAG, "MQTT> subscribe failed.\r\n");
        }
        free(msg_rev_buf);
        free(mqtt_buff);
        return 0;
    }
    else
    {
        ESP_LOGE(TAG, "MQTT> connect fail.\r\n");
        free(msg_rev_buf);
        free(mqtt_buff);
        return 0;
    }
}

void lan_mqtt_task(void *pvParameter)
{
    xEventGroupSetBits(Net_sta_group, MQTT_E_S_BIT);

    int8_t ret;
    // uint8_t mqtt_state = 1;
    int32_t rc;
    uint8_t fail_num = 0;
    uint8_t MQTT_IP[4];

    user_MQTTPacket_ConnectData.cleansession = 1;
    user_MQTTPacket_ConnectData.keepAliveInterval = 60;
    user_MQTTPacket_ConnectData.MQTTVersion = 3;    //mqtt version
    user_MQTTPacket_ConnectData.struct_version = 0; //must be 0
    user_MQTTPacket_ConnectData.username.cstring = mqtt_usr;
    user_MQTTPacket_ConnectData.password.cstring = mqtt_pwd;
    user_MQTTPacket_ConnectData.clientID.cstring = MQTT_CLIEND_ID;
    // user_MQTTPacket_ConnectData.cleansession = 1;
    ESP_LOGI(TAG, "mqtt_usr:%s,mqtt_usr:%s", user_MQTTPacket_ConnectData.username.cstring, user_MQTTPacket_ConnectData.password.cstring);

    xEventGroupWaitBits(Net_sta_group, CONNECTED_BIT, false, true, -1); //等待网络连接

    if (lan_dns_resolve((uint8_t *)MQTT_SERVER, MQTT_IP) == false)
    {
        ESP_LOGE(TAG, "lan dns resolve FAIL!\r\n");
        vTaskDelete(NULL); //网线断开，删除任务
    }
    ESP_LOGI(TAG, "MQTT_IP:%d.%d.%d.%d\r\n", MQTT_IP[0], MQTT_IP[1], MQTT_IP[2], MQTT_IP[3]);
    while (1)
    {
        xEventGroupWaitBits(Net_sta_group, CONNECTED_BIT, false, true, -1); //等待网络连接
        xEventGroupWaitBits(Net_sta_group, ACTIVED_BIT, false, true, -1);   //等待网络连接
        switch (getSn_SR(MQTT_SOCKET))
        {
        case SOCK_CLOSED:
            ESP_LOGI(TAG, "MQTT> SOCK_CLOSE.\r\n");
            if (net_mode != NET_LAN)
            {
                MQTT_E_STA = false;
                ESP_LOGI(TAG, "vTaskDelete LAN_MQTT!\r\n");
                xEventGroupClearBits(Net_sta_group, MQTT_E_S_BIT);
                vTaskDelete(NULL);
            }
            else if ((ret = lan_socket(MQTT_SOCKET, Sn_MR_TCP, 3000, 0)) != MQTT_SOCKET)
            {
                ESP_LOGE(TAG, "MQTT> socket open failed : %d.\r\n", ret);
                break;
            }
            break;
        case SOCK_ESTABLISHED: //TCP正常建立连接
            ESP_LOGI(TAG, "MQTT> tcp connnect.\r\n");
            //MQTT 连接封包
            rc = mqtt(); //阻塞任务，除非mqtt过程出现异常
            MQTT_E_STA = false;
            if (rc == 0)
            {
                ESP_LOGE(TAG, "MQTT> ERROR.reopen socket.\r\n");
            }
            lan_close(MQTT_SOCKET);
            fail_num = 0;
            break;
        case SOCK_CLOSE_WAIT:
            lan_close(MQTT_SOCKET);
            ESP_LOGI(TAG, "MQTT> SOCK_CLOSE_WAIT.\r\n");
            break;
        case SOCK_INIT:
            ESP_LOGI(TAG, "MQTT> socket state SOCK_INIT.\r\n");
            if ((ret = (uint32_t)lan_connect(MQTT_SOCKET, MQTT_IP, (uint16_t)strtoul(MQTT_PORT, 0, 10))) != SOCK_OK)
            {
                ESP_LOGE(TAG, "MQTT> socket connect faile : %d.\r\n", ret);
                break;
            }
            break;

        default:
            ESP_LOGE(TAG, "MQTT> unknow mqtt socke SR:%x.\r\n", getSn_SR(MQTT_SOCKET));
            fail_num++;
            if (fail_num >= 10)
            {
                fail_num = 0;
                // DHCP_INIT = true;
                RJ45_MODE = RJ45_INIT;
                ESP_LOGE(TAG, "fail time out getSn_SR=0x%02x\n", getSn_SR(MQTT_SOCKET));
                lan_close(MQTT_SOCKET);
            }
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void start_lan_mqtt(void)
{
    if ((xEventGroupGetBits(Net_sta_group) & (MQTT_E_S_BIT | MQTT_INITED_BIT)) == MQTT_INITED_BIT) //设备激活过，但MQTT未启动
    {
        ESP_LOGI(TAG, "start_lan_mqtt!\n");
        xTaskCreate(lan_mqtt_task, "lan_mqtt_task", 4096, NULL, 10, NULL);
    }
}

void stop_lan_mqtt(void)
{
    if ((xEventGroupGetBits(Net_sta_group) & MQTT_E_S_BIT) == MQTT_E_S_BIT)
    {
        ESP_LOGI(TAG, "stop lan mqtt!\n");
        lan_close(MQTT_SOCKET);
    }
}