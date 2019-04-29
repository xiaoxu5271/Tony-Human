#ifndef _W5500_SPI_H_
#define _W5500_SPI_H_

#include "driver/spi_master.h"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5

void spi_init(void);
void spi_test(void);
void spi_select_w5500(void);
void spi_deselect_w5500(void);
void spi_writebyte(uint8_t writebyte);
uint8_t spi_readbyte(void);
void spi_writeburst(uint8_t *write_buf, uint16_t buf_len);
void spi_readburst(uint8_t *read_buf, uint16_t buf_len);

#endif