#ifndef _OTA_H_
#define _OTA_H_

#include "freertos/FreeRTOS.h"

#define need_update_add 0x00
#define update_fail_num_add 0x01

uint8_t need_update;
uint8_t update_fail_num;
extern void ota_start(void);

#endif
