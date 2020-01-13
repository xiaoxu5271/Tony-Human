/*
* @brief        ESP32操作SHT30-I2C
* @author       hx-zsj 
* @par Copyright (c):  
*/

/* 
=============
头文件包含
=============
*/
#include <stdio.h>
#include "string.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sht30dis.h"
#include "Json_parse.h"

/*
===========================
宏定义
=========================== 
*/
//I2C
#define I2C_SCL_IO 19              //SCL->IO19
#define I2C_SDA_IO 18              //SDA->IO18
#define I2C_MASTER_NUM I2C_NUM_1   //I2C_1
#define WRITE_BIT I2C_MASTER_WRITE //写:0
#define READ_BIT I2C_MASTER_READ   //读:1
#define ACK_CHECK_EN 0x1           //主机检查从机的ACK
#define ACK_CHECK_DIS 0x0          //主机不检查从机的ACK
#define ACK_VAL 0x0                //应答
#define NACK_VAL 0x1               //不应答

//SHT30
#define SHT30_WRITE_ADDR 0x44 //地址
#define CMD_FETCH_DATA_H 0x22 //循环采样，参考sht30 datasheet           1S两次
#define CMD_FETCH_DATA_L 0x36

#define CMD_MSB_0_5MSP 0x20 //循环采样，参考sht30 datasheet          0.5MPS
#define CMD_LSB_0_5MSP 0x32

#define CMD_MSB_NO_CLK 0x2C //单次测量，参考sht30 datasheet          no clock
#define CMD_LSB_HIGH 0x06   //Repeatability HIGH
/*
===========================
全局变量定义
===========================
*/
unsigned char sht30_buf[6] = {0};
float tem = 0.00, hum = 0.00; //定义全局变量  温度，湿度
/*
===========================
函数声明
=========================== 
*/

/*
* IIC初始化
* @param[in]   void  	         :无
* @retval      void                    :无
* @note        修改日志     :EEPROM中已经初始化过，此函数无需再次调用
*               Ver0.0.1:
*/
void i2c_init(void)
{
    //i2c配置结构体
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;             //I2C模式
    conf.sda_io_num = I2C_SDA_IO;            //SDA IO映射
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE; //SDA IO模式
    conf.scl_io_num = I2C_SCL_IO;            //SCL IO映射
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE; //SCL IO模式
    conf.master.clk_speed = 100000;          //I2C CLK频率
    i2c_param_config(I2C_MASTER_NUM, &conf); //设置I2C
    //注册I2C服务即使能
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/*
* sht30初始化
* @param[in]   void  		        :无
* @retval      int                  :0成功，其他失败
* @note        修改日志 
*               Ver0.0.1:
*/
int sht30_init(void)
{
    int ret;
    //配置SHT30的寄存器
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                                //新建操作I2C句柄
    i2c_master_start(cmd);                                                       //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN); //发地址+写+检查ack
    i2c_master_write_byte(cmd, CMD_MSB_0_5MSP, ACK_CHECK_EN);                    //发数据高8位+检查ack
    i2c_master_write_byte(cmd, CMD_LSB_0_5MSP, ACK_CHECK_EN);                    //发数据低8位+检查ack
    // i2c_master_write_byte(cmd, CMD_MSB_NO_CLK, ACK_CHECK_EN);                    //发数据高8位+检查ack
    // i2c_master_write_byte(cmd, CMD_LSB_HIGH, ACK_CHECK_EN);                      //发数据低8位+检查ack
    i2c_master_stop(cmd);                                                    //停止I2C
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS); //I2C发送
    i2c_cmd_link_delete(cmd);                                                //删除I2C句柄
    return ret;
}

/*
* sht30校验算法
* @param[in]   pdata  		        :需要校验的数据
* @param[in]   nbrOfBytes  		    :需要校验的数据长度
* @retval      int                  :校验值
* @note        修改日志 
*               Ver0.0.1:
*/
unsigned char SHT3X_CalcCrc(unsigned char *data, unsigned char nbrOfBytes)
{
    unsigned char bit;               // bit mask
    unsigned char crc = 0xFF;        // calculated checksum
    unsigned char byteCtr;           // byte counter
    unsigned int POLYNOMIAL = 0x131; // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

    // calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)
    {
        crc ^= (data[byteCtr]);
        for (bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ POLYNOMIAL;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}
/*
* sht30数据校验
* @param[in]   pdata  		        :需要校验的数据
* @param[in]   nbrOfBytes  		    :需要校验的数据长度
* @param[in]   checksum  		    :校验的结果
* @retval      int                  :0成功，其他失败
* @note        修改日志 
*               Ver0.0.1:

*/
unsigned char SHT3X_CheckCrc(unsigned char *pdata, unsigned char nbrOfBytes, unsigned char checksum)
{
    unsigned char crc;
    crc = SHT3X_CalcCrc(pdata, nbrOfBytes); // calculates 8-Bit checksum
    if (crc != checksum)
    {
        return 1;
    }
    return 0;
}

/*
* 获取sht30温湿度
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
*/
int sht30_SS_get_value(void)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); //新建操作I2C句柄
    i2c_master_start(cmd);                        //启动I2C

    i2c_master_start(cmd);                                                       //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN); //发地址+写+检查ack
                                                                                 // i2c_master_write_byte(cmd, CMD_MSB_0_5MSP, ACK_CHECK_EN);                    //发数据高8位+检查ack
                                                                                 // i2c_master_write_byte(cmd, CMD_LSB_0_5MSP, ACK_CHECK_EN);                    //发数据低8位+检查ack
    i2c_master_write_byte(cmd, CMD_MSB_NO_CLK, ACK_CHECK_EN);                    //发数据高8位+检查ack
    i2c_master_write_byte(cmd, CMD_LSB_HIGH, ACK_CHECK_EN);                      //发数据低8位+检查ack
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS); //I2C发送
    if (ret != ESP_OK)
    {
        printf("i2c_master_cmd_begin ERR : %d\n", ret);
        return ret;
    }
    i2c_cmd_link_delete(cmd); //删除I2C句柄
    // vTaskDelay(50 / portTICK_RATE_MS);

    cmd = i2c_cmd_link_create();                                                //新建操作I2C句柄
    i2c_master_start(cmd);                                                      //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | READ_BIT, ACK_CHECK_EN); //发地址+读+检查ack
    i2c_master_read_byte(cmd, &sht30_buf[0], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[1], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[2], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[3], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[4], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[5], NACK_VAL);                         //读取数据+不回复ack
    i2c_master_stop(cmd);                                                       //停止I2C
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);    //I2C发送
    if (ret != ESP_OK)
    {
        printf("i2c_master_cmd_begin ERR2 : %d\n", ret);
        return ret;
    }
    i2c_cmd_link_delete(cmd); //删除I2C句柄
    //校验读出来的数据，算法参考sht30 datasheet
    if ((!SHT3X_CheckCrc(sht30_buf, 2, sht30_buf[2])) && (!SHT3X_CheckCrc(sht30_buf + 3, 2, sht30_buf[5])))
    {
        ret = ESP_OK; //成功
    }
    else
    {
        ret = 1;
    }
    return ret;
}
/*
* 获取sht30温湿度
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志 
*               Ver0.0.1:
*/
int sht30_get_value(void)
{
    int ret;
    //配置SHT30的寄存器
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();                               //新建操作I2C句柄
    i2c_master_start(cmd);                                                      //启动I2C
    i2c_master_write_byte(cmd, SHT30_WRITE_ADDR << 1 | READ_BIT, ACK_CHECK_EN); //发地址+读+检查ack
    i2c_master_read_byte(cmd, &sht30_buf[0], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[1], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[2], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[3], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[4], ACK_VAL);                          //读取数据+回复ack
    i2c_master_read_byte(cmd, &sht30_buf[5], NACK_VAL);                         //读取数据+不回复ack
    i2c_master_stop(cmd);                                                       //停止I2C
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);    //I2C发送
    if (ret != ESP_OK)
    {
        return ret;
    }
    i2c_cmd_link_delete(cmd); //删除I2C句柄
    //校验读出来的数据，算法参考sht30 datasheet
    if ((!SHT3X_CheckCrc(sht30_buf, 2, sht30_buf[2])) && (!SHT3X_CheckCrc(sht30_buf + 3, 2, sht30_buf[5])))
    {
        ret = ESP_OK; //成功
    }
    else
    {
        ret = 1;
    }
    return ret;
}

/*
* 获取sht30单次计算后的温湿度
* @param[in]   void  		       :无
* @retval      void                :无
* @note        修改日志  
*               Ver0.0.1:
*/

void sht30_SingleShotMeasure(float *temp, float *humi)
{
    if (sht30_get_value() == ESP_OK) //获取温湿度
    {
        //算法参考sht30 datasheet
        *temp = ((((sht30_buf[0] * 256) + sht30_buf[1]) * 175) / 65535.0 - 45);
        *humi = (((sht30_buf[3] * 256) + (sht30_buf[4])) * 100 / 65535.0);
    }
    else
    {
        ESP_LOGI("SHT30", "Get sht30 file !!!\r\n");
    }
}

void Sht30_Task(void *arg)
{
    sht30_init();
    uint8_t count = 0;
    while (1)
    {
        // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, -1);

        if (fn_th > 0)
        {
            if (count++ >= fn_th)
            {
                sht30_SingleShotMeasure(&tem, &hum);
                ESP_LOGI("SHT30", "temp:%4.2f C \r\n", tem); //℃打印出来是乱码,所以用C代替
                ESP_LOGI("SHT30", "hum:%4.2f %%RH \r\n", hum);
                count = 0;
            }
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}