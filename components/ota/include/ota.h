#ifndef _OTA_H_
#define _OTA_H_

#include "freertos/FreeRTOS.h"

uint8_t need_update;
uint8_t update_fail_num;
int8_t mid(char *src, char *s1, char *s2, char *sub);
extern void ota_start(void);

#endif
