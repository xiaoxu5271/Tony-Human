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

#include "socket.h"

#include "w5500_spi.h"
#include "dhcp.h"

// extern OsiSyncObj_t Spi_Mutex;   //Used for SPI Lock
// extern OsiSyncObj_t cJSON_Mutex; //Used for cJSON Lock
// extern OsiSyncObj_t xMutex5;     //Used for Post_Data_Buffer Lock

// extern OsiSyncObj_t xBinary2; //For Temp&Humi Sensor Task
// extern OsiSyncObj_t xBinary3; //For Light Sensor Task
// extern OsiSyncObj_t xBinary6; //For external Temprature Measure
// extern OsiSyncObj_t xBinary7; //For Power Measure Task

#define RJ45_DEBUG 1
#define FAILURE 0
#define SUCCESS 1
#define RJ45_STATUS_TIMEOUT 5

uint8_t LAN_HOST_NAME[16];
uint8_t ethernet_buf[ETHERNET_DATA_BUF_SIZE];
uint8_t dns_host_ip[4];
uint8_t standby_dns[4] = {8, 8, 8, 8};
wiz_NetInfo gWIZNETINFO;
wiz_NetInfo gWIZNETINFO_READ;
extern char HOST_NAME[64];
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

/*******************************************************************************
// Reset w5500 chip with w5500 RST pin                                
*******************************************************************************/
void w5500_reset(void)
{
        gpio_set_level(PIN_NUM_W5500_REST, 0);
        vTaskDelay(10 / portTICK_RATE_MS);
        gpio_set_level(PIN_NUM_W5500_REST, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
}

/*******************************************************************************
// callback function for critical section enter.                            
*******************************************************************************/
void spi_cris_en(void)
{
        //osi_SyncObjWait(&Spi_Mutex, OSI_WAIT_FOREVER); //SPI Semaphore Take
}

/*******************************************************************************
// callback function for critical section exit.                              
*******************************************************************************/
void spi_cris_ex(void)
{
        //////osi_SyncObjSignal(&Spi_Mutex); //SPI Semaphore Give
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

// /*******************************************************************************
// // callback function to read byte using SPI
// *******************************************************************************/
// uint8_t spi_readbyte(void)
// {
//         return SPI_SendReciveByte(0x00);
// }

// /*******************************************************************************
// // callback function to write byte using SPI
// *******************************************************************************/
// void spi_writebyte(uint8_t writebyte)
// {
//         SPI_SendReciveByte(writebyte);
// }

// /*******************************************************************************
// // callback function to burst read using SPI
// *******************************************************************************/
// void spi_readburst(uint8_t *read_buf, uint16_t buf_len)
// {
//         uint16_t i;

//         for (i = 0; i < buf_len; i++)
//         {
//                 read_buf[i] = spi_readbyte();
//         }
// }

// /*******************************************************************************
// // callback function to burst write using SPI
// *******************************************************************************/
// void spi_writeburst(uint8_t *write_buf, uint16_t buf_len)
// {
//         uint16_t i;

//         for (i = 0; i < buf_len; i++)
//         {
//                 spi_writebyte(write_buf[i]);
//         }
// }

// void spi_write_data(uint16_t reg_addr, uint8_t ctrl_cmd, uint8_t *write_buf, uint16_t buf_len)
// {
//         uint16_t i;

//         SET_SPI2_CS_OFF(); //w5500 spi cs enable

//         SPI_SendReciveByte((uint8_t)((reg_addr) >> 8));

//         SPI_SendReciveByte((uint8_t)(reg_addr)); //16bit address last 8 bit address

//         SPI_SendReciveByte(ctrl_cmd);

//         for (i = 0; i < buf_len; i++)
//         {
//                 SPI_SendReciveByte(write_buf[i]);
//         }

//         SET_SPI2_CS_ON(); //w5500 spi cs disable
// }

// void spi_read_data(uint16_t reg_addr, uint8_t ctrl_cmd, uint8_t *read_buf, uint16_t buf_len)
// {
//         uint16_t i;

//         SET_SPI2_CS_OFF(); //w5500 spi cs enable

//         SPI_SendReciveByte((uint8_t)((reg_addr) >> 8));

//         SPI_SendReciveByte((uint8_t)(reg_addr)); //16bit address last 8 bit address

//         SPI_SendReciveByte(ctrl_cmd);

//         for (i = 0; i < buf_len; i++)
//         {
//                 read_buf[i] = SPI_SendReciveByte(0x00);
//         }

//         SET_SPI2_CS_ON(); //w5500 spi cs disable
// }

/*******************************************************************************
// init w5500 driver lib                               
*******************************************************************************/
void w5500_lib_init(void)
{
        // /* Critical section callback */
        reg_wizchip_cris_cbfunc(spi_cris_en, spi_cris_ex);

        /* Chip selection call back  不可�? */
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
esp_err_t check_rj45_status(void)
{
        //uint8_t i;

        // for (i = 0; i < RJ45_STATUS_TIMEOUT; i++)
        // {
        if (IINCHIP_READ(PHYCFGR) & 0x01)
        {
                printf("RJ45 OK\n");
                return ESP_OK;
        }
        // vTaskDelay(100 / portTICK_RATE_MS);
        //printf("RJ45 FAIL\n ");

        //osi_Sleep(100); //delay 0.1s
        // }
        else
        {
                printf("RJ45 FAIL\n ");
                return ESP_FAIL;
        }
}

/**
*@brief		检RJ45连接
*@param		ÎÞ
*@return	ÎÞ
*/
// uint8_t getPHYStatus(void)
// {
//         return IINCHIP_READ(PHYCFGR);
// }

void PHY_check(void)
{
        uint8_t PHY_connect = 0;
        PHY_connect = 0x01 & IINCHIP_READ(PHYCFGR);
        if (PHY_connect == 0)
        {
                printf(" \r\n cheak net!\r\n");

                // while (PHY_connect == 0)
                // {
                //         PHY_connect = 0x01 & getPHYStatus();
                //         printf(" .");
                //         vTaskDelay(10 / portTICK_RATE_MS);
                // }
                // printf(" \r\n");
        }
        else
        {
                printf("success!!!\n");
        }
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
        uint8_t dns[4] = {6, 6, 6, 6};                         //< DNS server IP Address

        uint8_t txsize[MAX_SOCK_NUM] = {4, 2, 2, 2, 2, 2, 2, 0}; //socket 0,16K
        uint8_t rxsize[MAX_SOCK_NUM] = {4, 2, 2, 2, 2, 2, 2, 0}; //socket 0,16K

        wizchip_init(txsize, rxsize);

        //osi_at24c08_ReadData(MAC_ADDR, (uint8_t *)mac, sizeof(mac), 1); //Read mac

        //dhcp_mode = osi_at24c08_read_byte(DHCP_MODE_ADDR);

        //osi_at24c08_ReadData(STATIC_IP_ADDR, ip, sizeof(ip), 1);

        //osi_at24c08_ReadData(STATIC_SN_ADDR, sn, sizeof(sn), 1);

        //osi_at24c08_ReadData(STATIC_GW_ADDR, gw, sizeof(gw), 1);

        //osi_at24c08_ReadData(STATIC_DNS_ADDR, dns, sizeof(dns), 1);

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

        printf("Network_init success!!!\n");
}

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

void w5500_user_int(void)
{
        gpio_config_t io_conf;

        //disable interrupt
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        //set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT;
        //bit mask of the pins that you want to set,e.g.GPIO16
        io_conf.pin_bit_mask = (1 << PIN_NUM_W5500_REST);
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //disable pull-up mode
        io_conf.pull_up_en = 1;
        //configure GPIO with the given settings
        gpio_config(&io_conf);
        io_conf.pin_bit_mask = (1 << PIN_NUM_W5500_INT);
        gpio_config(&io_conf);

        w5500_reset();
        PHY_check();
        w5500_lib_init();

        // printf(" VERSIONR_ID: %02x\n", IINCHIP_READ(VERSIONR));

        // W5500_Network_Init();

        // esp_err_t ret;
        // ret = check_rj45_status();
        // ESP_ERROR_CHECK(ret);
        PHY_check();

        vTaskDelay(100 / portTICK_RATE_MS);
        my_ip_assign();

        //xTaskCreate(do_tcp_server, "do_tcp_server", 8192, NULL, 2, NULL);
}

/*******************************************************************************
                                      END         
*******************************************************************************/
