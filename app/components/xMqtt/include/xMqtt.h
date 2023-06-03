#ifndef _XMQTT_H_
#define _XMQTT_H_
#include "mqtt_client.h"
extern esp_mqtt_client_handle_t client;
extern void TaskXMqttRecieve(void *p);
#endif
