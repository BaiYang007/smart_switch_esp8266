/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "clock.h"
#include "lwip/apps/sntp.h"

static uint16_t SysTimerMs[DELAYTIME_END];//DELAYTIME_END


const int SNTP_DONE_BIT = BIT0;
const int SNTP_START_BIT = BIT1;
const int REFRESH_RELAY_IO_BIT = BIT2;
EventGroupHandle_t system_event_group;

static const char *TAG = "clock Log";

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
//    设置SNTP客户端的工作模式为“轮询”（Polling）方式。在轮询方式下，
//    SNTP客户端会周期性地向指定的SNTP服务器发送请求，获取最新的时间戳
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
//    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(0,"ntp.aliyun.com");
    sntp_init();
}

static void obtain_time(void)
{
    time_t now = 0;
    char strftime_buf[64];
    struct tm timeinfo = { 0 };
    int retry = 0;

    initialize_sntp();
    //一直等待到网络时间同步完成
    while (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d)", retry++);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    //设置系统时区为中国标准时间
    setenv("TZ", "CST-8", 1);
    //调用tzset()函数来确保新的时区设置生效
    tzset();

    localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
	xEventGroupSetBits(system_event_group,SNTP_DONE_BIT);
}


static void delay_tick_init(void)
{
	uint16_t temp_for;
	for(temp_for = 0; temp_for < DELAYTIME_END; temp_for++ )
	{
		SysTimerMs[temp_for] = 0;
	}
}

uint16_t GetTimer(uint8_t addr)
{
	return(SysTimerMs[addr]);
}

/*
 * @Description:
 * @param:
 * @param:
 * @return:
*/
void SetTimer(uint8_t addr,uint16_t time) //单位10ms
{
	SysTimerMs[addr] = time;
}

void sntp_task(void *arg)
{
    time_t now;
    struct tm timeinfo;
    //等待连接上wifi后同步网络时间
	xEventGroupWaitBits(system_event_group,SNTP_START_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
    //获取时间戳
    time(&now);
    //格式化时间
    localtime_r(&now, &timeinfo);

	// Is time set? If not, tm_year will be (1970 - 1900).
	if (timeinfo.tm_year < (2016 - 1900)) {
	  ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
	  obtain_time();
	}

	vTaskDelete(NULL);
}

void clock_task(void *arg)
{
	unsigned short Timer_pc;

    xTaskCreate(sntp_task, "sntp_task", 2048, NULL, 17, NULL);

    delay_tick_init();

    for (;;)
    {
		for(Timer_pc = 0; Timer_pc < DELAYTIME_END; Timer_pc++ )
		{
			if(SysTimerMs[Timer_pc])SysTimerMs[Timer_pc]--;
		}
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}


