#ifndef _LED_H_
#define _LED_H_

typedef enum
{
	LED_STROBE_DISABLE = 0,
	LED_STROBE_FAST,
	LED_STROBE_MEDIUM,
	LED_STROBE_SLOW
}Led_Strobe_en;

extern void set_led_strobe(Led_Strobe_en srtobeSpeed);
extern void led_task(void *pvParameters);

#endif
