#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lan_mqtt.h"
#include "w5500_socket.h"
//#include "MQTTConnect.h"
#include "MQTTPacket.h"
#include "w5500_driver.h"
#include "Json_parse.h"

#define TAG "LAN_MQTT"
/*
	brief: include DNS,connect mqtt,subscribe，publish，and receive server data.
	parameter: pvParameter
	return : NONE
*/

MQTTPacket_connectData user_MQTTPacket_ConnectData;
uint8_t mqtt_buff[2048]; //空间不够，可能会导致接收数据时返回 -1
uint8_t lan_mqtt_status = 0;

static int transport_getdata(uint8_t *buf, int count)
{
    return lan_recv(MQTT_SOCKET, buf, count);
}
void lan_mqtt_init(void)
{
    user_MQTTPacket_ConnectData.cleansession = 1;
    user_MQTTPacket_ConnectData.keepAliveInterval = 60;
    user_MQTTPacket_ConnectData.MQTTVersion = 3;    //mqtt version
    user_MQTTPacket_ConnectData.struct_version = 0; //must be 0
    user_MQTTPacket_ConnectData.username.cstring = MQTT_USER;
    user_MQTTPacket_ConnectData.password.cstring = MQTT_PASS;
    user_MQTTPacket_ConnectData.clientID.cstring = MQTT_CLIEND_ID;
}
uint8_t mqtt_remote_ip[4];
/*mqtt连接函数
	参数：空
	返回：1：连接成功，0连接失败
*/
static uint8_t mqtt_connect()
{
    uint8_t SessionFlg, ConnAckFlg;
    uint8_t count = 0;
    int32_t ret;
    int rc;
    while (1)
    {
        rc = MQTTSerialize_connect(mqtt_buff, sizeof(mqtt_buff), &user_MQTTPacket_ConnectData);
        if (rc == 0)
        {
            ESP_LOGE(TAG, "MQTT> serialize connect packet error: %d.\r\n", rc);
            return 0;
        }
        ESP_LOGI(TAG, "MQTT> serialize packet length : %d.\r\n", rc);
        ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
        if (ret != rc)
        {
            ESP_LOGE(TAG, "MQTT> send error:%d.rc:%d.\r\n", ret, rc);
            return 0;
        }
        //等待应答信号
        do
        {
            while (MQTTPacket_read(mqtt_buff, sizeof(mqtt_buff), transport_getdata) != CONNACK)
            {
                vTaskDelay(500 / portTICK_RATE_MS);
                count++;
                if (count > 10)
                {
                    count = 0;
                    ESP_LOGE(TAG, "MQTT> connect timerout.\r\n");
                    return 0;
                }
            }
        } while ((MQTTDeserialize_connack(&SessionFlg, &ConnAckFlg, mqtt_buff, sizeof(mqtt_buff)) != 1 || ConnAckFlg != 0));
        return 1;
    }
}
/*mqtt订阅主题
	参数：主题
	返回：1:成功,0:失败
*/

/**************************************************/
static uint8_t mqtt_subscribe(char *Topic)
{
    int msgid = 1;
    int req_qos = 0;
    int granted_qos;
    int subcount;
    unsigned short submsgid;
    int32_t rc, ret;
    uint8_t count = 0;

    MQTTString topicString = MQTTString_initializer;

    topicString.cstring = (char *)Topic;

    rc = MQTTSerialize_subscribe(mqtt_buff, sizeof(mqtt_buff), 0, msgid, 1, &topicString, &req_qos); //构建订阅报文

    if (rc < 0)
    {
        ESP_LOGE(TAG, "MQTT> serialize subscribe error.\r\n");
        return 0;
    }

    ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
    if (ret != rc)
    {
        ESP_LOGE(TAG, "MQTT> send subscribe error.\r\n");
    }
    do
    {
        while (MQTTPacket_read(mqtt_buff, sizeof(mqtt_buff), transport_getdata) != SUBACK)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            if (++count > 30)
            {
                count = 0;
                ESP_LOGE(TAG, "MQTT> wait suback timerout.\r\n");
                return 0;
            }
        }
        ESP_LOGD(TAG, "MQTT> MQTTPacket read SUBACK.\r\n");
        rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, mqtt_buff, sizeof(mqtt_buff));
    } while (granted_qos != 0);
    return 1;
}

/*
	MQTT 发送ping报文
	参数：空
	返回：1：发送成功，0：发送失败
*/
static uint8_t mqtt_ping()
{
    int rc;
    int32_t ret;

    rc = MQTTSerialize_pingreq(mqtt_buff, sizeof(mqtt_buff));
    ret = lan_send(MQTT_SOCKET, mqtt_buff, rc);
    // printf("rc = %d, ret = %d \n", rc, ret);
    if (rc != ret)
    {
        ESP_LOGE(TAG, "MQTT> ping send error.\r\n");
        return 0;
    }
    return 1;
}
/*mqtt运行任务，当tcp成功建立链接时，恢复该任务，否则是挂起状态
当LINK断开时，挂起该任务
该任务建立在TCP成功连接的基础上：连接mqtt服务器->订阅相关主题->
返回：正常情况阻塞任务，异常返回错误类型
*/
static uint8_t mqtt()
{
    uint8_t *payload_in;
    int ret;
    uint8_t dup, retained;
    uint16_t mssageid;
    int qos, rc, payloadlen_in;
    MQTTString topoc;
    // uint8_t msgbuf[1024];
    //topoc.cstring = "fdj/iot/control/12345";
    if (mqtt_connect()) //连接服务器
    {
        ESP_LOGI(TAG, "MQTT> connected.\r\n");
        memset(mqtt_buff, 0, sizeof(mqtt_buff));

        if (mqtt_subscribe(MQTT_Topic)) //订阅
        {
            ESP_LOGI(TAG, "MQTT> subscribe success.\r\n");
            while (1) //开始接收数据，阻塞任务
            {
                if (LAN_DNS_STATUS == 0)
                {
                    ESP_LOGE(TAG, "有线网断开，退出MQTT任务.\r\n");
                    return 0;
                }

                memset(mqtt_buff, 0, sizeof(mqtt_buff));
                if (!mqtt_ping()) //发送心跳包
                {
                    ESP_LOGE(TAG, "MQTT> ping error.\r\n");
                    return 0;
                }
                memset(mqtt_buff, 0, sizeof(mqtt_buff));
                ret = MQTTPacket_read(mqtt_buff, sizeof(mqtt_buff), transport_getdata);
                if (ret == -1) //收到数值大于变量值
                {
                    printf("收到数值大于数组空间，read return : %d\n", ret);
                }
                if (ret != PINGRESP) //不是心跳返回
                {
                    printf("read return : %d\n", ret);
                }

                if (ret == PUBLISH)
                {
                    printf("Recved !\n");
                    rc = MQTTDeserialize_publish(&dup, &qos, &retained, &mssageid, &topoc,
                                                 &payload_in, &payloadlen_in, mqtt_buff, sizeof(mqtt_buff));

                    parse_objects_mqtt((char *)mqtt_buff); //收到平台MQTT数据并解析
                    printf("message arrived %d: %s\n\r", payloadlen_in, payload_in);
                }
                vTaskDelay(100 / portTICK_RATE_MS);
            }
        }
        else
        {
            ESP_LOGE(TAG, "MQTT> subscribe failed.\r\n");
        }
        ESP_LOGI(TAG, "-----------test end---------------\r\n");
        return 0;
    }
    else
    {
        ESP_LOGE(TAG, "MQTT> connect fail.\r\n");
        return 0;
    }
}

void lan_mqtt_task(void *pvParameter)
{
    int8_t ret;
    // uint8_t mqtt_state = 1;
    int32_t rc;
    while (1)
    {

        switch (getSn_SR(MQTT_SOCKET))
        {
        case SOCK_CLOSED:
            ESP_LOGD(TAG, "MQTT> SOCK_CLOSE.\r\n");
            if (LAN_DNS_STATUS == 0)
            {
                lan_mqtt_status = 0;
                ESP_LOGI(TAG, "vTaskDelete LAN_MQTT!\r\n");
                vTaskDelete(NULL); //网线断开，删除任务
            }
            if ((ret = lan_socket(MQTT_SOCKET, Sn_MR_TCP, 3000, 0)) != MQTT_SOCKET)
            {
                ESP_LOGE(TAG, "MQTT> socket open failed : %d.\r\n", ret);
                break;
            }
            break;
        case SOCK_ESTABLISHED: //TCP正常建立连接
            ESP_LOGI(TAG, "MQTT> tcp connnect.\r\n");
            //MQTT 连接封包
            rc = mqtt(); //阻塞任务，除非mqtt过程出现异常
            if (rc == 0)
            {
                ESP_LOGE(TAG, "MQTT> ERROR.reopen socket.\r\n");
                lan_close(MQTT_SOCKET);
            }
            break;
        case SOCK_CLOSE_WAIT:
            ESP_LOGD(TAG, "MQTT> SOCK_CLOSE_WAIT.\r\n");
            break;
        case SOCK_INIT:
            ESP_LOGD(TAG, "MQTT> socket state SOCK_INIT.\r\n");
            if ((ret = (uint32_t)lan_connect(MQTT_SOCKET, dns_host_ip, MQTT_PORT)) != SOCK_OK)
            {
                ESP_LOGE(TAG, "MQTT> socket connect faile : %d.\r\n", ret);
                break;
            }

            break;

        default:
            ESP_LOGI(TAG, "MQTT> unknow mqtt socke SR:%x.\r\n", getSn_SR(MQTT_SOCKET));
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void start_lan_mqtt(void)
{
    printf("start_lan_mqtt!\n");
    lan_mqtt_status = 1;
    xTaskCreate(lan_mqtt_task, "lan_mqtt_task", 8192, NULL, 2, NULL); //开启 LAN_MQTT
}