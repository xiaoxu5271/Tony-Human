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
#ifndef __W5500_DRIVER_H__
#define __W5500_DRIVER_H__

#define RETRY_TIME_OUT 10

/* Operation mode bits */
#define VDM 0x00
#define FDM1 0x01
#define FDM2 0x02
#define FDM4 0x03

/* Read_Write control bit */
#define RWB_READ 0x00
#define RWB_WRITE 0x04

/* Block select bits */
#define COMMON_R 0x00

#define VERSIONR_ID 0x04

#define MAX_SOCK_NUM 8
#define SOCK_DHCP 3
#define SOCK_TCPS 0
#define SOCK_OTA 2
#define MQTT_SOCKET 1

#define HTTP_HOST_PORT 80
#define ETHERNET_DATA_BUF_SIZE 4096

#define ACTIVE_DEVICE_MODE 1
#define PIN_NUM_W5500_REST 25
#define PIN_NUM_W5500_INT 26

#define HOST_NAME "api.ubibot.cn" //"api.ubibot.cn"api.yeelink.net

#define RJ45_CONNECTED 1
#define RJ45_DISCONNECT 2
#define RJ45_DISCONNECT 2

extern uint8_t RJ45_STATUS;
extern uint8_t LAN_DNS_STATUS;
extern uint8_t http_dns_host_ip[4];
extern char current_net_ip[20]; //当前内网IP，用于上传

/*-------------------------------- Includes ----------------------------------*/
//extern short Ethernet_http_application(uint8_t mode);
int8_t w5500_user_int(void);
void W5500_Network_Init(void);
int8_t lan_dns_resolve(uint8_t *web_url, uint8_t *dns_host_ip);
int32_t lan_http_send(char *send_buff, uint16_t send_size, char *recv_buff, uint16_t recv_size);
int32_t lan_ota(void);
int8_t W5500_DHCP_Init(void);

#endif
/*******************************************************************************
                                      END         
*******************************************************************************/
