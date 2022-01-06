#include "stdlib.h"
#include "printf_config.h"
#include "as32m0_pwm.h"
#include "as32m0_gpio.h"
#include "as32m0_pinctrl.h"
#include "as32m0_ssp.h"
#include "as32m0_misc.h"
#include "rotary_sw.h"

#define HAL_Delay( t ) delay_ms( t )
#define SEGA 0x01
#define SEGB 0x02
#define SEGC 0x08
#define SEGD 0x04
#define SEGE 0x80
#define SEGF 0x20
#define SEGG 0x40
#define SEGDD 0x10
#define LED0 (SEGA | SEGB | SEGC | SEGD | SEGE | SEGF )
#define LED1 (SEGB | SEGC )
#define LED2 (SEGA | SEGB | SEGD | SEGE | SEGG )
#define LED3 (SEGA | SEGB | SEGC | SEGD | SEGG )
#define LED4 (SEGB | SEGC | SEGF | SEGG )
#define LED5 (SEGA | SEGC | SEGC | SEGD | SEGF | SEGG )
#define LED6 (SEGA | SEGC | SEGD | SEGE | SEGF | SEGG )
#define LED7 (SEGA | SEGB | SEGC )
#define LED8 (SEGA | SEGB | SEGC | SEGD | SEGE | SEGF | SEGG)
#define LED9 (SEGA | SEGB | SEGC | SEGF | SEGG )

void _sys_exit(void);

//extern int16_t direction_cnt;
//extern rotateCoder_t rotary;


/**
	* @brief  initial APB_PWM3 (PWM6) for beepper (4KHz, 78:22)
  * @param  void 
  */
uint32_t init_beep(void) {
	
	int32_t ret;
	
	PWM_INFO pwm_info;
	pwm_info.int_en = 0;				// pwm interrupt 0=disable, 1=enable
	pwm_info.div = 2;
	pwm_info.lmt = 3000;
	pwm_info.ch_1_info.ch_n_point = 2000;			//@ SPWM ch1 only
	pwm_info.ch_1_info.ch_p_point = 0;
	pwm_info.ch_1_info.oc_en      = 1;
	pwm_info.ch_1_info.oen_n      = 1;
	pwm_info.ch_1_info.oen_p      = 1;
	pwm_info.ch_1_info.out_n      = 1;
	pwm_info.ch_1_info.out_p      = 0;

	//printf("Configing APB_PWM3 beep \n");
	ret = apPWM_Config(APB_PWM3, &pwm_info);				// APB_PWM3 is pin PWM6
	if(ret < 0){
		printf("[%d] Check CMD Status busy \n",__LINE__);
	}	
	APB_PINC->PB6.FuncSel = 0xF0; // SWITCH TO GPIO
	return ret;
}

void led_io_init(void) {
	PIN_INFO PIN_INFO;
	PIN_INFO.pin_func = GIO_FUNC0;
	PIN_INFO.pin_od = OD_OFF;
	PIN_INFO.pin_stat = GIO_PU;
	PIN_INFO.pin_sonof = SONOF_OFF;
	PIN_INFO.pin_ds = GIO_DS_2_4;
	PinCtrl_GIOSet(PIN_CTL_GPIOC, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2| GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7, &PIN_INFO);
	APB_GPIO->GPIO_OE.SET = 0xFF << GPIOC;
	APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
	
	PinCtrl_GIOSet(PIN_CTL_GPIOA, GPIOA_Pin_0, &PIN_INFO);
	APB_GPIO->GPIO_OE.SET = GPIO_Pin_0 << GPIOA;
	APB_GPIO->GPIO_DO.SET = GPIOA_Pin_0;
	
}


int main()
{
	int16_t k;
	// prepare printf (uart0)
	config_uart0(OSC_CLK_FREQ, UART_SETBAUD, UART0_PD23);
	printf("==> \n");
	
	led_io_init();
	APB_GPIO->GPIO_DO.CLR = LED3 << GPIOC;
	
//	rotary_input_init();
	rotary_pin_init();
	start_rotary_sw();
	__enable_irq();	
	
	printf("==> \n");
	printf("==> TEST rotary switch \n");
	printf("==> \n");
	printf("==> \n");
	// wait
	while(apUART_Check_BUSY(APB_UART0) == 1);
	
	k = direction_cnt % 10;
	init_beep();
	while(1) {
		switch (k) {
			case 0:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = (LED0 << 16);
				break;
			case 1:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED1 << 16;
				break;
			case 2:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED2 << 16;
				break;
			case 3:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED3 << GPIOC;
				break;
			case 4:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED4 << GPIOC;
				break;
			case 5:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED5 << GPIOC;
				break;
			case 6:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED6 << GPIOC;
				break;
			case 7:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED7 << GPIOC;
				break;
			case 8:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED8 << GPIOC;
				break;
			case 9:
				APB_GPIO->GPIO_DO.SET = 0xFF << GPIOC;
				APB_GPIO->GPIO_DO.CLR = LED9 << GPIOC;
				break;
			deafult:
				break;
		}
		k = abs(direction_cnt) % 10;
//		printf("--> %d\n", k);
//		k++;
		if (k > 9) k = 0;
		if (rotary.push_switch) {
			APB_GPIO->GPIO_DO.CLR = GPIOA_Pin_0;
			HAL_Delay(100);
			rotary.push_switch = 0;
			APB_GPIO->GPIO_DO.SET = GPIOA_Pin_0;
		}
		HAL_Delay(1);
		
	}
	
	
	//-------  System Config ----------
	
	
  printf("\n");
	printf("\n");
	printf("==> TEST rotary switch PASSED \n");
	printf("\n");
	printf("\n");
	printf("#"); // invoke TB $finish;
	_sys_exit();
 	return 0x12;
}

