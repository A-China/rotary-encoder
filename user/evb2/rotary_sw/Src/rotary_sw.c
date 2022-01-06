/*
 * rotary switch routine s
 *
 */
 
#include "rotary_sw.h"
 
 
#define ROTARY_PORT	GPIOA			// port A 
#define ROTARY_PUSH_SW (GPIO_Pin_5 << ROTARY_PORT)
#define ROTARY_PIN_A	(GPIO_Pin_6 << ROTARY_PORT)
#define ROTARY_PIN_B	(GPIO_Pin_7 << ROTARY_PORT)

typedef void (*rotateCoder_cb_t)(void);


int16_t direction_cnt;
rotateCoder_t rotary;
rotateCoder_cb_t rotary_cb = rotary_callback;

 
void rotary_callback(void) {
	
	if (rotary.forward) {
		printf("Forward -> %d\n", rotary.count);
		rotary.forward = false;
	}
	else if (rotary.backward) {
		printf("Backward -> %d\n", rotary.count);
		rotary.backward = false;
	}
	else if (rotary.push_switch) {
		printf("Rotary push sw\n");
		rotary.push_switch = false;
	}
}



/**
	* @brief  initial input pin generate interrupt with rising edge detect
  * @param  void 
  */
void rotary_pin_init(void) {
	
	PIN_INFO PIN_INFO;
	PIN_INFO.pin_func = GIO_FUNC0;
	PIN_INFO.pin_od = OD_ON;
	PIN_INFO.pin_stat = GIO_PD;
	PIN_INFO.pin_sonof = SONOF_OFF;
	PIN_INFO.pin_ds = GIO_DS_1_2;
	PinCtrl_GIOSet(PIN_CTL_GPIOA,  GPIO_Pin_6 | GPIO_Pin_7, &PIN_INFO);
	PIN_INFO.pin_stat = GIO_PU;
	PinCtrl_GIOSet(PIN_CTL_GPIOA,  GPIO_Pin_5, &PIN_INFO);
	APB_GPIO->GPIO_OE.CLR = (ROTARY_PIN_A ) | (ROTARY_PIN_B ) | (ROTARY_PUSH_SW );

	GPIO_InitTypeDef gpio_init;
	gpio_init.GPIO_Detect = GPIO_POSEDGE;
	gpio_init.GPIO_Mode   = GPIO_Mode_In;
	gpio_init.GPIO_Pin    = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;		// here should pin number, init ruotine will shift to corresponding port 
	GPIO_Init(ROTARY_PORT, &gpio_init);		
	
	GPIO_Set_Debouncing_Lmt(OSC_CLK_FREQ * 0.0005);			// 500uS
	GPIO_Debouncing_Bit_Enable(ROTARY_PORT, ROTARY_PIN_A | ROTARY_PIN_B | ROTARY_PUSH_SW);
	GPIO_Debouncing_Enable();		// active pin debouncing
	
}

void start_rotary_sw(void) {
	NVIC_ClearPendingIRQ(n15_GIO_IRQn);
	NVIC_EnableIRQ(n15_GIO_IRQn );	
	GPIO_ClrIntStat(ROTARY_PIN_A | ROTARY_PIN_B);			// 
	
}

void stop_rotary_sw(void) {
	NVIC_ClearPendingIRQ(n15_GIO_IRQn);
	NVIC_DisableIRQ(n15_GIO_IRQn );	
	
}

void n15_GIO_IRQHandler(void){
	
	uint32_t which_pin;
	NVIC_DisableIRQ(n15_GIO_IRQn);
	which_pin = APB_GPIO->GPIO_DI & (ROTARY_PIN_A | ROTARY_PIN_B | ROTARY_PUSH_SW);
//	printf("which pin 0x%08X\n", which_pin);
	if ((direction_cnt > 99) | (direction_cnt < -99)) {
		direction_cnt = 0;
		rotary.count = 0;
	}
		if (rotary_cb != NULL) {
		rotary_cb();				// run customer callback routine
	}
	switch (which_pin) {
		case 0x20:
//			printf("Rotary push sw\n");
			rotary.push_switch = true;
			GPIO_ClrIntStat(ROTARY_PUSH_SW);
			break;
		case 0x80:
		case 0xA0:
//			printf("Forward -> %d\n", direction_cnt);
		  direction_cnt++;
			rotary.count++;
			rotary.forward = true;
			rotary.backward = false;
			break;
		case 0x40:
		case 0x60:
//			printf("Backward <- %d\n", direction_cnt);
			direction_cnt--;
			rotary.count--;
			rotary.backward = true;
			rotary.forward = false;
			break;
		default:
			break;
	}

	NVIC_ClearPendingIRQ(n15_GIO_IRQn);
	GPIO_ClrIntStat(ROTARY_PIN_A | ROTARY_PIN_B);		// 
	NVIC_EnableIRQ(n15_GIO_IRQn);
}
