
#ifndef _E2PROM_H_
#define _E2PROM_H_

#include "freertos/FreeRTOS.h"

#if 1
#define I2C_MASTER_SCL_IO 33     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 32     /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1 /*!< I2C port number for master dev */

#else
#define I2C_MASTER_SCL_IO 19     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 18     /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1 /*!< I2C port number for master dev */

#endif

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ 100000   /*!< I2C master clock frequency */

#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0       /*!< I2C ack value */
#define NACK_VAL 0x1      /*!< I2C nack value */

// #define FM24
//Device Address
#ifdef FM24
#define E2P_SIZE 8 * 1024
#define DEV_ADD 0XAE
#else
#define E2P_SIZE 1024
#define DEV_ADD 0XA8
#endif

#define PRODUCT_ID_LEN 32
#define SERISE_NUM_LEN 16
#define WEB_HOST_LEN 32
#define CHANNEL_ID_LEN 16
#define USER_ID_LEN 48
#define API_KEY_LEN 33

//参数地址
#define PRODUCT_ID_ADDR 0                                    //product id
#define SERISE_NUM_ADDR PRODUCT_ID_ADDR + PRODUCT_ID_LEN + 1 //serise num
#define WEB_HOST_ADD SERISE_NUM_ADDR + SERISE_NUM_LEN + 1    //web host
#define CHANNEL_ID_ADD WEB_HOST_ADD + WEB_HOST_LEN + 1       //chanel id
#define USER_ID_ADD CHANNEL_ID_ADD + CHANNEL_ID_LEN + 1      //user id
#define API_KEY_ADD USER_ID_ADD + USER_ID_LEN + 1            //user id
#define WEB_PORT_ADD API_KEY_ADD + API_KEY_LEN + 1           //web PORT
#define MQTT_HOST_ADD WEB_PORT_ADD + 5 + 1                   //MQTT HOST
#define MQTT_PORT_ADD MQTT_HOST_ADD + WEB_HOST_LEN + 1       //MQTT PORT

#define FN_SET_FLAG_ADD MQTT_PORT_ADD + 5 + 1 //metadata setted flag u8
#define FN_DP_ADD FN_SET_FLAG_ADD + 1 + 1     //数据发送频率 uint32_t
#define FN_SEN_ADD FN_DP_ADD + 4 + 1          //人感灵敏度 uint32_t
#define CG_DATA_LED_ADD FN_SEN_ADD + 4 + 1    //LED uint8_t

#define NET_MODE_ADD CG_DATA_LED_ADD + 1 + 1       //UINT 8
#define WIFI_SSID_ADD NET_MODE_ADD + 1 + 1         //32
#define WIFI_PASSWORD_ADD WIFI_SSID_ADD + 32 + 1   //64
#define ETHERNET_IP_ADD WIFI_PASSWORD_ADD + 64 + 1 //20
#define ETHERNET_DHCP_ADD ETHERNET_IP_ADD + 20 + 1 //1

#define E2P_USAGED ETHERNET_DHCP_ADD

void E2prom_Init(void);
esp_err_t E2P_WriteOneByte(uint16_t reg_addr, uint8_t dat);
uint8_t E2P_ReadOneByte(uint16_t reg_addr);
void E2P_WriteLenByte(uint16_t WriteAddr, uint32_t DataToWrite, uint8_t Len);
uint32_t E2P_ReadLenByte(uint16_t ReadAddr, uint8_t Len);
void E2P_Read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead);
void E2P_Write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite);
void E2prom_empty_all(void);
void E2prom_set_defaul(bool flag);

#endif
