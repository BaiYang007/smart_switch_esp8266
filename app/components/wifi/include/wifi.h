#ifndef _WIFI_H_
#define _WIFI_H_

//wifi初始配置
extern void wifi_init_config(void);

//wifi事件管理组
extern const int ESPTOUCH_DONE_BIT;
extern const int CONNECTED_BIT;
extern const int AIRKISS_DONE_BIT;
extern EventGroupHandle_t wifi_event_group;

#endif
