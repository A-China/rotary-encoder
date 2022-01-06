#ifndef __ROTARY_SW_H__
#define __ROTARY_SW_H__

#include "as32m0.h"
#include "stdlib.h"
#include "as32m0_gpio.h"
#include "as32m0_pinctrl.h"
#include "printf_config.h"
#include "as32m0_misc.h"
#include <stdbool.h>


void rotary_input_init(void);
void rotary_pin_init(void);
void start_rotary_sw(void);
void rotary_callback(void);

typedef struct rotateCoder {
	int16_t count;
	bool forward;
	bool backward;
	bool push_switch;
} rotateCoder_t;

extern int16_t direction_cnt;
extern rotateCoder_t rotary;


#endif
