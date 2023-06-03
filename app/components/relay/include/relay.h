#ifndef _IOT_RELAYDRIVER_H_
#define _IOT_RELAYDRIVER_H_

typedef struct
{
	uint32_t RelaySwitchMask;
}EEPROMPARAMETER_ST;

extern EEPROMPARAMETER_ST  EepromParameter_st;
extern void relay_task(void *arg);
extern void toggle_relay(void);

#endif
