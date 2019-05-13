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

#include "wizchip_conf.h"
#include "w5500_driver.h"

#include "w5500_socket.h"

#include "w5500_spi.h"
#include "w5500_dhcp.h"
#include "w5500_dns.h"
#include "Http.h"

// extern OsiSyncObj_t Spi_Mutex;   //Used for SPI Lock
// extern OsiSyncObj_t cJSON_Mutex; //Used for cJSON Lock
// extern OsiSyncObj_t xMutex5;     //Used for Post_Data_Buffer Lock

// extern OsiSyncObj_t xBinary2; //For Temp&Humi Sensor Task
// extern OsiSyncObj_t xBinary3; //For Light Sensor Task
// extern OsiSyncObj_t xBinary6; //For external Temprature Measure
// extern OsiSyncObj_t xBinary7; //For Power Measure Task

#define RJ45_DEBUG 1
#define FAILURE -1
#define SUCCESS 1
#define RJ45_STATUS_TIMEOUT 3
#define W5500_DNS_FAIL -3
#define NO_RJ45_ACCESS -4

uint32_t socker_port = 3000;
uint8_t LAN_WEB_SERVER[16];
uint8_t ethernet_buf[ETHERNET_DATA_BUF_SIZE];
uint8_t dns_host_ip[4];
uint8_t server_port = 80;
uint8_t standby_dns[4] = {8, 8, 8, 8};
uint8_t RJ45_STATUS;

wiz_NetInfo gWIZNETINFO;
wiz_NetInfo gWIZNETINFO_READ;
extern char POST_REQUEST_URI[255];
extern char Post_Data_Buffer[4096];
extern volatile bool NET_DATA_POST;
extern volatile unsigned long POST_NUM;
extern volatile unsigned long DELETE_ADDR, POST_ADDR, WRITE_ADDR;

extern short Prase_Resp_Data(uint8_t mode, char *recv_buf, uint16_t buf_len); //Prase GPRS/ETHERNET Post Resp Data
extern void PostAddrChage(unsigned long data_num, unsigned long end_addr);    //deleted the post data

uint8_t Ethernet_Timeout = 0; //ethernet http application time out
uint8_t Ethernet_netSet_val = 0;
uint8_t ethernet_http_mode = ACTIVE_DEVICE_MODE;
static bool lan_EndFlag = 0;
static uint16_t lan_read_data_num = 0;
static uint16_t lan_post_data_sum = 0;
static uint16_t lan_post_data_num = 0;
static uint16_t lan_delete_data_num = 0;
static unsigned long lan_post_data_len = 0;
static unsigned long lan_read_data_end_addr = 0;
static unsigned long lan_MemoryAddr = 0;
static char lan_mac_buf[18] = {0};
uint8_t Http_Buffer;

SemaphoreHandle_t xMutex_W5500_SPI;

/*******************************************************************************
// Reset w5500 chip with w5500 RST pin                                
*******************************************************************************/
void w5500_reset(void)
{
        gpio_set_level(PIN_NUM_W5500_REST, 0);
        vTaskDelay(10 / portTICK_RATE_MS);
        gpio_set_level(PIN_NUM_W5500_REST, 1);
        vTaskDelay(2000 / portTICK_RATE_MS);
}

/*******************************************************************************
// callback function for critical section enter.                            
*******************************************************************************/
void spi_cris_en(void)
{
        xSemaphoreTake(xMutex_W5500_SPI, portMAX_DELAY); //SPI Semaphore Take
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
        // printf("SPI CS SELECT! \n");
        // return;
}

/*******************************************************************************
// callback fucntion for WIZCHIP deselect                             
*******************************************************************************/
void spi_cs_deselect(void)
{
        spi_deselect_w5500();
        // printf("SPI CS DESELECT! \n");
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
        printf("lib_init success\n");
}

/*******************************************************************************
// w5500 enter sleep mode                              
*******************************************************************************/
// void w5500_enter_sleep(void)
// {
//         uint8_t write_val = 0x7f;
//         spi_write_data(0x002e, FDM1 | RWB_WRITE | COMMON_R, &write_val, 1);

//         write_val = 0xf7;
//         spi_write_data(0x002e, FDM1 | RWB_WRITE | COMMON_R, &write_val, 1);
// }

/*******************************************************************************
// check RJ45 connected                              
*******************************************************************************/
uint8_t check_rj45_status(void)
{
        uint8_t i;

        // for (i = 0; i < RJ45_STATUS_TIMEOUT; i++)

        if (IINCHIP_READ(PHYCFGR) & 0x01)
        {
                printf("RJ45 OK\n");
                return ESP_OK;
        }
        // printf("RJ45 FAIL\n ");
        // vTaskDelay(1000 / portTICK_RATE_MS);

        printf("RJ45 FAIL\n ");
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
        printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
        printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
        printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
        printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
        printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
#endif
}

/****************DHCP IP冲突函数*****************/
void my_ip_conflict(void)
{
        printf("DHCP IP 冲突\n");
}

/****************解析DNS*****************/
int8_t lan_dns_resolve(uint8_t *dns_buf)
{
        //   osi_at24c08_ReadData(HOST_ADDR,(uint8_t*)WEB_SERVER,sizeof(WEB_SERVER),1);  //read the host name

        DNS_init(SOCK_DHCP, dns_buf);

        if (DNS_run(gWIZNETINFO.dns, (uint8_t *)WEB_SERVER, dns_host_ip) > 0)
        {
#ifdef RJ45_DEBUG
                printf("host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif

                return SUCCESS;
        }
        else if (DNS_run(standby_dns, (uint8_t *)HOST_NAME, dns_host_ip) > 0)
        {
#ifdef RJ45_DEBUG
                printf("s_host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif

                return SUCCESS;
        }

#ifdef RJ45_DEBUG
        printf("n_host ip: %d.%d.%d.%d\r\n", dns_host_ip[0], dns_host_ip[1], dns_host_ip[2], dns_host_ip[3]);
#endif

        return FAILURE;
}

/*******************************************************************************
// w5500 init network                              
*******************************************************************************/
void W5500_Network_Init(void)
{
        uint8_t mac[6] = {0x06, 0x08, 0xdc, 0x00, 0xab, 0xcd}; //< Source Mac Address
        uint8_t dhcp_mode = 1;                                 //0:static ;1:dhcp
        uint8_t ip[4] = {192, 168, 1, 123};                    //< Source IP Address
        uint8_t sn[4] = {255, 255, 255, 0};                    //< Subnet Mask
        uint8_t gw[4] = {192, 168, 1, 1};                      //< Gateway IP Address
        uint8_t dns[4] = {114, 114, 114, 114};                 //< DNS server IP Address

        uint8_t txsize[MAX_SOCK_NUM] = {4, 2, 2, 2, 2, 2, 2, 0}; //socket 0,16K
        uint8_t rxsize[MAX_SOCK_NUM] = {4, 2, 2, 2, 2, 2, 2, 0}; //socket 0,16K

        esp_read_mac(mac, 3); //      获取芯片内部默认出厂MAC，

        wizchip_init(txsize, rxsize);

        memcpy(gWIZNETINFO.mac, mac, 6);
        memcpy(gWIZNETINFO.ip, ip, 4);
        memcpy(gWIZNETINFO.sn, sn, 4);
        memcpy(gWIZNETINFO.gw, gw, 4);
        memcpy(gWIZNETINFO.dns, dns, 4);

        if (dhcp_mode)
        {
                gWIZNETINFO.dhcp = NETINFO_DHCP; //< 1 - Static, 2 - DHCP
        }
        else
        {
                gWIZNETINFO.dhcp = NETINFO_STATIC; //< 1 - Static, 2 - DHCP
        }

        ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);

#ifdef RJ45_DEBUG
        ctlnetwork(CN_GET_NETINFO, (void *)&gWIZNETINFO_READ);
        printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO_READ.mac[0], gWIZNETINFO_READ.mac[1], gWIZNETINFO_READ.mac[2], gWIZNETINFO_READ.mac[3], gWIZNETINFO_READ.mac[4], gWIZNETINFO_READ.mac[5]);
        printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.ip[0], gWIZNETINFO_READ.ip[1], gWIZNETINFO_READ.ip[2], gWIZNETINFO_READ.ip[3]);
        printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.gw[0], gWIZNETINFO_READ.gw[1], gWIZNETINFO_READ.gw[2], gWIZNETINFO_READ.gw[3]);
        printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.sn[0], gWIZNETINFO_READ.sn[1], gWIZNETINFO_READ.sn[2], gWIZNETINFO_READ.sn[3]);
        printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO_READ.dns[0], gWIZNETINFO_READ.dns[1], gWIZNETINFO_READ.dns[2], gWIZNETINFO_READ.dns[3]);
#endif

        wiz_NetTimeout E_NetTimeout;
        E_NetTimeout.retry_cnt = 50;    //< retry count
        E_NetTimeout.time_100us = 1000; //< time unit 100us
        wizchip_settimeout(&E_NetTimeout);

        if (gWIZNETINFO.dhcp == NETINFO_DHCP)
        {
                Ethernet_Timeout = 0;
                uint8_t dhcp_retry = 0;

                reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);

                DHCP_init(SOCK_DHCP, ethernet_buf);

                while (DHCP_run() != DHCP_IP_LEASED)
                {
                        switch (DHCP_run())
                        {
                        case DHCP_IP_ASSIGN:
#ifdef RJ45_DEBUG
                                printf("DHCP_IP_ASSIGN.\r\n");
#endif
                        case DHCP_IP_CHANGED: /* If this block empty, act with default_ip_assign & default_ip_update */
                                              //
                                              // Add to ...
                                              //
#ifdef RJ45_DEBUG
                                printf("DHCP_IP_CHANGED.\r\n");
#endif
                                break;

                        case DHCP_IP_LEASED:
                                //
                                // TO DO YOUR NETWORK APPs.
                                //
#ifdef RJ45_DEBUG
                                printf("DHCP_IP_LEASED.\r\n");

                                printf("DHCP LEASED TIME : %d Sec\r\n", getDHCPLeasetime());
#endif
                                break;

                        case DHCP_FAILED:
#ifdef RJ45_DEBUG
                                printf("DHCP_FAILED.\r\n");
#endif

                                if (dhcp_retry++ > RETRY_TIME_OUT)
                                {
                                        DHCP_stop(); // if restart, recall DHCP_init()
                                }
                                break;

                        default:
                                break;
                        }
                }
        }
        printf("Network_init success!!!\n");
}

int8_t lan_http_send(char *send_buff, uint16_t send_size, char *recv_buff, uint16_t recv_size)
{
        int32_t ret = 0;
        uint16_t size = 0;

        while (1)
        {
                switch (getSn_SR(SOCK_DHCP))
                {

                case SOCK_INIT:
                        // printf("SOCK_INIT!!!\n");
                        ret = lan_connect(SOCK_DHCP, dns_host_ip, server_port);
                        if (ret <= 0)
                        {
                                printf("INIT FAIL CODE : %d\n", ret);
                                return ret;
                        }
                        break;

                case SOCK_ESTABLISHED:
                        if (getSn_IR(SOCK_DHCP) & Sn_IR_CON)
                        {
                                // printf("SOCK_ESTABLISHED!!!\n");
                                setSn_IR(SOCK_DHCP, Sn_IR_CON);
                        }
                        printf("send_buff   : %s, size :%d \n", (char *)send_buff, send_size);
                        lan_send(SOCK_DHCP, (const uint8_t *)send_buff, send_size);

                        vTaskDelay(100 / portTICK_RATE_MS); //需要延时一段时间，等待平台返回数据

                        size = getSn_RX_RSR(SOCK_DHCP);
                        printf("recv_size = %d\n", size);
                        if (size > 0)
                        {
                                if (size > ETHERNET_DATA_BUF_SIZE)
                                {
                                        size = ETHERNET_DATA_BUF_SIZE;
                                }
                                bzero((char *)recv_buff, recv_size); //写入之前清0
                                ret = lan_recv(SOCK_DHCP, (uint8_t *)recv_buff, size);
                                if (ret < 0)
                                {
                                        printf("w5500 recv failed! %d\n", ret);
                                        return ret;
                                        //break;
                                }
                                else
                                {
                                        //printf("w5500 recv  : %s\n", (char *)recv_buff);
                                }
                        }

                        lan_close(SOCK_DHCP);
                        break;

                case SOCK_CLOSE_WAIT:
                        printf("SOCK_CLOSE_WAIT!!!\n");

                        break;

                case SOCK_CLOSED:
                        // printf("Closed\r\n");
                        lan_socket(SOCK_DHCP, Sn_MR_TCP, socker_port++, 0x00);
                        if (ret > 0) //需要等到接收到数据才退出函数
                                return ret;
                        break;

                default:
                        break;
                }
                vTaskDelay(100 / portTICK_RATE_MS);
        }
}

/*****************RJ45_CHECK****************/
void RJ45_Check_Task(void *arg)
{
        uint8_t need_reinit = 0;
        while (1)
        {
                if (check_rj45_status() == ESP_OK)
                {
                        RJ45_STATUS = RJ45_CONNECTED;
                        if (need_reinit == 1)
                        {
                                W5500_Network_Init();
                        }
                        need_reinit = 0;
                }
                else
                {
                        RJ45_STATUS = RJ45_DISCONNECT;
                        need_reinit = 1;
                }
                vTaskDelay(500 / portTICK_RATE_MS);
        }
}

/*******************有线网初始化*******************/
int8_t w5500_user_int(void)
{
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

        xMutex_W5500_SPI = xSemaphoreCreateMutex(); //创建W5500 SPI 发送互斥信号

        w5500_reset();
        w5500_lib_init();

        // printf(" VERSIONR_ID: %02x\n", IINCHIP_READ(VERSIONR));
        int8_t ret;
        ret = check_rj45_status();
        if (ret != ESP_OK)
        {
                printf("未检测到网线接入!\n");
                return NO_RJ45_ACCESS;
        }
        printf("网线接入!\n");
        W5500_Network_Init();
        ret = lan_dns_resolve(ethernet_buf);
        if (ret != SUCCESS)
        {
                printf("初始化DNS解析失败!\n");
                return W5500_DNS_FAIL;
        }
        printf("DNS_SUCCESS!!!\n");
        RJ45_STATUS = RJ45_CONNECTED;
        // xTaskCreate(RJ45_Check_Task, "lan_http_send_task", 8192, NULL, 8, NULL); //创建任务，不断检查RJ45连接状态
        return SUCCESS;
}

/*******************************************************************************
                                      END         
*******************************************************************************/
