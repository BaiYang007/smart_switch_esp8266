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

#include <string.h>
#include <stdio.h>

#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#include "xMqtt.h"
#include "parseJSON.h"
#include "common.h"
#include "relay.h"
#include "clock.h"
static const char *TAG = "parseJSON Log";

User_data user_data;
xQueueHandle ParseJSONQueueHandler = NULL; //解析json数据的队列

char deviceUUID[17];
char MqttTopicSub[50], MqttTopicPub[50];

//是否连接服务器
bool isConnect2Server = false;

#define AT____SYNC       0X01
#define AT_UP_DATA       0X02
/*
 * @Description: 上报数据到服务器
 * @param: null
 * @return:
*/
void post_data_to_clouds(void)
{

	if (!isConnect2Server)
		return;

	cJSON *pRoot = cJSON_CreateObject();

//	cJSON *pHeader = cJSON_CreateObject();
	/* add 1st car to cars array */
//	cJSON_AddStringToObject(pHeader, "type", (DEVICE_TYPE));
//	cJSON_AddStringToObject(pHeader, "mac", deviceUUID);
	cJSON_AddStringToObject(pRoot, "mac", deviceUUID);
	cJSON_AddNumberToObject(pRoot, "RelaySwitchMask", EepromParameter_st.RelaySwitchMask);
//	cJSON *pRelay = cJSON_CreateObject();
//	cJSON_AddNumberToObject(pRelay,"RelayState",EepromParameter_st.RelayState);
//	cJSON_AddNumberToObject(pRelay,"ClockEnable",EepromParameter_st.ClockEnable);
//	cJSON_AddNumberToObject(pRelay,"ClockLen",EepromParameter_st.ClockLen);

//	cJSON *pAttr = cJSON_CreateArray();
//	for (int i = 0; i < 5; i++) {
//		cJSON *pAttrPower = cJSON_CreateObject();
//		cJSON_AddNumberToObject(pAttrPower, "start", EepromParameter_st.ClockList[i][0]);
//		cJSON_AddNumberToObject(pAttrPower, "end", EepromParameter_st.ClockList[i][1]);
//		cJSON_AddItemToArray(pAttr, pAttrPower);
//	}
//	cJSON_AddItemToObject(pRelay, "ClockList", pAttr);
//	cJSON_AddItemToObject(pRoot, "Relay", pRelay);

	char *s = cJSON_Print(pRoot);
	ESP_LOGI(TAG, "post_data_to_clouds topic end : %s", MqttTopicPub);
	ESP_LOGI(TAG, "post_data_to_clouds payload : %s", s);
	esp_mqtt_client_publish(client, MqttTopicPub, s, strlen(s), 1, 0);
	cJSON_free((void *)s);
	cJSON_Delete(pRoot);
}


/*
 * @Description: 解析下发数据的队列逻辑处理
 * @param: null
 * @return:
*/
void Task_ParseJSON(void *pvParameters)
{
	printf("[SY] Task_ParseJSON_Message creat ... \n");
	while (1)
	{
		struct __User_data *pMqttMsg;

		printf("[%d] Task_ParseJSON_Message xQueueReceive wait ... \n", esp_get_free_heap_size());
		xQueueReceive(ParseJSONQueueHandler, &pMqttMsg, portMAX_DELAY);

		////首先整体判断是否为一个json格式的数据
		cJSON *pJsonRoot = cJSON_Parse(pMqttMsg->allData);
		//如果是否json格式数据
		if (pJsonRoot == NULL)
		{
			printf("[SY] Task_ParseJSON_Message xQueueReceive not json ... \n");
			goto __cJSON_Delete;
		}

		cJSON *pJSON_Item = cJSON_GetObjectItem(pJsonRoot, "AT_CMD");
		if (pJSON_Item != NULL && pJSON_Item->type == cJSON_Number)
		{
			switch (pJSON_Item->valueint)
			{
			case AT____SYNC:
				break;
			case AT_UP_DATA:
				pJSON_Item = cJSON_GetObjectItem(pJsonRoot, "RelaySwitchMask");
				if (pJSON_Item != NULL && pJSON_Item->type == cJSON_Number)
				{
					EepromParameter_st.RelaySwitchMask = pJSON_Item->valueint;
				}
				else goto __cJSON_Delete;
				SetTimer(SaveTimer, 200);
				break;

			default: goto __cJSON_Delete;
			}
		}
		else goto __cJSON_Delete;


		post_data_to_clouds();

	__cJSON_Delete:
		cJSON_Delete(pJsonRoot);
	}
}

//{
//  "Common": 34,
//  "RelayState": 66,
//  "ClockEnable": 44,
//  "ClockLen": 5,
//  "ClockList": [
//    {"start": 22, "end": 2},
//    {"start": 1, "end": 2},
//    {"start": 1, "end": 2},
//    {"start": 1, "end": 2},
//    {"start": 1, "end": 2}
//  ]
//}
