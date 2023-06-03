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

#include "lwip/sockets.h"
#include "esp_wifi.h"
#include "airkiss.h"
#include "esp_log.h"
#include "mbedtls/base64.h"
#include "esp_err.h"
#include "router.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"

#include "wifi.h"
#include "smart.h"
#include "led.h"

static const char *TAG = "smart Log";

//近场发现自定义消息
uint8_t deviceInfo[100] = {};

static int sock_fd;
static xTaskHandle handleLlocalFind = NULL;

/*
 * @Description:  微信配网近场发现，可注释不要
 * @param:
 * @return:
*/
static void TaskCreatSocket(void *pvParameters)
{

	printf("TaskCreatSocket to create!\n");

	char rx_buffer[128];
	uint8_t tx_buffer[512];
	uint8_t lan_buf[300];
	uint16_t lan_buf_len;
	struct sockaddr_in server_addr;
	int sock_server; /* server socked */
	int err;
	int counts = 0;
	size_t len;
	//创建一个udp套接字,AF_INET:协议簇，SOCK_DGRAM:数据报套接字，0：udp协议
	sock_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_server == -1)
	{
		printf("failed to create sock_fd!\n");
		vTaskDelete(NULL);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("255.255.255.255");
	server_addr.sin_port = htons(LOCAL_UDP_PORT);

	err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1)
	{
		vTaskDelete(NULL);
	}

	//base64加密要发送的数据
	if (mbedtls_base64_encode(tx_buffer, strlen((char *)tx_buffer), &len, deviceInfo, strlen((char *)deviceInfo)) != 0)
	{
		printf("[xuhong] fail mbedtls_base64_encode %s\n", tx_buffer);
		vTaskDelete(NULL);
	}

	printf("[xuhong] mbedtls_base64_encode %s\n", tx_buffer);

	struct sockaddr_in sourceAddr;
	socklen_t socklen = sizeof(sourceAddr);
	while (1)
	{
		memset(rx_buffer, 0, sizeof(rx_buffer));
		int len = recvfrom(sock_server, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

		ESP_LOGI(TAG, "IP:%s:%d", (char *)inet_ntoa(sourceAddr.sin_addr), htons(sourceAddr.sin_port));
		//ESP_LOGI(TAG, "Received %s ", rx_buffer);

		// Error occured during receiving
		if (len < 0)
		{
			ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
			break;
		}
		// Data received
		else
		{
			rx_buffer[len] = 0;												   // Null-terminate whatever we received and treat like a string
			airkiss_lan_ret_t ret = airkiss_lan_recv(rx_buffer, len, &akconf); //检测是否为微信发的数据包
			airkiss_lan_ret_t packret;
			switch (ret)
			{
			case AIRKISS_LAN_SSDP_REQ:

				lan_buf_len = sizeof(lan_buf);
				//开始组装打包
				packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, ACCOUNT_ID, tx_buffer, 0, 0, lan_buf, &lan_buf_len, &akconf);
				if (packret != AIRKISS_LAN_PAKE_READY)
				{
					ESP_LOGE(TAG, "Pack lan packet error!");
					continue;
				}
				ESP_LOGI(TAG, "Pack lan packet ok ,send: %s", tx_buffer);
				//发送至微信客户端
				int err = sendto(sock_server, (char *)lan_buf, lan_buf_len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
				if (err < 0)
				{
					ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				}
				else if (counts++ > COUNTS_BOACAST)
				{
					shutdown(sock_fd, 0);
					closesocket(sock_fd);
					handleLlocalFind = NULL;
					vTaskDelete(NULL);
				}
				break;
			default:
				break;
			}
		}
	}
}

/*
 * @Description: 配网回调
 * @param:
 * @param:
 * @return:
*/
static void sc_callback(smartconfig_status_t status, void *pdata)
{
	switch (status)
	{
	//已经成功连接到一个 Wi-Fi 热点获取了 临时IP地址，之后向路由器发送 DHCP 请求，请求分配一个可用的 IP 地址。
	//这个过程需要一定的时间，通常需要几秒钟到几十秒的时间，ESP8266 连接 Wi-Fi 热点时，需要先建立连接并获得一个本地 IP地址，
	//然后再向路由器请求分配一个可用的 IP 地址，这两个步骤是需要顺序执行的
	case SC_STATUS_LINK:
	{
		wifi_config_t *wifi_config = pdata;
		ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
		ESP_ERROR_CHECK(esp_wifi_disconnect());
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
		ESP_ERROR_CHECK(esp_wifi_connect());

		router_wifi_save_info(wifi_config->sta.ssid, wifi_config->sta.password);
	}
	break;
	//表示 Wi-Fi 连接已经完成， ESP8266 成功通过DHCP请求获取到 IP地址
	case SC_STATUS_LINK_OVER:
		ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
		//这里乐鑫回调目前在master分支已区分是否为微信配网还是esptouch配网，当airkiss配网才近场回调！
		if (pdata != NULL)
		{
			sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
			switch (sc_callback_data->type)
			{
			case SC_ACK_TYPE_ESPTOUCH:
				ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
				ESP_LOGI(TAG, "TYPE: ESPTOUCH");
				xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
				break;
			case SC_ACK_TYPE_AIRKISS:
				ESP_LOGI(TAG, "TYPE: AIRKISS");
				xEventGroupSetBits(wifi_event_group, AIRKISS_DONE_BIT);
				break;
			default:
				ESP_LOGE(TAG, "TYPE: ERROR");
				xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
				break;
			}
		}

		break;
	default:
		break;
	}
}


/*
 * @Description:  一键配网
 * @param:
 * @return:
*/
void TaskSmartConfigAirKiss2Net(void *parm)
{
	EventBits_t uxBits;
	//判别是否自动连接
	bool isAutoConnect = routerStartConnect();
	//是的，则不进去配网模式，已连接路由器
	if (isAutoConnect)
	{
		ESP_LOGI(TAG, "Next connectting router.");
		vTaskDelete(NULL);
	}
	//否，进去配网模式
	else
	{
		set_led_strobe(LED_STROBE_FAST);
		ESP_LOGI(TAG, "into smartconfig mode");
		ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
		ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
	}

	//阻塞等待配网完成结果
	while (1)
	{
		uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | AIRKISS_DONE_BIT, true, false, portMAX_DELAY);
		// 微信公众号配网完成
		if (uxBits & AIRKISS_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , start find device");
			esp_smartconfig_stop();

			//把设备信息通知到微信公众号
			if (handleLlocalFind == NULL)
				xTaskCreate(TaskCreatSocket, "TaskCreatSocket", 1024 * 2, NULL, 6, &handleLlocalFind);

			ESP_LOGI(TAG, "getAirkissVersion %s", airkiss_version());

			vTaskDelete(NULL);
		}
		// smartconfig配网完成
		if (uxBits & ESPTOUCH_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over , but don't find device by airkiss...");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
}
