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
#include "esp_log.h"
#include "mqtt_client.h"

#include "xMqtt.h"
#include "parseJSON.h"
#include "led.h"
static const char *TAG = "xMqtt Log";

//客户端的结构体指针类型，用于表示创建并配置后的 MQTT客户端实例
esp_mqtt_client_handle_t client;

/*
 * @Description: MQTT服务器的下发消息回调
 * @param:
 * @return:
*/
esp_err_t MqttCloudsCallBack(esp_mqtt_event_handle_t event)
{
	int msg_id;
	client = event->client;
	switch (event->event_id)
	{
		//连接成功
	case MQTT_EVENT_CONNECTED:
		msg_id = esp_mqtt_client_subscribe(client, MqttTopicSub, 1);
		ESP_LOGI(TAG, "sent subscribe[%s] successful, msg_id=%d", MqttTopicSub, msg_id);
		ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
		//post_data_to_clouds();
		isConnect2Server = true;
		break;
		//断开连接回调
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		isConnect2Server = false;
		break;
		//订阅成功
	case MQTT_EVENT_SUBSCRIBED:
		set_led_strobe(LED_STROBE_DISABLE);
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
		//订阅失败
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
		//推送发布消息成功
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
		//服务器下发消息到本地成功接收回调
	case MQTT_EVENT_DATA:
	{
		ESP_LOGI(TAG, " xQueueReceive  data [%s] \n", event->data);
		//发送数据到队列
		struct __User_data *pTmper;
		sprintf(user_data.allData, "%s", event->data);
		pTmper = &user_data;
		user_data.dataLen = event->data_len;
		xQueueSend(ParseJSONQueueHandler, (void *)&pTmper, portMAX_DELAY);
		break;
	}
	default:
		break;
	}
	return ESP_OK;
}


/*
 * @Description: MQTT参数连接的配置
 * @param:
 * @return:
*/
void TaskXMqttRecieve(void *p)
{
	//连接的配置参数
	esp_mqtt_client_config_t mqtt_cfg = {
		.host = "broker-cn.emqx.io", //连接的域名 ，请务必修改为您的
		.port = 1883,			   //端口，请务必修改为您的
		.username = "",	   //用户名，请务必修改为您的
		.password = "",   //密码，请务必修改为您的
		.client_id = deviceUUID,
		.event_handle = MqttCloudsCallBack, //设置回调函数
		.keepalive = 120,					//心跳
		.disable_auto_reconnect = false,	//开启自动重连
		.disable_clean_session = false,		//开启 清除会话
	};
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);
	vTaskDelete(NULL);
}
