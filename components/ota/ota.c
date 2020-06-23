#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "Smartconfig.h"
#include "Json_parse.h"
#include "Http.h"
#include "Human.h"
#include "sht30dis.h"
#include "Mqtt.h"
#include "E2prom.h"
#include "w5500_driver.h"
#include "w5500_socket.h"
#include "w5500.h"
#include "Led.h"

#include "ota.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

//数据包长度
#define BUFFSIZE 2048
#define TEXT_BUFFSIZE 2048

static const char *TAG = "ota";
//LNA-OTA数据
static char ota_write_data[BUFFSIZE + 1] = {0};
//LNA-接收数据
static char text[BUFFSIZE + 1] = {0};
//LNA-镜像大小
static int binary_file_length = 0;
//LAN 报文镜像长度
uint32_t content_len = 0;

uint8_t ota_dns_host_ip[4];

TaskHandle_t ota_handle = NULL;
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    esp_restart();
    (void)vTaskDelete(NULL);

    // while (1)
    // {
    //     ;
    // }
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void wifi_ota_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example...");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(Net_sta_group, CONNECTED_BIT,
                        false, true, -1);
    ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

    // E2prom_page_Read(ota_url_add, (uint8_t *)mqtt_json_s.mqtt_ota_url, 128);
    ESP_LOGI(TAG, "OTA-URL=[%s]\r\n", mqtt_json_s.mqtt_ota_url);

    esp_http_client_config_t config = {
        .url = mqtt_json_s.mqtt_ota_url,
        // .cert_pem = (char *)server_cert_pem_start,   //https 证书
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1)
    {
        // Led_Status = LED_STA_OTA;
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        // printf("\n ota_write_data=%2x \n", (unsigned int)ota_write_data);
        if (data_read < 0)
        {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        }
        else if (data_read > 0)
        {
            if (image_header_was_checked == false)
            {
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
                {
                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                }
                else
                {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                }
            }
            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK)
            {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGI(TAG, "Written image length %d", binary_file_length);
        }
        else if (data_read == 0)
        {
            ESP_LOGI(TAG, "Connection closed,all data received");
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return;
}

/*****************OTA***********************/
// get mid str 把src中 s1 到 s2之间的数据拷贝（包括s1不包括S2）到 sub中
int8_t mid(char *src, char *s1, char *s2, char *sub)
{
    char *sub1;
    char *sub2;
    uint16_t n;

    sub1 = strstr(src, s1);
    if (sub1 == NULL)
        return -1;
    sub1 += strlen(s1);
    sub2 = strstr(sub1, s2);
    if (sub2 == NULL)
        return -1;
    n = sub2 - sub1;
    strncpy(sub, sub1, n);
    sub[n] = 0;
    return 1;
}
/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char *buffer, char delim, int len)
{
    int i = 0;
    while (buffer[i] != delim && i < len)
    {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
static bool read_past_http_header(char text[], int total_len, esp_ota_handle_t update_handle)
{
    printf("\r\n%s\r\n", text);
    /* 获取镜像大小*/
    char sub[20];
    if (mid((char *)text, "Content-Length: ", "\r\n", sub) != 1)
    {
        ESP_LOGE(TAG, "URL or SERVER ERROR!!!");
        return false;
    }

    content_len = atol(sub);
    ESP_LOGI(TAG, "Content-Length:%d\n", content_len);

    /* i means current position */
    int i = 0, i_read_len = 0;
    while (text[i] != 0 && i < total_len)
    {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2)
        {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write(update_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                return false;
            }
            else
            {
                ESP_LOGI(TAG, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }
            return true;
        }
        i += i_read_len;
    }
    return false;
}

void lan_ota_task(void *arg)
{

    printf("lan ota task start \n");

    int16_t con_ret;
    int32_t size;
    int32_t buff_len;
    uint16_t no_recv_time = 0;
    char ota_url[1024] = {0};
    char ota_sever[128] = {0};

    // E2prom_page_Read(ota_url_add, (uint8_t *)mqtt_json_s.mqtt_ota_url, 128);
    if (mid(mqtt_json_s.mqtt_ota_url, "://", "/", ota_sever) != 1)
    {
        printf("ota url err!!!\n");
        vTaskDelete(NULL);
    }

    snprintf(ota_url, sizeof(ota_url), "GET %s HTTP/1.1\r\n" //"POST http://www.relianyun.com/otaftp/esp32_project.bin HTTP/1.1\r\n"
                                       "Host:%s\r\n"
                                       "Accept:image/gif,image/x-xbitmap,image/jpeg,image/pjpeg,*/*\r\n"
                                       "Pragma:no-cache\r\n"
                                       "Accept-Encoding: gzip,deflate\r\n"
                                       "Connection:close\r\n" //keep-alive 打开会导致升级失败，数据包下载中途卡住
                                       "\r\n",                //不可缺少 否则会导致接收不到正确数据
             mqtt_json_s.mqtt_ota_url,
             ota_sever

    );

    printf("ota_url=%s", ota_url);

    lan_dns_resolve(SOCK_OTA, (uint8_t *)ota_sever, ota_dns_host_ip);

    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example...");
    //获取当前boot位置
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    //获取当前系统执行的固件所在的Flash分区
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    while (1)
    {
        // printf("while ing !!!\n");
        uint8_t temp;
        switch (temp = getSn_SR(SOCK_OTA))
        {
        case SOCK_INIT:
            printf("SOCK_INIT!!!\n");
            con_ret = lan_connect(SOCK_OTA, ota_dns_host_ip, 80);
            if (con_ret <= 0)
            {
                printf("INIT FAIL CODE : %d\n", con_ret);
                lan_dns_resolve(SOCK_OTA, (uint8_t *)ota_sever, ota_dns_host_ip);
                lan_close(SOCK_OTA);
                // return con_ret;
            }
            break;

        case SOCK_ESTABLISHED:
            if (getSn_IR(SOCK_OTA) & Sn_IR_CON)
            {
                printf("SOCK_ESTABLISHED!!!\n");
                setSn_IR(SOCK_OTA, Sn_IR_CON);
            }
            printf("send_buff: %s \n", ota_url);
            lan_send(SOCK_OTA, (uint8_t *)ota_url, sizeof(ota_url));

            vTaskDelay(500 / portTICK_RATE_MS); //需要延时一段时间，等待平台返回数据

            //获取当前系统下一个（紧邻当前使用的OTA_X分区）可用于烧录升级固件的Flash分区
            update_partition = esp_ota_get_next_update_partition(NULL);
            ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                     update_partition->subtype, update_partition->address);
            assert(update_partition != NULL);

            //OTA写开始
            err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
                lan_close(SOCK_OTA);
                vTaskDelete(NULL);
            }
            ESP_LOGI(TAG, "esp_ota_begin succeeded");

            bool resp_body_start = false;
            //包头
            if (getSn_RX_RSR(SOCK_OTA) > 0)
            {
                //写入之前清0
                memset(text, 0, TEXT_BUFFSIZE);
                memset(ota_write_data, 0, BUFFSIZE);

                buff_len = lan_recv(SOCK_OTA, (uint8_t *)text, TEXT_BUFFSIZE);
                memcpy(ota_write_data, text, buff_len);
                resp_body_start = read_past_http_header(text, buff_len, update_handle);
            }

            while (binary_file_length != content_len)
            {
                // Led_Status = LED_STA_OTA;
                //开始接收数据
                size = getSn_RX_RSR(SOCK_OTA);
                if (size > 0)
                {
                    //写入之前清0
                    memset(text, 0, TEXT_BUFFSIZE);
                    memset(ota_write_data, 0, BUFFSIZE);
                    //接收http包
                    buff_len = lan_recv(SOCK_OTA, (uint8_t *)text, TEXT_BUFFSIZE);

                    if (buff_len < 0)
                    {
                        //包异常
                        ESP_LOGE(TAG, "Error: receive data error! errno=%d buff_len=%d", errno, buff_len);
                        lan_close(SOCK_OTA);
                        vTaskDelete(NULL);
                        // task_fatal_error();
                    }
                    else if (buff_len > 0 && resp_body_start)
                    {
                        //数据段包
                        memcpy(ota_write_data, text, buff_len);
                        //写flash
                        err = esp_ota_write(update_handle, (const void *)ota_write_data, buff_len);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                            lan_close(SOCK_OTA);
                            vTaskDelete(NULL);
                        }
                        binary_file_length += buff_len;
                        ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
                    }
                    else
                    {
                        //未知错误
                        ESP_LOGE(TAG, "Unexpected recv result");
                    }
                }
                else
                {
                    no_recv_time++;
                    //OTA超时2S未收到数据
                    if (no_recv_time > 200)
                    {
                        ESP_LOGE(TAG, "OTA IMAGE RECV TIMEOUT!");
                        no_recv_time = 0;
                        lan_close(SOCK_OTA);
                        break;
                    }
                }

                vTaskDelay(10 / portTICK_RATE_MS);
            }
            lan_close(SOCK_OTA);
            ESP_LOGI(TAG, "Connection closed, all packets received");
            ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);
            //OTA写结束
            if (esp_ota_end(update_handle) != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_ota_end failed!");
                vTaskDelete(NULL);
            }
            //升级完成更新OTA data区数据，重启时根据OTA data区数据到Flash分区加载执行目标（新）固件
            err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
                vTaskDelete(NULL);
            }
            ESP_LOGI(TAG, "Update Successed\r\n ");

            vTaskDelay(1000 / portTICK_RATE_MS); //延时
            esp_restart();

            break;

        case SOCK_CLOSE_WAIT:
            printf("SOCK_CLOSE_WAIT!!!\n");
            lan_close(SOCK_OTA);
            break;

        case SOCK_CLOSED:
            printf("Closed\r\n");
            lan_socket(SOCK_OTA, Sn_MR_TCP, 8000, 0x00);
            if (LAN_DNS_STATUS != 1)
            {
                vTaskDelete(NULL);
            }
            break;

        default:
            printf("send get %2x\n", temp);
            lan_close(SOCK_OTA); //网线断开重联后会返回 0X22，自动进入UDP模式，所以需要关闭连接。
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void ota_start(void) //建立OTA升级任务，目的是为了让此函数被调用后尽快执行完毕
{
    if (LAN_DNS_STATUS == 1)
    {
        xTaskCreate(lan_ota_task, "lan_ota_task", 8192, NULL, 1, NULL); //
    }
    else
    {
        xTaskCreate(wifi_ota_task, "wifi_ota_task", 8192, NULL, 2, ota_handle);
    }
}