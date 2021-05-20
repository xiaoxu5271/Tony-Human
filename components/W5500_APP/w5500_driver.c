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
// #include "nvs_flash.h"

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
#include "My_Mqtt.h"
#include "Json_parse.h"
#include "E2prom.h"

#define TAG "W5500_driver"

TaskHandle_t RJ45_Task_Handle;

uint32_t socker_port = 3000; //本地端口 不可变
uint8_t ethernet_buf[ETHERNET_DATA_BUF_SIZE];
uint8_t http_dns_host_ip[4];
bool DHCP_INIT = false;         //重新初始化DHCP 标志，
char current_net_ip[20];        //当前内网IP，用于上传
uint8_t netinfo_buff[16] = {0}; //IP 设置参数
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
SemaphoreHandle_t xMutex_W5500_DNS;

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

    ctlnetwork(CN_GET_NETINFO, (void *)&gWIZNETINFO);
    memset(current_net_ip, 0, sizeof(current_net_ip));
    snprintf(current_net_ip, sizeof(current_net_ip), "&IP=%d.%d.%d.%d", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    ESP_LOGI(TAG, "%s \n", current_net_ip);

    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    ESP_LOGI(TAG, "SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    ESP_LOGI(TAG, "GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    ESP_LOGI(TAG, "SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    ESP_LOGI(TAG, "DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
}

/****************DHCP IP冲突函数*****************/
void my_ip_conflict(void)
{
    ESP_LOGI(TAG, "DHCP IP 冲突\n");
}

/****************解析DNS*****************/
bool lan_dns_resolve(uint8_t *web_url, uint8_t *dns_host_ip)
{
    xSemaphoreTake(xMutex_W5500_DNS, -1);
    bool ret;
    DNS_init(SOCK_DNS, ethernet_buf);
    ESP_LOGI(TAG, "url : %s\n", web_url);

    if (DNS_run(gWIZNETINFO.dns, web_url, dns_host_ip) > 0)
    {
        ret = true;
    }
    else if (DNS_run(standby_dns, web_url, dns_host_ip) > 0)
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    xSemaphoreGive(xMutex_W5500_DNS);
    return ret;
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

    gWIZPHYCONF.speed = PHY_SPEED_10;
    ctlwizchip(CW_SET_PHYCONF, (void *)&gWIZPHYCONF); //设置连接速度，降低功耗、温度。  然而发现并没有什么卵用
    ctlwizchip(CW_GET_PHYCONF, (void *)&gWIZPHYCONF_READ);

    // EE_byte_Read(ADDR_PAGE2, dhcp_mode_add, &user_dhcp_mode); //获取DHCP模式
    if (user_dhcp_mode == 0)
    {
        uint8_t i = 0;
        ESP_LOGI(TAG, "dhcp mode --- 0 Static\n");
        // E2P_Read(ETHERNET_IP_ADD, netinfo_buff, 16);
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
        reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
        DHCP_init(SOCK_DHCP, ethernet_buf);
    }
    ESP_LOGI(TAG, "Network_init success!!!\n");
}

int8_t W5500_DHCP_Init(void)
{
    if (gWIZNETINFO.dhcp == NETINFO_DHCP)
    {
        // Ethernet_Timeout = 0;
        uint8_t dhcp_retry = 0;
        uint8_t ret_DHCP_run = 0;

        while ((ret_DHCP_run = DHCP_run()) != DHCP_IP_LEASED)
        {
            ESP_LOGI(TAG, "ret_DHCP_run = %d \n", ret_DHCP_run);
            switch (ret_DHCP_run)
            {
            case DHCP_IP_ASSIGN:
                ESP_LOGI(TAG, "DHCP_IP_ASSIGN.\r\n");

            case DHCP_IP_CHANGED: /* If this block empty, act with default_ip_assign & default_ip_update */
                ESP_LOGI(TAG, "DHCP_IP_CHANGED.\r\n");
                break;

            case DHCP_IP_LEASED:
                ESP_LOGI(TAG, "DHCP_IP_LEASED.\r\n");
                ESP_LOGI(TAG, "DHCP LEASED TIME : %d Sec\r\n", getDHCPLeasetime());
                break;

            case DHCP_FAILED:
                ESP_LOGI(TAG, "DHCP_FAILED.\r\n");
                if (dhcp_retry++ > RETRY_TIME_OUT)
                {
                    dhcp_retry = 0;
                    return FAILURE;
                    DHCP_stop(); // if restart, recall DHCP_init()
                }
                break;

            case DHCP_STOPPED:
                ESP_LOGI(TAG, "DHCP_STOPPED.\r\n");
                return FAILURE;
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
    if (lan_dns_resolve((uint8_t *)WEB_SERVER, http_dns_host_ip) == false)
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
            // ESP_LOGI(TAG, "SOCK_INIT, server_ip:%d.%d.%d.%d!!!\n", http_dns_host_ip[0], http_dns_host_ip[1], http_dns_host_ip[2], http_dns_host_ip[3]);
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
            // ESP_LOGI(TAG, "send_buff   : %s, size :%d \n", (char *)send_buff, send_size);
            lan_send(SOCK_TCPS, (uint8_t *)send_buff, send_size);

            //等待平台返回
            for (uint16_t i = 0; i < 500; i++)
            {
                vTaskDelay(10 / portTICK_RATE_MS); //需要延时一段时间，等待平台返回数据
                if ((size = getSn_RX_RSR(SOCK_TCPS)) != 0)
                {
                    ESP_LOGI(TAG, "recv_size = %d\n", size);
                    break;
                }
            }

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
                    // ESP_LOGI(TAG, "w5500 recv failed! %d\n", rec_ret);
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
                RJ45_MODE = RJ45_INIT;
                return -temp;
            }
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

int32_t lan_http_init(char *post_header)
{
    int32_t ret = 1;
    uint8_t fail_num = 0;
    uint8_t temp;
    if (lan_dns_resolve((uint8_t *)WEB_SERVER, http_dns_host_ip) == false)
    {
        LAN_DNS_STATUS = 0;
        ESP_LOGI(TAG, "IW5500_DNS_FAIL\n");
        ret = -1;
        return ret;
    }
    LAN_DNS_STATUS = 1;

    while (1)
    {
        switch (temp = getSn_SR(SOCK_TCPS))
        {
        case SOCK_INIT:
            // ESP_LOGI(TAG, "SOCK_INIT, server_ip:%d.%d.%d.%d!!!\n", http_dns_host_ip[0], http_dns_host_ip[1], http_dns_host_ip[2], http_dns_host_ip[3]);
            // ret = lan_connect(SOCK_TCPS, http_dns_host_ip, server_port);

            // http_dns_host_ip[0] = 192;
            // http_dns_host_ip[1] = 168;
            // http_dns_host_ip[2] = 123;
            // http_dns_host_ip[3] = 121;

            ret = lan_connect(SOCK_TCPS, http_dns_host_ip, server_port);
            if (ret != SOCK_OK)
            {
                ESP_LOGI(TAG, "INIT FAIL CODE : %d\n", ret);
                return ret;
            }
            break;

        case SOCK_ESTABLISHED:
            if (getSn_IR(SOCK_TCPS) & Sn_IR_CON)
            {
                // ESP_LOGI(TAG,"SOCK_ESTABLISHED!!!\n");
                setSn_IR(SOCK_TCPS, Sn_IR_CON);
            }
            ESP_LOGI(TAG, "%d,%s", __LINE__, post_header);
            ret = lan_send(SOCK_TCPS, (uint8_t *)post_header, strlen(post_header));
            // str[8] = 0;
            // ret = lan_send(SOCK_TCPS, (uint8_t *)str, strlen(str));
            // ret = lan_send(SOCK_TCPS, (uint8_t *)str2, strlen(str2));
            return ret;

        case SOCK_CLOSE_WAIT:
            lan_close(SOCK_TCPS);
            ESP_LOGI(TAG, "SOCK_CLOSE_WAIT!!!\n");
            break;

        case SOCK_CLOSED:
            // ESP_LOGI(TAG,"Closed\r\n");
            lan_socket(SOCK_TCPS, Sn_MR_TCP, socker_port, 0x00);
            break;

        default:
            fail_num++;
            if (fail_num >= 10)
            {
                fail_num = 0;
                ESP_LOGI(TAG, "fail time out getSn_SR=  0x%02x\n", temp);
                RJ45_MODE = RJ45_INIT;
                return -temp;
            }
            break;
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

int32_t lan_http_write(char *write_buff)
{
    uint8_t temp;
    int32_t ret = 1;
    switch (temp = getSn_SR(SOCK_TCPS))
    {
    case SOCK_ESTABLISHED:
        ret = lan_send(SOCK_TCPS, (uint8_t *)write_buff, strlen(write_buff));
        break;

    default:
        ESP_LOGE(TAG, "fail time out getSn_SR=  0x%02x\n", temp);
        lan_close(SOCK_TCPS);
        ret = -temp;
    }
    return ret;
}

int32_t lan_http_read(char *recv_buff, uint16_t buff_len)
{
    uint8_t temp;
    uint16_t size;
    int32_t ret = 1;
    switch (temp = getSn_SR(SOCK_TCPS))
    {
    case SOCK_ESTABLISHED:

        for (uint16_t i = 0; i < 500; i++)
        {
            if ((size = getSn_RX_RSR(SOCK_TCPS)) != 0)
            {
                ESP_LOGI(TAG, "recv_size = %d\n", size);
                break;
            }
            vTaskDelay(10 / portTICK_RATE_MS); //需要延时一段时间，等待平台返回数据
        }

        if (size > 0)
        {
            ret = lan_recv(SOCK_TCPS, (uint8_t *)recv_buff, buff_len);
            // if (ret < 0)
            // {
            //     // ESP_LOGI(TAG, "w5500 recv failed! %d\n", rec_ret);
            // }
            // else
            // {
            //     ESP_LOGI(TAG, " len : %d ------------w5500 recv  : %s\n", ret, (char *)recv_buff);
            // }
        }
        lan_close(SOCK_TCPS);
        break;

    default:
        ESP_LOGE(TAG, "fail time out getSn_SR=  0x%02x\n", temp);
        lan_close(SOCK_TCPS);
        ret = -temp;
    }
    return ret;
}

void RJ45_Task(void *arg)
{
    uint8_t ret_dhcp;
    xEventGroupSetBits(Net_sta_group, ETH_Task_BIT);
    RJ45_MODE = RJ45_INIT;
    while (net_mode == NET_LAN)
    {
        switch (RJ45_MODE)
        {
        case RJ45_INIT:
            xEventGroupClearBits(Net_sta_group, CONNECTED_BIT);
            Start_Active();
            W5500_Network_Init();
            RJ45_MODE = RJ45_WORK;
            break;

        case RJ45_WORK:
            if ((check_rj45_status() == ESP_OK) && (DHCP_INIT == false))
            {
                if (gWIZNETINFO.dhcp == NETINFO_DHCP)
                {
                    ret_dhcp = DHCP_run();
                    switch (ret_dhcp)
                    {
                    case DHCP_IP_LEASED:
                        // ESP_LOGI(TAG, "DHCP LEASED TIME : %d Sec\r\n", getDHCPLeasetime());
                        xEventGroupSetBits(Net_sta_group, CONNECTED_BIT);
                        break;

                    default:
                        ESP_LOGI(TAG, "DHCP:%d", ret_dhcp);
                        Net_sta_flag = false;
                        Net_ErrCode = 502;
                        xEventGroupClearBits(Net_sta_group, CONNECTED_BIT);
                        Start_Active();

                        break;
                    }
                }
                else
                {
                    xEventGroupSetBits(Net_sta_group, CONNECTED_BIT);
                }
            }
            else
            {
                DHCP_INIT = false;
                DHCP_init(SOCK_DHCP, ethernet_buf);
                Net_sta_flag = false;
                Net_ErrCode = 501;
                xEventGroupClearBits(Net_sta_group, CONNECTED_BIT);
                Start_Active();
            }
            break;

        default:
            break;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    xEventGroupClearBits(Net_sta_group, ETH_Task_BIT);
    vTaskDelete(NULL);
}

/*******************有线网初始化*******************/
int8_t w5500_user_int(void)
{
    uint8_t fail_num = 0;
    xMutex_W5500_SPI = xSemaphoreCreateMutex(); //创建W5500 SPI 发送互斥信号
    xMutex_W5500_DNS = xSemaphoreCreateMutex();

    w5500_spi_init();
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
        fail_num++;
        if (fail_num > 5)
        {
            ETH_FLAG = false;
            ESP_LOGE(TAG, "w5500 read err!");
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    if (net_mode == NET_LAN)
    {
        Start_Eth_Net();
    }
    // xTaskCreate(RJ45_Check_Task, "RJ45_Check_Task", 4096, NULL, 7, NULL); //创建任务，不断检查RJ45连接状态

    return ETH_FLAG;
}

void Start_Eth_Net(void)
{
    if ((xEventGroupGetBits(Net_sta_group) & ETH_Task_BIT) != ETH_Task_BIT)
    {
        xTaskCreate(RJ45_Task, "RJ45_Task", 4096, NULL, 20, &RJ45_Task_Handle);
    }
    RJ45_MODE = RJ45_INIT;
}

/*******************************************************************************
                                      END         
*******************************************************************************/
