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

#include "include/led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

static const char *TAG = "led Log";

typedef enum
{
	LED_STATE_OFF = 0,
	LED_STATE_ON
}Led_State_en;

typedef struct
{
	Led_State_en LedState;
	Led_Strobe_en StrobeSpeed;
}LED_PARAMETERS_ST;

LED_PARAMETERS_ST LedParameters_st = {LED_STATE_OFF,LED_STROBE_DISABLE};

#define GPIO_INPUT_IO_4     4
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_4)
#define LED__ON  gpio_set_level(GPIO_INPUT_IO_4,0)
#define LED_OFF  gpio_set_level(GPIO_INPUT_IO_4,1)

/**
 * @description: led硬件初始化
 * @param {type}
 * @return:
 */
static void led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
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
 * @description: led频闪速度控制
 * @param {type}
 * @return:
 */
void set_led_strobe(Led_Strobe_en srtobeSpeed)
{
	LedParameters_st.StrobeSpeed = srtobeSpeed;
}

/**
 * @description: led任务分配
 * @param {type}
 * @return:
 */
void led_dispatch(void)
{
	static unsigned char timerCnt = 0, revFlag = 0, lastStrobeSpeed = 0;
	const uint8_t ledStrobeArr[4] = {0,10,50,100};

////////////////////////////////LED频闪控制//////////////////////////////////////////////////
	if (!ledStrobeArr[LedParameters_st.StrobeSpeed]) {
		LedParameters_st.LedState = LED_STATE_ON;
	} else {
		if (lastStrobeSpeed != LedParameters_st.StrobeSpeed) {
			if (timerCnt > ledStrobeArr[LedParameters_st.StrobeSpeed]) {
				timerCnt = ledStrobeArr[LedParameters_st.StrobeSpeed];
			}
			lastStrobeSpeed = LedParameters_st.StrobeSpeed;
		}

		if (!timerCnt) {
			revFlag = !revFlag;
			timerCnt = ledStrobeArr[LedParameters_st.StrobeSpeed];
		} else timerCnt--;

		if (revFlag) LedParameters_st.LedState = LED_STATE_ON;
		else LedParameters_st.LedState = LED_STATE_OFF;
	}
}

void led_driver(void)
{
	static unsigned char lastLedState;
	if (lastLedState !=  LedParameters_st.LedState) {
		lastLedState = LedParameters_st.LedState;
	   	if(LedParameters_st.LedState == LED_STATE_ON) {
			LED__ON;
		}
	    else LED_OFF;
	}
}

/**
 * @description: led任务
 * @param {type}
 * @return:
 */
void led_task(void *pvParameters)
{
	led_init();

    for (;;)
    {
    	led_dispatch();
    	led_driver();
    	vTaskDelay(10 / portTICK_RATE_MS);
    }
}
