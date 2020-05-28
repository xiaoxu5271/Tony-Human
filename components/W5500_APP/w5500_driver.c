/*******************************************************************************
  * @file       Uart1 Application Task  
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

/*-------------------------------- Includes ----------------------------------*/
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "wizchip_conf.h"
#include "w5500_driver.h"

#include "w5500_socket.h"
#include "w5500.h"
#include "w5500_spi.h"
#include "w5500_dhcp.h"
#include "w5500_dns.h"
#include "Http.h"
#include "Led.h"
#include "lan_mqtt.h"
#include "Smartconfig.h"
#include "Mqtt.h"
#include "Json_parse.h"
#include "E2prom.h"

#define TAG "W5500_driver"

// #define WEB_SERVER "api.ubibot.cn"

uint32_t socker_port = 3000; //本地端口 不可变
uint8_t ethernet_buf[ETHERNET_DATA_BUF_SIZE];
uint8_t http_dns_host_ip[4];

char current_net_ip[20]; //当前内网IP，用于上传
uint8_t server_port = 80;
uint8_t standby_dns[4] = {8, 8, 8, 8};
uint8_t RJ45_STATUS;
uint8_t LAN_DNS_STATUS = 0;
uint16_t LAN_ERR_CODE = 0;

uint8_t user_dhcp_mode = 1; //0:static ;1:dhcp

// uint8_t Ethernet_Timeout = 0; //ethernet http application time out

wiz_NetInfo gWIZNETINFO;
wiz_NetInfo gWIZNETINFO_READ;

wiz_PhyConf gWIZPHYCONF;
wiz_PhyConf gWIZPHYCONF_READ;

SemaphoreHandle_t xMutex_W5500_SPI;

/*******************************************************************************
// Reset w5500 chip with w5500 RST pin                                
*******************************************************************************/
void w5500_reset(void)
{
    gpio_set_level(PIN_NUM_W5500_REST, 0);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_W5500_REST, 1);
    vTaskDelay(2000 / portTICK_RATE_MS); //时间不能再短
}

/*******************************************************************************
// callback function for critical section enter.                            
*******************************************************************************/
void spi_cris_en(void)
{
    xSemaphoreTake(xMutex_W5500_SPI, -1); //SPI Semaphore Take
}

/*******************************************************************************
// callback function for critical section exit.                              
*******************************************************************************/
void spi_cris_ex(void)
{
    xSemaphoreGive(xMutex_W5500_SPI); //SPI Semaphore Give
}

/*******************************************************************************
// callback function for WIZCHIP select                             
*******************************************************************************/
void spi_cs_select(void)
{
    // gpio_set_level(PIN_NUM_CS, 0);
    spi_select_w5500();
    // ESP_LOGI(TAG,"SPI CS SELECT! \n");
    // return;
}

/*******************************************************************************
// callback fucntion for WIZCHIP deselect                             
*******************************************************************************/
void spi_cs_deselect(void)
{
    spi_deselect_w5500();
    // ESP_LOGI(TAG,"SPI CS DESELECT! \n");
    // return;
    // gpio_set_level(PIN_NUM_CS, 1);
}

/*******************************************************************************
// init w5500 driver lib                               
*******************************************************************************/
void w5500_lib_init(void)
{
    // /* Critical section callback */
    reg_wizchip_cris_cbfunc(spi_cris_en, spi_cris_ex);

    /* Chip selection call back  */
    reg_wizchip_cs_cbfunc(spi_cs_select, spi_cs_deselect);

    /* SPI Read & Write callback function */
    reg_wizchip_spi_cbfunc(spi_readbyte, spi_writebyte);

    /* Registers call back function for SPI interface */
    reg_wizchip_spiburst_cbfunc(spi_readburst, spi_writeburst);
    ESP_LOGI(TAG, "lib_init success\n");
}

/*******************************************************************************
// w5500 enter sleep mode                              
*******************************************************************************/
// void w5500_enter_sleep(void)
// {
//     uint8_t write_val = 0x7f;
//     spi_write_data(0x002e, FDM1 | RWB_WRITE | COMMON_R, &write_val, 1);

//     write_val = 0xf7;
//     spi_write_data(0x002e, FDM1 | RWB_WRITE | COMMON_R, &write_val, 1);
// }

/*******************************************************************************
// check RJ45 connected                              
*******************************************************************************/
int8_t check_rj45_status(void)
{
    if (IINCHIP_READ(PHYCFGR) & 0x01)
    {
        // ESP_LOGI(TAG,"RJ45 OK\n");
        return ESP_OK;
    }
    return ESP_FAIL;
}

/****************DHCP IP更新回调函数*****************/
void my_ip_assign(void)
{
    getIPfromDHCP(gWIZNETINFO.ip);

    getGWfromDHCP(gWIZNETINFO.gw);

    getSNfromDHCP(gWIZNETINFO.sn);

    getDNSfromDHCP(gWIZNETINFO.dns);

    gWIZNETINFO.dhcp = NETINFO_DHCP; //NETINFO_STATIC; //< 1 - Static, 2 - DHCP

    ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);

#ifdef RJ45_DEBUG
    ctlnetwork(CN_GET_NETINFO, (void *)&gWIZNETINFO);
    memset(current_net_ip, 0, sizeof(current_net_ip));
    sprintf(current_net_ip, "&IP=%d.%d.%d.%d", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    ESP_LOGI(TAG, "%s \n", current_net_ip);

    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    ESP_LOGI(TAG, "SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    ESP_LOGI(TAG, "GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    ESP_LOGI(TAG, "SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    ESP_LOGI(TAG, "DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
#endif
}

/****************DHCP IP冲突函数*****************/
void my_ip_conflict(void)
{
    ESP_LOGI(TAG, "DHCP IP 冲突\n");
}

/****************解析DNS*****************/
int8_t lan_dns_resolve(uint8_t sock, uint8_t *web_url, uint8_t *dns_host_ip)
{

    DNS_init(sock, ethernet_buf);
    ESP_LOGI(TAG, "url : %s\n", web_url);

    if (DNS_run(gWIZNETINFO.dns, web_url, dns_host_ip) > 0)
    {
#ifdef RJ45_DEBUG
        ESP_LOGI(TAG, "host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif

        return SUCCESS;
    }
    else if (DNS_run(standby_dns, web_url, dns_host_ip) > 0)
    {
#ifdef RJ45_DEBUG
        ESP_LOGI(TAG, "s_host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif

        return SUCCESS;
    }

#ifdef RJ45_DEBUG
    ESP_LOGI(TAG, "n_host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif
    return FAILURE;
}

/*******************************************************************************
// w5500 init network                              
*******************************************************************************/
void W5500_Network_Init(void)
{
    uint8_t mac[6]; //< Source Mac Address
    // uint8_t dhcp_mode = 1; //0:static ;1:dhcp
    uint8_t ip[4];  //< Source IP Address
    uint8_t sn[4];  //< Subnet Mask
    uint8_t gw[4];  //< Gateway IP Address
    uint8_t dns[4]; //< DNS server IP Address

    uint8_t txsize[MAX_SOCK_NUM] = {4, 4, 4, 2, 2, 0, 0, 0}; //socket 0,16K
    uint8_t rxsize[MAX_SOCK_NUM] = {4, 4, 4, 2, 2, 0, 0, 0}; //socket 0,16K

    w5500_reset();

    esp_read_mac(mac, 3); //获取芯片内部默认出厂MAC，
    memcpy(gWIZNETINFO.mac, mac, 6);

    wizchip_init(txsize, rxsize); //设置接收发送缓存空间

    wiz_NetTimeout E_NetTimeout;
    E_NetTimeout.retry_cnt = 50;    //< retry count
    E_NetTimeout.time_100us = 1000; //< time unit 100us
    wizchip_settimeout(&E_NetTimeout);

    // gWIZPHYCONF.speed = PHY_SPEED_10;
    // ctlwizchip(CW_SET_PHYCONF, (void *)&gWIZPHYCONF); //设置连接速度，降低功耗、温度。  然而发现并没有什么卵用
    // ctlwizchip(CW_GET_PHYCONF, (void *)&gWIZPHYCONF_READ);

    EE_byte_Read(ADDR_PAGE2, dhcp_mode_add, &user_dhcp_mode); //获取DHCP模式
    if (user_dhcp_mode == 0)
    {
        uint8_t i = 0;
        uint8_t netinfo_buff[16];

        ESP_LOGI(TAG, "dhcp mode --- 0 Static\n");
        E2prom_page_Read(NETINFO_add, netinfo_buff, 16);
        for (uint8_t j = 0; j < 17; j++)
        {
            ESP_LOGI(TAG, "netinfo_buff[%d]:%d\n", j, netinfo_buff[j]);
        }

        while (i < 16)
        {
            if (i < 4)
            {
                ip[i] = netinfo_buff[i];
            }
            else if (i >= 4 && i < 8)
            {
                // if (i == 7)
                //     sn[i - 4] = 0;
                // else
                //     sn[i - 4] = 255;
                sn[i - 4] = netinfo_buff[i];
            }
            else if (i >= 8 && i < 12)
            {
                gw[i - 8] = netinfo_buff[i];
            }
            else
            {
                dns[i - 12] = netinfo_buff[i];
            }
            // ESP_LOGI(TAG,"netinfo_buff[%d]:%d \n", i, netinfo_buff[i]);
            i++;
        }

        memcpy(gWIZNETINFO.ip, ip, 4);
        memcpy(gWIZNETINFO.sn, sn, 4);
        memcpy(gWIZNETINFO.gw, gw, 4);
        memcpy(gWIZNETINFO.dns, dns, 4);

        gWIZNETINFO.dhcp = NETINFO_STATIC; //< 1 - Static, 2 - DHCP
        ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
    }

    else if (user_dhcp_mode == 1)
    {
        ESP_LOGI(TAG, "dhcp mode --- 1 DHCP\n");
        gWIZNETINFO.dhcp = NETINFO_DHCP; //< 1 - Static, 2 - DHCP
        ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
        W5500_DHCP_Init();
    }

#ifdef RJ45_DEBUG
    ctlnetwork(CN_GET_NETINFO, (void *)&gWIZNETINFO_READ);

    memset(current_net_ip, 0, sizeof(current_net_ip));
    sprintf(current_net_ip, "&IP=%d.%d.%d.%d", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    ESP_LOGI(TAG, "%s \n", current_net_ip);

    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO_READ.mac[0], gWIZNETINFO_READ.mac[1], gWIZNETINFO_READ.mac[2], gWIZNETINFO_READ.mac[3], gWIZNETINFO_READ.mac[4], gWIZNETINFO_READ.mac[5]);
    ESP_LOGI(TAG, "SIP: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.ip[0], gWIZNETINFO_READ.ip[1], gWIZNETINFO_READ.ip[2], gWIZNETINFO_READ.ip[3]);
    ESP_LOGI(TAG, "GAR: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.gw[0], gWIZNETINFO_READ.gw[1], gWIZNETINFO_READ.gw[2], gWIZNETINFO_READ.gw[3]);
    ESP_LOGI(TAG, "SUB: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.sn[0], gWIZNETINFO_READ.sn[1], gWIZNETINFO_READ.sn[2], gWIZNETINFO_READ.sn[3]);
    ESP_LOGI(TAG, "DNS: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.dns[0], gWIZNETINFO_READ.dns[1], gWIZNETINFO_READ.dns[2], gWIZNETINFO_READ.dns[3]);
#endif

    bl_flag = 0;
    ESP_LOGI(TAG, "Network_init success!!!\n");
}

int8_t W5500_DHCP_Init(void)
{
    if (gWIZNETINFO.dhcp == NETINFO_DHCP)
    {
        // Ethernet_Timeout = 0;
        uint8_t dhcp_retry = 0;
        uint8_t ret_DHCP_run = 0;
        reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);

        DHCP_init(SOCK_DHCP, ethernet_buf);

        while ((ret_DHCP_run = DHCP_run()) != DHCP_IP_LEASED)
        {
#ifdef RJ45_DEBUG
            ESP_LOGI(TAG, "ret_DHCP_run = %d \n", ret_DHCP_run);
#endif
            switch (ret_DHCP_run)
            {
            case DHCP_IP_ASSIGN:
#ifdef RJ45_DEBUG
                ESP_LOGI(TAG, "DHCP_IP_ASSIGN.\r\n");
#endif
            case DHCP_IP_CHANGED: /* If this block empty, act with default_ip_assign & default_ip_update */
                                  //
                                  // Add to ...
                                  //
#ifdef RJ45_DEBUG
                ESP_LOGI(TAG, "DHCP_IP_CHANGED.\r\n");
#endif
                break;

            case DHCP_IP_LEASED:
                //
                // TO DO YOUR NETWORK APPs.
                //
#ifdef RJ45_DEBUG
                ESP_LOGI(TAG, "DHCP_IP_LEASED.\r\n");
                ESP_LOGI(TAG, "DHCP LEASED TIME : %d Sec\r\n", getDHCPLeasetime());
#endif
                break;

            case DHCP_FAILED:
#ifdef RJ45_DEBUG
                ESP_LOGI(TAG, "DHCP_FAILED.\r\n");
#endif

                if (dhcp_retry++ > RETRY_TIME_OUT)
                {
                    dhcp_retry = 0;
                    return FAILURE;
                    DHCP_stop(); // if restart, recall DHCP_init()
                }
                break;

            case DHCP_STOPPED:
#ifdef RJ45_DEBUG
                ESP_LOGI(TAG, "DHCP_STOPPED.\r\n");
#endif
                return FAILURE;
                // vTaskDelay(10000 / portTICK_RATE_MS); //失败后延时重新开启DHCP
                // DHCP_init(SOCK_DHCP, ethernet_buf);
                break;

            default:
                break;
            }
            vTaskDelay(200 / portTICK_RATE_MS);
        }
    }

    return SUCCESS;
}

int32_t lan_http_send(char *send_buff, uint16_t send_size, char *recv_buff, uint16_t recv_size)
{
    int32_t rec_ret = 0;
    int8_t con_ret = 0;
    uint16_t size = 0;
    uint8_t fail_num = 0;
    uint8_t temp;
    if (lan_dns_resolve(SOCK_TCPS, (uint8_t *)WEB_SERVER, http_dns_host_ip) == FAILURE)
    {
        LAN_DNS_STATUS = 0;
        ESP_LOGI(TAG, "IW5500_DNS_FAIL\n");
        return -1;
    }
    LAN_DNS_STATUS = 1;

    while (1)
    {
        switch (temp = getSn_SR(SOCK_TCPS))
        {
        case SOCK_INIT:
            // ESP_LOGI(TAG,"SOCK_INIT!!!\n");
            con_ret = lan_connect(SOCK_TCPS, http_dns_host_ip, server_port);
            if (con_ret != SOCK_OK)
            {
                ESP_LOGI(TAG, "INIT FAIL CODE : %d\n", con_ret);
                return con_ret;
            }
            break;

        case SOCK_ESTABLISHED:
            if (getSn_IR(SOCK_TCPS) & Sn_IR_CON)
            {
                // ESP_LOGI(TAG,"SOCK_ESTABLISHED!!!\n");
                setSn_IR(SOCK_TCPS, Sn_IR_CON);
            }
            fail_num = 0;
            // ESP_LOGI(TAG,"send_buff   : %s, size :%d \n", (char *)send_buff, send_size);
            lan_send(SOCK_TCPS, (uint8_t *)send_buff, send_size);

            vTaskDelay(100 / portTICK_RATE_MS); //需要延时一段时间，等待平台返回数据

            size = getSn_RX_RSR(SOCK_TCPS);
            // ESP_LOGI(TAG,"recv_size = %d\n", size);
            if (size > 0)
            {
                if (size > ETHERNET_DATA_BUF_SIZE)
                {
                    size = ETHERNET_DATA_BUF_SIZE;
                }
                bzero((char *)recv_buff, recv_size); //写入之前清0
                rec_ret = lan_recv(SOCK_TCPS, (uint8_t *)recv_buff, size);
                if (rec_ret < 0)
                {
                    ESP_LOGI(TAG, "w5500 recv failed! %d\n", rec_ret);
                    return rec_ret;
                    //break;
                }
                else
                {
                    // ESP_LOGI(TAG," len : %d ------------w5500 recv  : %s\n", rec_ret, (char *)recv_buff);
                }
            }

            lan_close(SOCK_TCPS);
            break;

        case SOCK_CLOSE_WAIT:
            lan_close(SOCK_TCPS);
            ESP_LOGI(TAG, "SOCK_CLOSE_WAIT!!!\n");

            break;

        case SOCK_CLOSED:
            // ESP_LOGI(TAG,"Closed\r\n");
            lan_socket(SOCK_TCPS, Sn_MR_TCP, socker_port, 0x00);
            if (rec_ret > 0) //需要等到接收到数据才退出函数
            {
                // ESP_LOGI(TAG,"rec_ret: %d\r\n", rec_ret);
                return rec_ret;
            }
            break;

        default:
            fail_num++;
            if (fail_num >= 10)
            {
                fail_num = 0;
                ESP_LOGI(TAG, "fail time out getSn_SR=  0x%02x\n", temp);
                return -temp;
            }
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

/*****************RJ45_CHECK****************/
void RJ45_Check_Task(void *arg)
{
    // uint8_t need_reinit = 1;
    W5500_Network_Init();
    while (1)
    {
        if (bl_flag == 0) //非配网状态
        {
            switch (net_mode)
            {
            case NET_AUTO: //自动选择
                start_user_wifi();
                //获取网络状态
                if (check_rj45_status() == ESP_OK)
                {
                    RJ45_STATUS = RJ45_CONNECTED; //
                    if (LAN_DNS_STATUS == 0)
                    {
                        if (W5500_DHCP_Init() == SUCCESS) //获取内网IP成功
                        {
                            lan_dns_resolve(SOCK_TCPS, (uint8_t *)WEB_SERVER, http_dns_host_ip);
                            LAN_DNS_STATUS = 1;
                        }
                        else
                        {
                            LAN_DNS_STATUS = 0;
                            // vTaskDelay(5000 / portTICK_RATE_MS); //失败后延时重新开启DHCP
                        }
                    }
                }
                else
                {
                    RJ45_STATUS = RJ45_DISCONNECT;
                    LAN_DNS_STATUS = 0;
                }

                //针对网络状态
                if (LAN_DNS_STATUS == 0)
                {
                    ESP_LOGD(TAG, "有线网络连接断开！\n");
                    if (lan_mqtt_status == 1)
                    {
                        stop_lan_mqtt();
                    }

                    if (wifi_connect_sta == connect_Y)
                    {
                        if (wifi_mqtt_status == 0 && MQTT_INIT_STA == 1) //WIFI_MQTT初始化完成
                        {
                            start_wifi_mqtt();
                        }
                    }
                    else //断网断开MQTT
                    {
                        if (wifi_mqtt_status == 1 && MQTT_INIT_STA == 1) //WIFI_MQTT初始化完成
                        {
                            stop_wifi_mqtt();
                        }
                    }
                }
                else
                {
                    ESP_LOGD(TAG, "有线网络已连接！\n");
                    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT); //联网标志

                    if (lan_mqtt_status == 0)
                    {
                        start_lan_mqtt();
                    }
                    if (wifi_mqtt_status == 1)
                    {
                        stop_wifi_mqtt();
                    }
                }

                break;

            case NET_LAN: //仅网线
                if (check_rj45_status() == ESP_OK)
                {
                    RJ45_STATUS = RJ45_CONNECTED; //
                    if (LAN_DNS_STATUS == 0)
                    {
                        if (W5500_DHCP_Init() == SUCCESS) //获取内网IP成功
                        {
                            LAN_ERR_CODE = 0;
                            LAN_DNS_STATUS = 1;
                        }
                        else
                        {
                            LAN_ERR_CODE = LAN_NO_IP;
                            LAN_DNS_STATUS = 0;
                            // vTaskDelay(5000 / portTICK_RATE_MS); //失败后延时重新开启DHCP
                        }
                    }
                }
                else
                {
                    LAN_ERR_CODE = LAN_NO_RJ45;
                    RJ45_STATUS = RJ45_DISCONNECT;
                    LAN_DNS_STATUS = 0;
                }

                if (LAN_DNS_STATUS == 0)
                {
                    ESP_LOGD(TAG, "有线网络连接断开！\n");

                    Led_Status = LED_STA_WIFIERR;                          //断网
                    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT); //断网标志
                    if (lan_mqtt_status == 1)
                    {
                        stop_lan_mqtt();
                    }
                }
                else
                {
                    ESP_LOGD(TAG, "有线网络已连接！\n");
                    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT); //联网标志

                    if (lan_mqtt_status == 0)
                    {
                        start_lan_mqtt();
                    }
                }

                if (wifi_mqtt_status == 1) //停用WIFI MQTT
                {
                    stop_wifi_mqtt();
                }

                stop_user_wifi();
                break;

            case NET_WIFI: //仅WIFI
                LAN_DNS_STATUS = 0;
                start_user_wifi();
                if (lan_mqtt_status == 1)
                {
                    stop_lan_mqtt();
                }
                if (wifi_connect_sta == connect_Y)
                {
                    if (wifi_mqtt_status == 0 && MQTT_INIT_STA == 1) //WIFI_MQTT初始化完成
                    {
                        start_wifi_mqtt();
                    }
                }
                else //断网断开ＭＱＴＴ
                {
                    if (wifi_mqtt_status == 1 && MQTT_INIT_STA == 1) //WIFI_MQTT初始化完成
                    {
                        stop_wifi_mqtt();
                    }
                }
                break;

            default:
                break;
            }
        }
        else
        {
            LAN_DNS_STATUS = 0;
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT); //断网标志
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

/*******************有线网初始化*******************/
int8_t w5500_user_int(void)
{
    xMutex_W5500_SPI = xSemaphoreCreateMutex(); //创建W5500 SPI 发送互斥信号

    w5500_spi_init();
    lan_mqtt_init();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << PIN_NUM_W5500_REST);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    io_conf.pin_bit_mask = (1 << PIN_NUM_W5500_INT);
    gpio_config(&io_conf);
    w5500_lib_init();
    w5500_reset();
    while ((IINCHIP_READ(VERSIONR)) != VERSIONR_ID)
    {
        ETH_FLAG = false;
        ESP_LOGE(TAG, "w5500 read err!");
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    ETH_FLAG = true;

    xTaskCreate(RJ45_Check_Task, "RJ45_Check_Task", 4096, NULL, 7, NULL); //创建任务，不断检查RJ45连接状态

    return SUCCESS;
}

/*******************************************************************************
                                      END         
*******************************************************************************/
