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
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#include "smart.h"
#include "parseJSON.h"
#include "xMqtt.h"
#include "led.h"
#include "clock.h"

const int CONNECTED_BIT = BIT0;
const int ESPTOUCH_DONE_BIT = BIT1;
const int AIRKISS_DONE_BIT = BIT2;

//WIFI管理事件组
EventGroupHandle_t wifi_event_group;
//MQTT任务的句柄指针
static xTaskHandle handleMqtt = NULL;
//CJSON解析任务的句柄指针
static xTaskHandle mHandlerParseJSON = NULL;	  //任务队列


/*
 * @Description:  系统的wifi协议栈回调
 * @param:
 * @param:
 * @return:
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		set_led_strobe(LED_STROBE_MEDIUM);
		xTaskCreate(TaskSmartConfigAirKiss2Net, "TaskSmartConfigAirKiss2Net", 1024 * 2, NULL, 3, NULL);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
	{
		xEventGroupSetBits(system_event_group,SNTP_START_BIT);
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		int ret = pdFAIL;
		if (handleMqtt == NULL)
			ret = xTaskCreate(TaskXMqttRecieve, "TaskXMqttRecieve", 1024 * 4, NULL, 5, &handleMqtt);
		else
		{
			esp_mqtt_client_start(client);
			printf("esp_mqtt_client_start.\n");
		}
		if (ParseJSONQueueHandler == NULL)
			ParseJSONQueueHandler = xQueueCreate(5, sizeof(struct esp_mqtt_msg_type *));

		//开启json解析线程
		if (mHandlerParseJSON == NULL)
		{
			xTaskCreate(Task_ParseJSON, "Task_ParseJSON", 4096, NULL, 3, &mHandlerParseJSON);
		}

		if (ret != pdPASS)
		{
			printf("create TaskXMqttRecieve thread failed.\n");
		}

		break;
	}

	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		//esp_mqtt_client_stop(client);
		set_led_strobe(LED_STROBE_MEDIUM);
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		isConnect2Server = false;

		break;
	default:
		break;
	}
	return ESP_OK;
}


void wifi_init_config(void)
{

///////////////////wifi模块初始化配置/////////////////////////////////////
	//它用于初始化 TCP/IP 适配器 配置端口、ip、网关
	tcpip_adapter_init();
	//注册wifi事件回调函数
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	//宏，它用于初始化 WiFi配置,包括：WiFi 模式、信道、带宽、加密方式、SSID 和密码等。
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	//wifi模式设置
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	//启动wifi模块
	ESP_ERROR_CHECK(esp_wifi_start());
}
