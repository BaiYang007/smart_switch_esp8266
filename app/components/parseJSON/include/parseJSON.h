#ifndef _PARSE_JSON_H_
#define _PARSE_JSON_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct __User_data
{
	char allData[1024];
	int dataLen;
} User_data;

extern User_data user_data;

extern void post_data_to_clouds(void);
extern char deviceUUID[17];
extern bool isConnect2Server;
extern void Task_ParseJSON(void *pvParameters);
extern xQueueHandle ParseJSONQueueHandler;
extern char MqttTopicSub[50], MqttTopicPub[50];

#endif
