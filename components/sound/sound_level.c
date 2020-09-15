#include <stdio.h>
#include <string.h>
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "sound_level.h"

#define ADC1_TEST_CHANNEL (6)
#define DEFAULT_VREF 1100 //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6; //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_11;	//ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

float value_voice = 0;

static void check_efuse(void)
{
	//Check TP is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
	{
		printf("eFuse Two Point: Supported\n");
	}
	else
	{
		printf("eFuse Two Point: NOT supported\n");
	}

	//Check Vref is burned into eFuse
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
	{
		printf("eFuse Vref: Supported\n");
	}
	else
	{
		printf("eFuse Vref: NOT supported\n");
	}
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
	if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
	{
		printf("Characterized using Two Point Value\n");
	}
	else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
	{
		printf("Characterized using eFuse Vref\n");
	}
	else
	{
		printf("Characterized using Default Vref\n");
	}
}

void adc1task(void *arg)
{
	check_efuse();
	// initialize ADC
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(channel, atten);

	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_10, DEFAULT_VREF, adc_chars);
	print_char_val_type(val_type);

	while (1)
	{
		float adc_reading = 0, adc_reading_2 = 0;
		float value_1 = 0;

		//Multisampling
		for (int i = 0; i < NO_OF_SAMPLES; i++)
		{
			adc_reading += adc1_get_raw((adc1_channel_t)channel);
		}
		adc_reading /= NO_OF_SAMPLES;
		//Convert adc_reading to voltage in mV
		float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

		if (adc_reading >= 0.29)
		{
			value_1 = 0.095702 * adc_reading + 0.0005;
			value_voice = (20 * log10(value_1 / 0.00039)) - 50;
		}

		printf("voice:%.1f,%.1f\n", value_voice, voltage);
		/*if ((voltage >= 500) && (voltage < 750))
        {
            value_voice = (voltage) / 1000 * 138;
            }
        if ((voltage >= 750) && (voltage < 1000))
        {
            value_voice = (voltage) / 1000 * 110;
        }
        if (voltage > 1000)
        {
            value_voice = (voltage) / 1000 * 70;
        }*/

		/*for (int i = 0; i < 5; i++)
        {
            a[i] = value_voice;
            //max = a[i];
            if (max > a[i])
            {
                max = a[i];
            }
        }

        printf(" max: %.1f\n", max);*/

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
void Sound_Init(void)
{
	xTaskCreate(adc1task, "adc1task", 1024 * 3, NULL, 10, NULL);
}
