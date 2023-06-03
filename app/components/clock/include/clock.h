#ifndef __CLOCK__H
#define __CLOCK__H
#include "freertos/event_groups.h"

extern const int SNTP_START_BIT;
extern const int SNTP_DONE_BIT;
extern const int REFRESH_RELAY_IO_BIT;

enum
{
	SaveTimer,
	RelayAlarmClockTimer,
//将延时常数放在上面
	DELAYTIME_END
};

extern EventGroupHandle_t system_event_group;
extern void clock_task(void *arg);
extern void SetTimer(uint8_t addr,uint16_t time) ;
extern uint16_t GetTimer(uint8_t addr);

#endif
