#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "Json_parse.h"
#include "Http.h"
#include "user_key.h"
#include "Led.h"
#include "crc8_16.h"
#include "ota.h"

#include "E2prom.h"

static const char *TAG = "EEPROM";
SemaphoreHandle_t At24_Mutex = NULL;

static bool AT24CXX_Check(void);
static void E2prom_read_defaul(void);

void E2prom_Init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.clk_flags = 0; //4.3 新增
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);

    At24_Mutex = xSemaphoreCreateMutex();

    while (AT24CXX_Check() != true)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        ESP_LOGE(TAG, "eeprom err !");
        E2P_FLAG = false;
        return;
    }
    E2P_FLAG = true;
}

esp_err_t AT24CXX_WriteOneByte(uint16_t reg_addr, uint8_t dat)
{
    xSemaphoreTake(At24_Mutex, -1);
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEV_ADD + ((reg_addr / 256) << 1)), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (reg_addr % 256), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, dat, ACK_CHECK_EN);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(20 / portTICK_RATE_MS);
    xSemaphoreGive(At24_Mutex);
    return ret;
}
esp_err_t AT24CXX_Write(uint16_t reg_addr, uint8_t *dat, uint16_t len)
{
    while (len)
    {
        AT24CXX_WriteOneByte(reg_addr, *dat);
        reg_addr++;
        dat++;
        len--;
    }
    return ESP_OK;
}

uint8_t AT24CXX_ReadOneByte(uint16_t reg_addr)
{
    xSemaphoreTake(At24_Mutex, -1);
    uint8_t temp = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEV_ADD + ((reg_addr / 256) << 1)), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (reg_addr % 256), ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DEV_ADD + 1, ACK_CHECK_EN);

    i2c_master_read_byte(cmd, &temp, NACK_VAL); //只读1 byte 不需要应答

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(20 / portTICK_RATE_MS);
    xSemaphoreGive(At24_Mutex);
    return temp;
}
esp_err_t AT24CXX_Read(uint16_t reg_addr, uint8_t *dat, uint16_t len)
{
    while (len)
    {
        *dat = AT24CXX_ReadOneByte(reg_addr);
        reg_addr++;
        dat++;
        len--;
    }
    return ESP_OK;
}

esp_err_t FM24C_Write(uint16_t reg_addr, uint8_t *dat, uint16_t len)
{
    xSemaphoreTake(At24_Mutex, -1);
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, DEV_ADD, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (reg_addr & 0xff00) >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr & 0xff, ACK_CHECK_EN);

    while (len)
    {
        i2c_master_write_byte(cmd, *dat, ACK_CHECK_EN); //send data value
        dat++;
        len--;
    }

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(At24_Mutex);
    return ret;
}

esp_err_t FM24C_Read(uint16_t reg_addr, uint8_t *dat, uint16_t len)
{
    xSemaphoreTake(At24_Mutex, -1);
    int ret, i;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, DEV_ADD, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (reg_addr & 0xff00) >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr & 0xff, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DEV_ADD + 1, ACK_CHECK_EN);

    for (i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            i2c_master_read_byte(cmd, dat, NACK_VAL); //只读1 byte 不需要应答;  //read a byte no ack
        }
        else
        {
            i2c_master_read_byte(cmd, dat, ACK_VAL); //read a byte ack
        }
        dat++;
    }

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(At24_Mutex);
    return ret;
}

esp_err_t FM24C_Empty(uint16_t len)
{
    xSemaphoreTake(At24_Mutex, -1);
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, DEV_ADD, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (0 & 0xff00) >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0 & 0xff, ACK_CHECK_EN);

    while (len)
    {
        i2c_master_write_byte(cmd, 0, ACK_CHECK_EN); //send data value
        len--;
    }

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(At24_Mutex);
    return ret;
}

esp_err_t E2P_WriteOneByte(uint16_t reg_addr, uint8_t dat)
{
    int ret = ESP_OK;
    uint8_t check_temp[2];
    uint8_t data_buff[2];
    while (1)
    {
        data_buff[0] = dat;
        data_buff[1] = Get_Crc8(data_buff, 1);
#ifdef FM24

        FM24C_Write(reg_addr, data_buff, 2);
        FM24C_Read(reg_addr, check_temp, 2);
        if (memcmp(check_temp, data_buff, 2) == 0)
        {
            break;
        }
        else
        {
            ESP_LOGE(TAG, "E2P_WriteOneByte err!!!");
        }
#else
        AT24CXX_Write(reg_addr, data_buff, 2);
        AT24CXX_Read(reg_addr, check_temp, 2);
        if (memcmp(check_temp, data_buff, 2) == 0)
        {
            E2P_FLAG = true;
            break;
        }
        else
        {
            E2P_FLAG = false;
            ESP_LOGE(TAG, "E2P_WriteOneByte err!!!");
        }
#endif
    }

    return ret;
}

uint8_t E2P_ReadOneByte(uint16_t reg_addr)
{
    uint8_t temp = 0;
    uint8_t temp_buf[2];

    // FM24C_Read(reg_addr, temp_buf, 2);
    // temp = temp_buf[0];
    for (uint8_t i = 0; i < 10; i++)
    {
#ifdef FM24
        FM24C_Read(reg_addr, temp_buf, 2);

        if (temp_buf[1] == Get_Crc8(temp_buf, 1))
        {
            temp = temp_buf[0];
            break;
        }
        else
        {
            ESP_LOGE(TAG, "E2P_ReadOneByte err");
        }
#else
        AT24CXX_Read(reg_addr, temp_buf, 2);

        if (temp_buf[1] == Get_Crc8(temp_buf, 1))
        {
            E2P_FLAG = true;
            temp = temp_buf[0];
            break;
        }
        else
        {
            E2P_FLAG = false;
            ESP_LOGE(TAG, "E2P_ReadOneByte err");
        }
#endif
    }

    return temp;
}

//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
void E2P_WriteLenByte(uint16_t WriteAddr, uint32_t DataToWrite, uint8_t Len)
{
    uint8_t *check_temp;
    uint8_t t;
    uint8_t *data_temp;

    data_temp = (uint8_t *)malloc(Len + 1);
    check_temp = (uint8_t *)malloc(Len + 1);

    while (1)
    {

        memset(data_temp, 0, Len + 1);
        memset(check_temp, 0, Len + 1);
        for (t = 0; t < Len; t++)
        {
            data_temp[t] = (DataToWrite >> (8 * t)) & 0xff;
        }
        data_temp[Len] = Get_Crc8(data_temp, Len);
#ifdef FM24
        FM24C_Write(WriteAddr, data_temp, Len + 1);
        FM24C_Read(WriteAddr, check_temp, Len + 1);
#else
        AT24CXX_Write(WriteAddr, data_temp, Len + 1);
        AT24CXX_Read(WriteAddr, check_temp, Len + 1);
#endif
        if (memcmp(data_temp, check_temp, Len + 1) == 0)
        {
            E2P_FLAG = true;
            break;
        }
        else
        {
            E2P_FLAG = false;
            // esp_log_buffer_hex(TAG, data_temp, Len + 1);
            // esp_log_buffer_hex(TAG, check_temp, Len + 1);
            ESP_LOGE(TAG, "E2P_WriteLenByte err!!!");
        }
    }
    free(data_temp);
    free(check_temp);
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址
//返回值     :数据
//Len        :要读出数据的长度2,4
uint32_t E2P_ReadLenByte(uint16_t ReadAddr, uint8_t Len)
{
    uint8_t t;
    uint32_t temp = 0;
    uint8_t *data_temp;
    data_temp = (uint8_t *)malloc(Len + 1);

    for (uint8_t i = 0; i < 10; i++)
    {
#ifdef FM24

        FM24C_Read(ReadAddr, data_temp, Len + 1);
#else
        AT24CXX_Read(ReadAddr, data_temp, Len + 1);
#endif
        if (data_temp[Len] == Get_Crc8(data_temp, Len)) //校验正确
        {
            for (t = 0; t < Len; t++)
            {
                temp += (data_temp[t] << (8 * t));
            }
            E2P_FLAG = true;
            break;
        }
        else
        {
            E2P_FLAG = false;
            // esp_log_buffer_hex(TAG, data_temp, Len + 1);
            ESP_LOGE(TAG, "E2P_ReadLenByte err!!!");
        }
    }

    free(data_temp);
    return temp;
}

//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c08为0~1023
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void E2P_Read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead)
{
    uint8_t *check_buff;
    check_buff = (uint8_t *)malloc(NumToRead + 1);

    for (uint8_t i = 0; i < 10; i++)
    {
#ifdef FM24
        FM24C_Read(ReadAddr, check_buff, NumToRead + 1);
#else
        AT24CXX_Read(ReadAddr, check_buff, NumToRead + 1);
#endif
        if (check_buff[NumToRead] == Get_Crc8(check_buff, NumToRead))
        {
            memcpy(pBuffer, check_buff, NumToRead);
            E2P_FLAG = true;
            break;
        }
        else
        {
            E2P_FLAG = false;
            ESP_LOGE(TAG, "E2P_Read err!!!");
        }
    }
    free(check_buff);
}
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c08为0~1023
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
void E2P_Write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
    uint8_t *check_buff;
    uint8_t *write_buff;
    check_buff = (uint8_t *)malloc(NumToWrite + 1);
    write_buff = (uint8_t *)malloc(NumToWrite + 1);

    while (1)
    {

        memset(check_buff, 0, NumToWrite + 1);
        memset(write_buff, 0, NumToWrite + 1);

        memcpy(write_buff, pBuffer, NumToWrite);
        write_buff[NumToWrite] = Get_Crc8(pBuffer, NumToWrite);
#ifdef FM24
        FM24C_Write(WriteAddr, write_buff, NumToWrite + 1);
        FM24C_Read(WriteAddr, check_buff, NumToWrite + 1);
#else
        AT24CXX_Write(WriteAddr, write_buff, NumToWrite + 1);
        AT24CXX_Read(WriteAddr, check_buff, NumToWrite + 1);
#endif
        if (memcmp(write_buff, check_buff, NumToWrite + 1) == 0)
        {
            E2P_FLAG = true;
            break;
        }
        else
        {
            E2P_FLAG = false;
            ESP_LOGE(TAG, "E2P_Write err!!!");
        }
    }
    free(check_buff);
    free(write_buff);
}

/******************************************************************************
//at24c08 write page,addr:0-1023,Size:1-16
******************************************************************************/
esp_err_t at24c08_WritePage(uint16_t reg_addr, uint8_t buf_len, uint8_t data)
{
    // MulTry_I2C_WR_mulReg(at24c08_addr0 + reg_addr / 256, reg_addr % 256, buffer, buf_len); //iic multi write

    xSemaphoreTake(At24_Mutex, -1);
    // IIC_Start(); //IIC start
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEV_ADD + ((reg_addr / 256) << 1)), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (reg_addr % 256), ACK_CHECK_EN);

    while (buf_len)
    {
        i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
        buf_len--;
    }

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(At24_Mutex);

    vTaskDelay(15 / portTICK_RATE_MS);

    return ret;
}

#define PAGE_SIZE 16
void at24c08_WriteData(uint16_t addr, uint16_t size, uint8_t data)
{

    uint16_t i;
    uint16_t remain;

    remain = PAGE_SIZE - addr % PAGE_SIZE;

    if (remain)
    {
        remain = size > remain ? remain : size;
        ESP_LOGI(TAG, "%d", __LINE__);
        at24c08_WritePage(addr, remain, data);

        addr += remain;
        size -= remain;
    }

    remain = size / PAGE_SIZE;

    for (i = 0; i < remain; i++)
    {
        ESP_LOGI(TAG, "%d", __LINE__);
        at24c08_WritePage(addr, PAGE_SIZE, data);

        addr += PAGE_SIZE;
    }

    remain = size % PAGE_SIZE;

    if (remain)
    {
        ESP_LOGI(TAG, "%d", __LINE__);
        at24c08_WritePage(addr, remain, data);
        addr += remain;
    }
}

void E2prom_empty_all(bool flag)
{
    ESP_LOGI(TAG, "\nempty all set\n");

#ifdef FM24
    FM24C_Empty(E2P_SIZE / 8);
#else
    if (flag)
    {
        // for (uint16_t i = FN_SET_FLAG_ADD; i < E2P_USAGED; i++)
        // {
        //     AT24CXX_WriteOneByte(i, 0);
        // }
        at24c08_WriteData(FN_SET_FLAG_ADD, E2P_USAGED - FN_SET_FLAG_ADD, 0);
    }
    else
    {
        // for (uint16_t i = 0; i < E2P_SIZE; i++)
        // {
        //     AT24CXX_WriteOneByte(i, 0);
        // }
        at24c08_WriteData(0, E2P_SIZE, 0);
    }

#endif
}

static void E2prom_read_defaul(void)
{
    ESP_LOGI(TAG, "\nread defaul\n");

    // E2P_Read(SERISE_NUM_ADDR, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    // E2P_Read(PRODUCT_ID_ADDR, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    // E2P_Read(WEB_HOST_ADD, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    // E2P_Read(WEB_PORT_ADD, (uint8_t *)WEB_PORT, 5);
    // E2P_Read(MQTT_HOST_ADD, (uint8_t *)MQTT_SERVER, WEB_HOST_LEN);
    // E2P_Read(MQTT_PORT_ADD, (uint8_t *)MQTT_PORT, 5);

    // //备份
    // E2P_Write(SERISE_NUM_ADDR + E2P_SIZE / 2, (uint8_t *)SerialNum, SERISE_NUM_LEN);
    // E2P_Write(PRODUCT_ID_ADDR + E2P_SIZE / 2, (uint8_t *)ProductId, PRODUCT_ID_LEN);
    // E2P_Write(WEB_HOST_ADD + E2P_SIZE / 2, (uint8_t *)WEB_SERVER, WEB_HOST_LEN);
    // E2P_Write(WEB_PORT_ADD + E2P_SIZE / 2, (uint8_t *)WEB_PORT, 5);
    // E2P_Write(MQTT_HOST_ADD + E2P_SIZE / 2, (uint8_t *)MQTT_SERVER, WEB_HOST_LEN);
    // E2P_Write(MQTT_PORT_ADD + E2P_SIZE / 2, (uint8_t *)MQTT_PORT, 5);
}
//清空并写入默认值
//flag =1 写入序列号相关
//flag =0 不写入序列号相关
void E2prom_set_defaul(bool flag)
{
    Set_defaul_flag = true;
    // E2prom_read_defaul();
    E2prom_empty_all(flag);
    //写入默认值
    ESP_LOGI(TAG, "set defaul\n");

    E2P_WriteLenByte(FN_DP_ADD, 60, 4);
    E2P_WriteOneByte(CG_DATA_LED_ADD, 1);
    Set_defaul_flag = false;

    // esp_restart();
}

void E2prom_set_0XFF(void)
{
    Set_defaul_flag = true;
    // E2prom_read_defaul();
    at24c08_WriteData(0, E2P_SIZE, 0xff);
    //写入默认值

    Set_defaul_flag = false;
    ESP_LOGI(TAG, "set defaul\n");
}

//检查AT24CXX是否正常,以及是否为新EEPROM
//这里用了24XX的最后一个地址(1023)来存储标志字.
//如果用其他24C系列,这个地址要修改

static bool AT24CXX_Check(void)
{
    uint8_t Check_dat = 0x55;
    uint8_t temp;
// temp = E2P_ReadOneByte(E2P_SIZE - 1);
#ifdef FM24
    FM24C_Read((E2P_SIZE - 1), &temp, 1);

    if (temp == 0XFF)
    {
        // temp = E2P_ReadOneByte(E2P_SIZE - 2);
        FM24C_Read((E2P_SIZE - 10), &temp, 1);
        if (temp == 0XFF)
        {
            ESP_LOGI(TAG, "\nnew eeprom\n");
            E2prom_set_defaul(false);
        }
    }

    if (temp == Check_dat)
    {
        ESP_LOGI(TAG, "eeprom check ok!\n");
        return true;
    }
    else //排除第一次初始化的情况
    {
        FM24C_Write(E2P_SIZE - 1, &Check_dat, 1);
        FM24C_Read((E2P_SIZE - 1), &temp, 1);
        if (temp == Check_dat)
        {
            ESP_LOGI(TAG, "eeprom check ok!\n");
            return true;
        }
    }
    ESP_LOGI(TAG, "eeprom check fail!\n");

#else
    temp = AT24CXX_ReadOneByte(E2P_SIZE - 1);
    if (temp == 0XFF)
    {
        temp = AT24CXX_ReadOneByte(E2P_SIZE - 10);
        if (temp == 0XFF)
        {
            ESP_LOGI(TAG, "\nnew eeprom\n");
            E2prom_set_defaul(false);
        }
    }

    if (temp == Check_dat)
    {
        ESP_LOGI(TAG, "eeprom check ok!\n");
        return true;
    }
    else //排除第一次初始化的情况
    {
        AT24CXX_WriteOneByte((E2P_SIZE - 1), Check_dat);
        temp = AT24CXX_ReadOneByte(E2P_SIZE - 1);
        if (temp == Check_dat)
        {
            ESP_LOGI(TAG, "eeprom check ok!\n");
            return true;
        }
    }
    ESP_LOGI(TAG, "eeprom check fail!\n");
#endif

    return false;
}