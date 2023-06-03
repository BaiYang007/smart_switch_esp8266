/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "include/relay.h"
#include "nvs_flash.h"
#include <string.h>
#include "esp_log.h"
#include "clock.h"
#include <time.h>
#include "esp_system.h"
#define RELAY1_BIT       1
#define RELAY2_BIT       2

#define HIGH       1
#define LOW        0
#define SET_RELAY1_LEVEL(a)        gpio_set_level(GPIO_OUTPUT_IO_12,a);
#define SET_RELAY2_LEVEL(a)        gpio_set_level(GPIO_OUTPUT_IO_13,a);

static const char *TAG = "relay Log";
//relay配置参数
EEPROMPARAMETER_ST  EepromParameter_st;
EEPROMPARAMETER_ST  TempEepromParameter_st;
//relay io口定义
#define GPIO_OUTPUT_IO_12    12
#define GPIO_OUTPUT_PIN12_SEL  (1ULL<<GPIO_OUTPUT_IO_12)
#define GPIO_OUTPUT_IO_13    13
#define GPIO_OUTPUT_PIN13_SEL  (1ULL<<GPIO_OUTPUT_IO_13)

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
static void relay_driver_Init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN12_SEL|GPIO_OUTPUT_PIN13_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_12,0);
    gpio_set_level(GPIO_OUTPUT_IO_13,0);
}

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
static void eeprom_parameter_save(void)
{
    nvs_handle out_handle;

    TempEepromParameter_st = EepromParameter_st;
    if (nvs_open("EepromParameter", NVS_READWRITE, &out_handle) == ESP_OK)
    {
        if (nvs_set_blob(out_handle, "EepromParameter", &TempEepromParameter_st, sizeof(TempEepromParameter_st)) == ESP_OK)
        {
        	ESP_LOGI(TAG, "EepromParameter save success");
        }
        nvs_close(out_handle);
    }
}

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
static void eeprom_parameter_init(void)
{
    size_t size = 0;
    nvs_handle out_handle;

    if (nvs_open("EepromParameter", NVS_READWRITE, &out_handle) == ESP_OK)
    {
        size = sizeof(EepromParameter_st);

        if (nvs_get_blob(out_handle, "EepromParameter", &EepromParameter_st, &size) == ESP_OK)
        {
        	ESP_LOGI(TAG, "read relayParameter success");
        }
        else if (nvs_set_blob(out_handle, "EepromParameter", &EepromParameter_st, size) == ESP_OK)
        {
        	ESP_LOGI(TAG, "create relayParameter success");
        }
        else ESP_LOGI(TAG, "read relayParameter error");

        nvs_close(out_handle);
    }
}


/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
static void relay_dispatch(void)
{
	static uint8_t lastRelayState,lastAlarmClock=0xff,AlarmClockRunFlag = 0;
	uint8_t alarmColck,redoRelayStateFlag = 0;

	//网络时间同步完成
	if (SNTP_DONE_BIT & xEventGroupWaitBits(system_event_group,SNTP_DONE_BIT,pdTRUE,pdFALSE,0))
	{

	}

	if (lastRelayState != EepromParameter_st.RelaySwitchMask)
	{
		lastRelayState = EepromParameter_st.RelaySwitchMask;
		xEventGroupSetBits(system_event_group,REFRESH_RELAY_IO_BIT);
	}


////////////////////////////////数据保存/////////////////////////////////////////////////////////////////
	if (GetTimer(SaveTimer) == 1)
	{
		SetTimer(SaveTimer,0);
		eeprom_parameter_save();
	}
}

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
static void relay_level_output(void)
{
	if (REFRESH_RELAY_IO_BIT & xEventGroupWaitBits(system_event_group,REFRESH_RELAY_IO_BIT,pdTRUE,pdFALSE,0))
	{
		if (EepromParameter_st.RelaySwitchMask & RELAY1_BIT)
		{
			SET_RELAY1_LEVEL(HIGH);
			ESP_LOGI(TAG, "relay1 on");
		}
		else
		{
			SET_RELAY1_LEVEL(LOW);
			ESP_LOGI(TAG, "relay1 off");
		}

		if (EepromParameter_st.RelaySwitchMask & RELAY2_BIT)
		{
			SET_RELAY2_LEVEL(HIGH);
			ESP_LOGI(TAG, "relay2 on");
		}
		else
		{
			SET_RELAY2_LEVEL(LOW);
			ESP_LOGI(TAG, "relay2 off");
		}
	}
}

/*
 * @Description:反转继电器电平通断
 * @param:
 * @param:
 * @return:
*/
void toggle_relay(void)
{
	if (EepromParameter_st.RelaySwitchMask & (RELAY1_BIT|RELAY2_BIT))
	{
		EepromParameter_st.RelaySwitchMask &= ~(RELAY1_BIT|RELAY2_BIT);
	}
	else EepromParameter_st.RelaySwitchMask |= (RELAY1_BIT|RELAY2_BIT);

	xEventGroupSetBits(system_event_group,REFRESH_RELAY_IO_BIT);
}

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
void relay_task(void *arg)
{
	relay_driver_Init();
	eeprom_parameter_init();

	for (;;)
	{
		relay_dispatch();
		relay_level_output();
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}



