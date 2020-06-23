#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "Json_parse.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "Uart0.h"

#define UART0_TXD (UART_PIN_NO_CHANGE)
#define UART0_RXD (UART_PIN_NO_CHANGE)
#define UART0_RTS (UART_PIN_NO_CHANGE)
#define UART0_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE 1024
static const char *TAG = "UART0";
QueueHandle_t Log_uart_queue;

void Uart0_Task(void *arg);

void Uart0_Init(void)
{
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

	uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 2, &Log_uart_queue, 0);
	uart_param_config(UART_NUM_0, &uart_config);
	uart_set_pin(UART_NUM_0, UART0_TXD, UART0_TXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	//串口0 接收解析
	xTaskCreate(Uart0_Task, "Uart0_Task", 5120, NULL, 9, NULL);
}

void Uart0_Task(void *arg)
{
	uart_event_t event;
	uint8_t data_u0[BUF_SIZE] = {0};
	uint16_t all_read_len = 0;
	while (1)
	{
		if (xQueueReceive(Log_uart_queue, (void *)&event, (portTickType)portMAX_DELAY))
		{
			switch (event.type)
			{
			case UART_DATA:
				if (event.size >= 1)
				{
					if (all_read_len + event.size >= BUF_SIZE)
					{
						ESP_LOGE(TAG, "read len flow");
						all_read_len = 0;
						memset(data_u0, 0, BUF_SIZE);
					}
					uart_read_bytes(UART_NUM_0, data_u0 + all_read_len, event.size, portMAX_DELAY);

					uxQueueMessagesWaiting(Log_uart_queue);

					all_read_len += event.size;
					data_u0[all_read_len] = 0; //去掉字符串结束符，防止字符串拼接不成功

					if (strstr((const char *)data_u0, "\n") != NULL || strstr((const char *)data_u0, "\r") != NULL)
					{
						ESP_LOGI(TAG, "uart0 recv,  len:%d,%s", strlen((const char *)data_u0), data_u0);
						ParseTcpUartCmd((char *)data_u0);
						all_read_len = 0;
						memset(data_u0, 0, BUF_SIZE);
						uart_flush(UART_NUM_0);
					}
				}
				break;

			case UART_FIFO_OVF:
				ESP_LOGI(TAG, "hw fifo overflow");
				uart_flush_input(UART_NUM_0);
				xQueueReset(Log_uart_queue);
				break;

			case UART_BUFFER_FULL:
				ESP_LOGI(TAG, "ring buffer full");
				uart_flush_input(UART_NUM_0);
				xQueueReset(Log_uart_queue);
				break;

			//Others
			default:
				// ESP_LOGI(TAG, "uart type: %d", event.type);
				break;
			}
		}
	}
}
