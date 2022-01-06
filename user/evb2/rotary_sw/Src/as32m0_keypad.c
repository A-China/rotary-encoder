#include "as32m0_keypad.h"
	
void init_keypad(void){
	PIN_INFO PIN_INFO;
	GPIO_InitTypeDef GPIO_InitDef;
	PIN_INFO.pin_func = GIO_FUNC0;
	PIN_INFO.pin_stat = GIO_PU;	
	PinCtrl_GIOSet(PIN_CTL_GPIOA, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 , &PIN_INFO);  
	GPIO_InitDef.GPIO_Detect	= GPIO_NONE;
	GPIO_InitDef.GPIO_Mode		= GPIO_Mode_In;
	GPIO_InitDef.GPIO_Pin			= GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitDef);
	GPIO_Set_Debouncing_Lmt(OSC_CLK_FREQ * 0.001);// 1ms
	APB_GPIO->GPIO_OE.SET = GPIOA_Pin_0;
	APB_GPIO->GPIO_DO.CLR = GPIOA_Pin_0;
	GPIO_Debouncing_Bit_Enable(GPIOA, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);
	GPIO_Debouncing_Enable();// DEBOUNCING Enable
}

uint8_t get_key(void){
	uint8_t key, prev_key;
	uint32_t timeout=0;
	prev_key = 0;
	key = (APB_GPIO->GPIO_DI & 0xF0<< GPIOA) >> 4;
	do{
		prev_key = key;
		timeout++;
		key = (APB_GPIO->GPIO_DI & 0xF0<< GPIOA) >> 4;
	}while(prev_key == key && (key != 0x0f ));
	
	return ~prev_key & 0x0F;
}
