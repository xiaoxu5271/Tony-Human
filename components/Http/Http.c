#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "nvs.h"
#include "Json_parse.h"
#include "E2prom.h"
#include "Bluetooth.h"
#include "Human.h"
#include "Led.h"
#include "Smartconfig.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "freertos/event_groups.h"
#include "w5500_driver.h"
#include "my_base64.h"
#include "Http.h"

TaskHandle_t Binary_Heart_Send = NULL;
TaskHandle_t Binary_dp = NULL;

TaskHandle_t Active_Task_Handle = NULL;
// extern uint8_t data_read[34];

static char *TAG = "HTTP";
uint8_t post_status = POST_NOCOMMAND;
bool fn_dp_flag = true;

esp_timer_handle_t http_timer_suspend_p = NULL;

void timer_heart_cb(void *arg);
esp_timer_handle_t timer_heart_handle = NULL; //定时器句柄
esp_timer_create_args_t timer_heart_arg = {
    .callback = &timer_heart_cb,
    .arg = NULL,
    .name = "Heart_Timer"};

//1min 定时，用来触发各组数据采集/发送
void timer_heart_cb(void *arg)
{
    vTaskNotifyGiveFromISR(Binary_Heart_Send, NULL);
    static uint64_t min_num = 0;
    min_num++;
    if (fn_dp)
        if (min_num * 60 % fn_dp == 0)
        {
            fn_dp_flag = true;
            vTaskNotifyGiveFromISR(Binary_dp, NULL);
        }
}

int32_t wifi_http_send(char *send_buff, uint16_t send_size, char *recv_buff, uint16_t recv_size)
{
    // printf("wifi http send start!\n");
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int32_t s = 0, r = 0;

    int err = getaddrinfo((const char *)WEB_SERVER, "80", &hints, &res); //step1：DNS域名解析

    if (err != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return -1;
    }

    /* Code to print the resolved IP.
		Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0); //step2：新建套接字
    if (s < 0)
    {
        ESP_LOGE(TAG, "... Failed to allocate socket. err:%d", s);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return -1;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) //step3：连接IP
    {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return -1;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    ESP_LOGD(TAG, "http_send_buff send_buff: %s\n", (char *)send_buff);
    if (write(s, (char *)send_buff, send_size) < 0) //step4：发送http包
    {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return -1;
    }
    ESP_LOGI(TAG, "... socket send success");
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, //step5：设置接收超时
                   sizeof(receiving_timeout)) < 0)
    {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(s);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return -1;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    /* Read HTTP response */

    bzero((uint16_t *)recv_buff, recv_size);
    r = read(s, (uint16_t *)recv_buff, recv_size);
    ESP_LOGD(TAG, "r=%d,activate recv_buf=%s\r\n", r, (char *)recv_buff);
    close(s);
    // printf("http send end!\n");
    return r;
}

int32_t http_send_buff(char *send_buff, uint16_t send_size, char *recv_buff, uint16_t recv_size)
{
    xSemaphoreTake(xMutex_Http_Send, -1);

    int32_t ret;
    if (net_mode == NET_LAN)
    {
        ESP_LOGI(TAG, "lan send!!!\n");
        ret = lan_http_send(send_buff, send_size, recv_buff, recv_size);
        // printf("lan_http_send return :%d\n", ret);
        xSemaphoreGive(xMutex_Http_Send);
        return ret;
    }

    else
    {
        ESP_LOGI(TAG, "wifi send!!!\n");
        ret = wifi_http_send(send_buff, send_size, recv_buff, recv_size);
        xSemaphoreGive(xMutex_Http_Send);
        return ret;
    }
}

void http_get_task(void *pvParameters)
{

    while (1)
    {
        xEventGroupWaitBits(Net_sta_group, ACTIVED_BIT, false, true, -1);
        http_send_mes();

        ulTaskNotifyTake(pdTRUE, -1);
    }
}

void http_send_mes(void)
{
    int ret = 0;
    char recv_buf[1024] = {0};
    char build_po_url[512] = {0};
    char build_po_url_json[1024] = {0};
    char NET_INFO[64] = {0};

    if (LAN_DNS_STATUS == 1)
    {
        sprintf(NET_INFO, "&net=ethernet");
    }

    creat_json *pCreat_json1 = malloc(sizeof(creat_json)); //为 pCreat_json1 分配内存  动态内存分配，与free() 配合使用
    //创建POST的json格式
    create_http_json(pCreat_json1, fn_dp_flag);
    fn_dp_flag = false;

    if (post_status == POST_NOCOMMAND) //无commID
    {
        sprintf(build_po_url, "POST http://%s/update.json?api_key=%s&metadata=true&firmware=%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: dalian urban ILS1\r\nContent-Length:%d\r\n\r\n",
                WEB_SERVER,
                ApiKey,
                FIRMWARE,
                WEB_SERVER,
                pCreat_json1->creat_json_c);
        // sprintf(build_po_url, "%s%s%s%s%s%s%s%s%s%s%s%s%d%s", http.POST, http.POST_URL1, ApiKey, http.POST_URL_METADATA, http.POST_URL_FIRMWARE, FIRMWARE, http.POST_URL_SSID, NET_NAME,
        //         http.HTTP_VERSION11, http.HOST, http.USER_AHENT, http.CONTENT_LENGTH, pCreat_json1->creat_json_c, http.ENTER);
    }
    else
    {
        post_status = POST_NOCOMMAND;

        sprintf(build_po_url, "POST http://%s/update.json?api_key=%s&metadata=true&firmware=%s&command_id=%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: dalian urban ILS1\r\nContent-Length:%d\r\n\r\n",
                WEB_SERVER,
                ApiKey,
                FIRMWARE,
                mqtt_json_s.mqtt_command_id,
                WEB_SERVER,
                pCreat_json1->creat_json_c);

        // sprintf(build_po_url, "%s%s%s%s%s%s%s%s%s%s%s%s%d%s", http.POST, http.POST_URL1, ApiKey, http.POST_URL_METADATA, http.POST_URL_SSID, NET_NAME, http.POST_URL_COMMAND_ID, mqtt_json_s.mqtt_command_id,
        //         http.HTTP_VERSION11, http.HOST, http.USER_AHENT, http.CONTENT_LENGTH, pCreat_json1->creat_json_c, http.ENTER);
    }

    sprintf(build_po_url_json, "%s%s", build_po_url, pCreat_json1->creat_json_b);

    // printf("JSON_test = : %s\n", pCreat_json1->creat_json_b);

    free(pCreat_json1);
    ESP_LOGI(TAG, "POST:\n%s", build_po_url_json);

    //发送并解析返回数据
    /***********調用函數發送***********/

    if (http_send_buff(build_po_url_json, 1024, recv_buf, 1024) > 0)
    {
        // printf("解析返回数据！\n");
        ESP_LOGI(TAG, "mes recv:%s", recv_buf);
        if (parse_objects_http_respond(recv_buf))
        {
            Net_sta_flag = true;
        }
        else
        {
            Net_sta_flag = false;
        }
    }
    else
    {
        Net_sta_flag = false;
        printf("send return : %d \n", ret);
    }
}

//发送心跳包
bool Send_herat(void)
{
    char *build_heart_url;
    char *recv_buf;
    bool ret = false;
    xEventGroupWaitBits(Net_sta_group, ACTIVED_BIT, false, true, -1); //等网络连接

    build_heart_url = (char *)malloc(256);
    recv_buf = (char *)malloc(HTTP_RECV_BUFF_LEN);

    sprintf(build_heart_url, "GET /heartbeat?api_key=%s HTTP/1.1\r\nHost: %s\r\n\r\n",
            ApiKey,
            WEB_SERVER);

    if ((http_send_buff(build_heart_url, 256, recv_buf, HTTP_RECV_BUFF_LEN)) > 0)
    {
        if (parse_objects_heart(recv_buf))
        {
            //successed
            ret = true;
            Net_sta_flag = true;
        }
        else
        {
            ESP_LOGE(TAG, "hart recv:%s", recv_buf);
            ret = false;
            Net_sta_flag = false;
        }
    }
    else
    {
        ret = false;
        Net_sta_flag = false;
        ESP_LOGE(TAG, "hart recv 0!\r\n");
    }

    free(recv_buf);
    free(build_heart_url);
    return ret;
}

void send_heart_task(void *arg)
{
    while (1)
    {
        ESP_LOGW("heart_memroy check", " INTERNAL RAM left %dKB，free Heap:%d",
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
                 esp_get_free_heap_size());

        while (Send_herat() == false)
        {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        ulTaskNotifyTake(pdTRUE, -1);
    }
}

//激活流程
uint16_t http_activate(void)
{
    char *build_http = (char *)malloc(256);
    char *recv_buf = (char *)malloc(HTTP_RECV_BUFF_LEN);
    uint16_t ret = 0;

    xEventGroupWaitBits(Net_sta_group, CONNECTED_BIT, false, true, -1); //等网络连接

    sprintf(build_http, "GET /products/%s/devices/%s/activate HTTP/1.1\r\nHost: %s\r\n\r\n", ProductId, SerialNum, WEB_SERVER);
    ESP_LOGI(TAG, "%s", build_http);
    if (http_send_buff(build_http, 256, recv_buf, HTTP_RECV_BUFF_LEN) < 0)
    {
        Net_sta_flag = false;
        ret = 301;
    }
    else
    {
        if (parse_objects_http_active(recv_buf))
        {
            Net_sta_flag = true;
            ret = 1;
        }
        else
        {
            ESP_LOGE(TAG, "active recv:%s", recv_buf);
            Net_sta_flag = false;
            ret = 302;
        }
    }

    free(build_http);
    free(recv_buf);
    return ret;
}

void Active_Task(void *arg)
{
    xEventGroupSetBits(Net_sta_group, ACTIVE_S_BIT);
    while (1)
    {
        xEventGroupClearBits(Net_sta_group, ACTIVED_BIT);
        while ((Net_ErrCode = http_activate()) != 1) //激活
        {
            ESP_LOGE(TAG, "activate fail\n");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        xEventGroupSetBits(Net_sta_group, ACTIVED_BIT);
        break;
    }
    xEventGroupClearBits(Net_sta_group, ACTIVE_S_BIT);
    vTaskDelete(NULL);
}

void Start_Active(void)
{
    if ((xEventGroupGetBits(Net_sta_group) & ACTIVE_S_BIT) != ACTIVE_S_BIT)
    {
        xTaskCreate(Active_Task, "Active_Task", 3072, NULL, 4, &Active_Task_Handle);
    }
}

void initialise_http(void)
{
    xTaskCreate(send_heart_task, "send_heart_task", 4096, NULL, 5, &Binary_Heart_Send);
    xTaskCreate(http_get_task, "http_get_task", 8192, NULL, 4, &Binary_dp);
    esp_err_t err = esp_timer_create(&timer_heart_arg, &timer_heart_handle);
    err = esp_timer_start_periodic(timer_heart_handle, 60 * 1000000); //创建定时器，单位us，定时60s
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "timer heart create err code:%d\n", err);
    }
}