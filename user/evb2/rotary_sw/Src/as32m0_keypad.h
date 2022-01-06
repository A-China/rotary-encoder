#ifndef AS32M0_KEYPAD
#define AS32M0_KEYPAD
#include "as32m0.h"
#include "as32m0_gpio.h"
#include "as32m0_pinctrl.h"
void init_keypad(void);
uint8_t get_key(void);	
#endif
