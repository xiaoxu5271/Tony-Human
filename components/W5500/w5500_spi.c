#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "w5500_spi.h"

/*
This ISR is called when the handshake line goes high.
*/

spi_device_handle_t spi;
spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1

};

//Configuration for the SPI device on the other side of the bus
spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .clock_speed_hz = 25 * 1000 * 1000,
    .duty_cycle_pos = 128, //50% duty cycle
    .mode = 0,
    .spics_io_num = -1, //PIN_NUM_CS,           //W5500需要自己调用该引脚的开关
                        //     .cs_ena_posttrans = 3, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
    .queue_size = 7,
    .flags = 0

};

void w5500_spi_init(void)
{
        /************初始化CS引脚***********/
        gpio_config_t io_conf;
        //disable interrupt
        io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
        //set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT;
        //bit mask of the pins that you want to set,e.g.GPIO16
        io_conf.pin_bit_mask = (1 << PIN_NUM_CS);
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //disable pull-up mode
        io_conf.pull_up_en = 1;
        //configure GPIO with the given settings
        gpio_config(&io_conf);
        gpio_set_level(PIN_NUM_CS, 1);

        /**************安装SPI设备*************/
        esp_err_t ret;
        //Initialize the SPI bus
        ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
        ESP_ERROR_CHECK(ret);
        //Attach the LCD to the SPI bus
        ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
        ESP_ERROR_CHECK(ret);
}

void spi_select_w5500(void)
{
        gpio_set_level(PIN_NUM_CS, 0);
}

void spi_deselect_w5500(void)
{
        gpio_set_level(PIN_NUM_CS, 1);
}

/**
  * @brief  从SPI写入一个字节
  * @retval 
  */
void spi_writebyte(uint8_t writebyte)
//void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
        esp_err_t ret;
        spi_transaction_t t;
        memset(&t, 0, sizeof(t)); //Zero out the transaction
        t.flags = SPI_TRANS_USE_TXDATA;
        t.length = sizeof(uint8_t) * 8; //Command is 8 bits
        t.tx_data[0] = writebyte;       //The data is the cmd itself
        //xSemaphoreTake(rdySem, 100);        //portMAX_DELAY); //Wait until slave is ready
        ret = spi_device_transmit(spi, &t); //Transmit!
        assert(ret == ESP_OK);              //Should have had no issues.
}

/**
  * @brief  从SPI读一个字节
  * @retval  读到的数据
  */
uint8_t spi_readbyte(void)
{
        //spi_writebyte(0x00);
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 8;
        t.flags = SPI_TRANS_USE_RXDATA;

        esp_err_t ret = spi_device_transmit(spi, &t);
        assert(ret == ESP_OK);
        // printf("RXDATA: %04X\n", t.rx_data[0]);
        return (uint8_t)t.rx_data[0];
}

/**
  * @brief  从SPI写多个字节
  * @retval  
  */
void spi_writeburst(uint8_t *write_buf, uint16_t buf_len)
//void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
        uint16_t i;
        for (i = 0; i < buf_len; i++)
        {
                spi_writebyte(write_buf[i]);
        }
}

/**
  * @brief  从SPI读多个字节
  * @retval  返回读到的数据
  */
void spi_readburst(uint8_t *read_buf, uint16_t buf_len)
{
        uint16_t i;
        for (i = 0; i < buf_len; i++)
        {
                read_buf[i] = spi_readbyte();
        }
}
