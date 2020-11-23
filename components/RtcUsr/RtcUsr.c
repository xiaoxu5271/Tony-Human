#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "RtcUsr.h"

static const char *TAG = "RtcUsr";

void Rtc_Set(int year, int mon, int day, int hour, int min, int sec)
{
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    time_t t = mktime(&tm);
    ESP_LOGI(TAG, "Setting time: %s", asctime(&tm));
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
}

void Rtc_Read(int *year, int *month, int *day, int *hour, int *min, int *sec)
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);
    *year = (1900 + p->tm_year);
    *month = (1 + p->tm_mon);
    *day = p->tm_mday;
    *hour = p->tm_hour;
    *min = p->tm_min;
    *sec = p->tm_sec;
    //ESP_LOGI(TAG, "Read:%d-%d-%d %d:%d:%d No.%d",(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec,(1+p->tm_yday));
}