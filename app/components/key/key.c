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

#include "esp_log.h"
#include "esp_system.h"

#include "router.h"
#include "driver/gpio.h"
#include "include/key.h"
#include "led.h"
#include "parseJSON.h"
#include "relay.h"
#include <time.h>
#include "clock.h"

static const char *TAG = "key Log";

#define BOOT_KEY  0x01         //按键类型
static unsigned char keyShort; //按下触发
static unsigned char keyLong;  //长按触发
static unsigned char shortKey; //按下弹起触发
static unsigned char longKey;  //长按弹起触发

#define GPIO_INPUT_IO_0     0
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)


/**
 * @description: 按键硬件初始化
 * @param {type}
 * @return:
 */
static void button_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

/**
 * @description: 按键驱动
 * @param {type}
 * @return:
 */
static unsigned char key_driver(void)
{
	static unsigned char keyValue = 0;

	if (!gpio_get_level(GPIO_INPUT_IO_0))
	{
		keyValue |= BOOT_KEY;
	}
	else keyValue &= ~BOOT_KEY;

	return ~keyValue;
}

/**
 * @description: 按键扫描
 * @param {type}
 * @return:
 */
static void key_scan(void)
{
	static unsigned int keyLongBuff,longKeyBuff,LongKeyTimer;
	static unsigned char tmpReadData;
	unsigned char readData;

	readData = key_driver()^0xff;
	keyShort = readData & (readData ^ keyLongBuff);
	keyLongBuff = readData;

	if (!keyLongBuff)
	{
		keyLong = 0;
		LongKeyTimer = 300;
	}
	else
	{
		if (!LongKeyTimer)
		{
			keyLong = keyLongBuff;
			LongKeyTimer = 5;
		}
		else LongKeyTimer--;
	}

	if (readData) //计算按键按下到弹起的时间
	{
		longKeyBuff++;
		shortKey = longKey = 0;
		tmpReadData = readData;
	}
	else
	{
		if (longKeyBuff < 50) shortKey = tmpReadData;      //按键按下到弹起的时间小于500ms判定为短按
		else if (longKeyBuff > 300) longKey = tmpReadData; //按键按下到弹起的时间大于3s判定为长按
		else shortKey = longKey = 0;                       //按键按下到弹起的时间不在判断范围按键无效
		tmpReadData = longKeyBuff = 0;
	}

}
/**
 * @description: 按键任务
 * @param {type}
 * @return:
 */
void key_task(void *pvParameters)
{
	button_init();

    for (;;)
    {
    	key_scan();
    	if (keyShort == BOOT_KEY)
    	{
    		toggle_relay();
    		post_data_to_clouds();
    		SetTimer(SaveTimer, 1000);
    		ESP_LOGI(TAG, "Key Type: %s", "keyShort");
    	}
    	if (keyLong == BOOT_KEY)
    	{
    		ESP_LOGI(TAG, "Key Type: %s", "keyLong");
			router_wifi_clean_info();
			vTaskDelay(2500 / portTICK_RATE_MS);
			esp_restart();
			vTaskDelete(NULL);
    	}
    	if (shortKey == BOOT_KEY)
    	{
    		ESP_LOGI(TAG, "Key Type: %s", "shortKey");
    	}
    	if (longKey == BOOT_KEY)
    	{
    		ESP_LOGI(TAG, "Key Type: %s", "longKey");
    	}
    	vTaskDelay(10 / portTICK_RATE_MS);
    }
}
