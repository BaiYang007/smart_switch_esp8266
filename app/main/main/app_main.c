#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "key.h"
#include "relay.h"
#include "wifi.h"
#include "smart.h"
#include "parseJSON.h"
#include "common.h"
#include "led.h"
#include "clock.h"
static const char *TAG = "app_main Log";


static void system_flash_init(void)
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

static void system_info_printf(void)
{
	printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
	//获取IDF版本
	printf("     SDK version:%s\n", esp_get_idf_version());
	//获取芯片可用内存
	printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
	//获取从未使用过的最小内存
	printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
	//获取芯片的内存分布，返回值具体见结构体 flash_size_map
	printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
	//获取mac地址（station模式）
	uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	sprintf(deviceUUID, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	sprintf((char *)deviceInfo, "{\"type\":\"%s\",\"mac\":\"%s\"}", DEVICE_TYPE, deviceUUID);
	//组建MQTT订阅的主题
	sprintf(MqttTopicSub, "/%s/%s/devSub", DEVICE_TYPE, deviceUUID);
	//组建MQTT推送的主题
	sprintf(MqttTopicPub, "/%s/%s/devPub", DEVICE_TYPE, deviceUUID);

	ESP_LOGI(TAG, "deviceUUID: %s", deviceUUID);
	ESP_LOGI(TAG, "deviceInfo: %s", deviceInfo);
	ESP_LOGI(TAG, "MqttTopicSub: %s", MqttTopicSub);
	ESP_LOGI(TAG, "MqttTopicPub: %s", MqttTopicPub);
}


/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{
	//创建事件组用于在多个任务之间同步和通信。
	wifi_event_group = xEventGroupCreate();
	system_event_group = xEventGroupCreate();

	system_flash_init();
	system_info_printf();
	wifi_init_config();

	xTaskCreate(clock_task, "clock_task", 1024, NULL, 13, NULL);
	xTaskCreate(key_task, "key_task", 2048, NULL, 14, NULL);
	xTaskCreate(led_task, "led_task", 1024, NULL, 15, NULL);
	xTaskCreate(relay_task, "relay_task", 1024, NULL, 16, NULL);
}
