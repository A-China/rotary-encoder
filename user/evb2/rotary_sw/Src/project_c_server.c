#include "printf_config.h"
#include "as32m0_gpio.h"
#include "as32m0_misc.h"
#include "as32m0_timer.h"
#include "as32m0_systick.h"
#include "as32m0_ecc.h"
#include "as32m0_des.h"
#include "as32m0_WL1601.h"
#include "as32m0_security_com.h"
#include "as32m0_keypad.h"
#include "as32m0_sha256.h"
#include "as32m0_nvr.h"
#include "string.h"
#include <stdlib.h>
extern void _sys_exit(void);
void init_timer(float second);
void init_timer_for_rolling(float second);
void start_timer(void);
void stop_timer(void);
void start_roll(void);
void stop_roll(void);
uint64_t generate_host_id(uint8_t *buf8 );
uint32_t generate_puf_seed(uint8_t *mesg);
uint8_t irq_occur=0;
uint32_t irq_count = 0;
void sw_to_rx_int(uint8_t power_gain, uint8_t channel);
extern ECC_POLY_TypeDef ecc_poly;

int main(void)
{
	uint32_t i, p, private_key;
	uint64_t buf64[8];
	uint8_t buf8[64], state_count=0;
	uint16_t packet_type=0;
	uint32_t sha256[8], recv_sha256[8], device_public_key_x, device_public_key_y, public_key_x, public_key_y, shared_key_x, shared_key_y;
	uint64_t host_ID, device_ID;
	uint32_t *tmp32_ptr;
	uint64_t *tmp64_ptr;
	
	GPIO_InitTypeDef gpio_init;
	gpio_init.GPIO_Detect = GPIO_POSEDGE;
	gpio_init.GPIO_Mode   = GPIO_Mode_In;
	gpio_init.GPIO_Pin    = GPIO_Pin_7;
	GPIO_Init(GPIOB, &gpio_init);		
	
	config_uart0(OSC_CLK_FREQ, UART_SETBAUD, UART0_PD23);
	memset(buf8, 0x55, 64);
	memset(buf64, 0xAA, 8);
	strcpy((char*)buf8,"Server48");
	printf("\n===>  Exchange CR ================\n");
	printf("===>  	\n");
	printf("===>  	\n");
	printf("===>  	\n");
	host_ID = generate_host_id(buf8);
	printf("generate_host_ID: %llx\n", host_ID);
	printf("generate_puf_seed: %x", generate_puf_seed(buf8));
	srand(generate_puf_seed(buf8));	
	p = prime_number_search(1200,0x000FFFF);
	printf("Random: %d \n", p);
	init_ecc_cure(2, 9, p);	
	init_keypad();
	APB_GPIO->GPIO_OE.SET = GPIOC_Pin_0;
	APB_GPIO->GPIO_DO.CLR = GPIOC_Pin_0;
	wl1601_init_spi(6);
	wl1601_init_RF();
	wl1601_init_TX(8, 110);
	WL1600_SetFlag_Mask(WL1600_Idle_Mode);
	printf("Ready\n");
//	init_timer_for_rolling(0.01);
	__enable_irq();		
//	start_roll();
	tmp64_ptr = (uint64_t *)FLASH_NVR1_BASE;
	if(*tmp64_ptr == 0x55AA55AA55AA55AA){
			state_count = 0x80;
			printf("Enrolled\n");
	}
	else
			state_count = 0x00;	
	while(1){
		packet_type = 0;
		__disable_irq();
		if(irq_occur==1){
			wl1601_rd_FIFO(63, buf8, 0);
			packet_type = 0x00; 
			irq_occur = 0;
			for(i=0 ; i<64; i++){
				if(i==63)
					buf8[63] = 0x00;
				else
					buf8[i] = buf8[i+1];
			}	
			printf("\nOrg: \n");
			for(i=0;i<64;i++){
				printf("%.2x", buf8[i]);
			}					
			buf8_to_buf64(buf8, buf64);
			ser_decrypt(buf64, 8, TDES, FROM_REG, DEFAULT_KEY0, DEFAULT_KEY1, DEFAULT_KEY2);
			buf64_to_buf8(buf8, buf64);
			printf("\nDecrypt: \n");
			for(i=0;i<64;i++){
				printf("%.2x", buf8[i]);
			}		
			if(create_packet_checksum(buf8) == buf8[55]){
				printf("\nChecksum OK\n");
				packet_type = buf8[1] << 8 |  buf8[0];
			}
		}		
		__enable_irq();
		switch (state_count){
			case 0x00:
				if(get_key()==0x01)
					state_count = 0x01;
				else
					state_count = 0x00;
				break;
			case 0x01: // Create packet
				stop_roll();
				create_packet_ser(TYPE_ENROLL, buf64, buf8, host_ID, 0);
				state_count = 0x02;
				break;
			case 0x02: // Send packet
				wl1601_sw_to_TX_send_pkt(8, 110, buf8, 63);
				state_count = 0x03;
				break;
			case 0x03:// Set to Rx
				sw_to_rx_int(8, 110);
				state_count = 0x04;
				break;
			case 0x04:
				if(packet_type==0xA010){ //Enroll
					state_count = 0x05;		
					private_key = rand();
					printf("\nprivate_key %d\n", private_key);
				}
				break;
			case 0x05:
				if(buf64[1] == host_ID){
					device_ID = buf64[2];
					device_public_key_x = buf64[3]>>32;
					device_public_key_y = (uint32_t)buf64[3];
					printf("device_ID : %llx\n", device_ID);
					printf("device_public_key_x : %d\n", device_public_key_x);
					printf("device_public_key_y : %d\n", device_public_key_y);
					ecc_n_time_g((uint32_t)private_key, &public_key_x, &public_key_y);		
					printf("Public Key: x: %d, y: %d\n", public_key_x, public_key_y);		
					ecc_point_time_g(private_key, device_public_key_x, device_public_key_y, &shared_key_x, &shared_key_y);
					printf("Shared key_: %d %d\n", shared_key_x, shared_key_y);
					state_count = 0x06;		
				}
				else
					state_count = 0x00;		
				break;
			case 0x06:
				buf64[1] = host_ID;
				buf64[2] = device_ID;
				buf64[3] = public_key_x;
				buf64[3] = buf64[3] << 16;
				buf64[3] = buf64[3] << 16;
				buf64[3] |= public_key_y;			
				buf64_to_buf8(buf8, buf64);
				create_packet_ser(TYPE_ENROLL2, buf64, buf8, host_ID, 0);
				state_count = 0x07;			
				break;
			case 0x07:// Send packet
				wl1601_sw_to_TX_send_pkt(8, 110, buf8, 63);
				state_count = 0x08;	
				break;
			case 0x08:// Set to Rx
				sw_to_rx_int(8, 110);
				state_count = 0x09;
				break;	
			case 0x09:// Set to Rx
				if(packet_type==0xA011){ //Enroll
					state_count = 0x10;		
					private_key = rand();
					printf("State 0x09\n");
				}
				break;
			case 0x10:
				if(buf64[1] == host_ID){
					recv_sha256[0] = (uint32_t)buf64[2];
					recv_sha256[1] = (uint32_t)(buf64[2]>>32);	
					recv_sha256[2] = (uint32_t)buf64[3];
					recv_sha256[3] = (uint32_t)(buf64[3]>>32);	
					recv_sha256[4] = (uint32_t)buf64[4];
					recv_sha256[5] = (uint32_t)(buf64[4]>>32);	
					recv_sha256[6] = (uint32_t)buf64[5];
					recv_sha256[7] = (uint32_t)(buf64[5]>>32);						
					buf64[2] = device_ID;
					buf64[3] = shared_key_x;
					buf64[4] = shared_key_y;		
					tmp32_ptr = (uint32_t *)&buf64[2];
					run_sha256(tmp32_ptr, 8*3*8, sha256);
					printf("sha256: %x%x%x%x%x%x%x%x\n", sha256[0], sha256[1], sha256[2], sha256[3], sha256[4], sha256[5], sha256[6], sha256[7]);	
					printf("recv_sha256: %x%x%x%x%x%x%x%x\n", recv_sha256[0], recv_sha256[1], recv_sha256[2], recv_sha256[3], recv_sha256[4], recv_sha256[5], recv_sha256[6], recv_sha256[7]);	
					for(i=0;i<8;i++){
						if(recv_sha256[i] != sha256[i]){
							state_count = 0x00;
							break;
						}
						else{
							state_count = 0x11;
						}
					}
				}
				else
					state_count = 0x00;
				break;
			case 0x11:
				buf64[1] = host_ID;
			  buf64[2] = device_ID;				
				buf64_to_buf8(buf8, buf64);
				create_packet_ser(TYPE_ENROLL_VERIFY_OK, buf64, buf8, host_ID, 0);
				wl1601_sw_to_TX_send_pkt(8, 110, buf8, 63);	
				state_count = 0x12;
				break;
			case 0x12: // Store and Complete Enroll
				printf("State 0x12\n");
				buf64[0] = 0x55AA55AA55AA55AA;
				buf64[1] = host_ID;
				buf64[2] = device_ID;
				buf64[3] = private_key;
				buf64[4] = (uint64_t)shared_key_y<<32 | shared_key_x;
				ser_encrypt(&buf64[1], 4, TDES, FROM_PUF, DEFAULT_KEY0, DEFAULT_KEY1, DEFAULT_KEY2);
				printf("BUF0: %llx\n", buf64[0]);
				printf("BUF1: %llx\n", buf64[1]);
				printf("BUF2: %llx\n", buf64[2]);
				printf("BUF3: %llx\n", buf64[3]);
				printf("BUF4: %llx\n", buf64[4]);
				nvr_erase(NVR1_sector);				
				nvr_prog_sector_word(FLASH_NVR1_BASE,(uint32_t*)buf64, 10);
				for(i=0;i<16;i++){
					printf("%d:Data %x\n",i, io_read(FLASH_NVR1_BASE + 4*i));
				}			
				state_count = 0x00;
				break;
			case 0x80:
				printf("State 0x80\n");
				tmp64_ptr = (uint64_t *)(FLASH_NVR1_BASE + 8);
				for(i=1;i<5;i++, tmp64_ptr++){
					buf64[i] = *tmp64_ptr;
				}
				printf("BUF0: %llx\n", buf64[0]);
				printf("BUF1: %llx\n", buf64[1]);
				printf("BUF2: %llx\n", buf64[2]);
				printf("BUF3: %llx\n", buf64[3]);
				printf("BUF4: %llx\n", buf64[4]);	
				printf("BUF5: %llx\n", buf64[5]);					
				ser_decrypt(&buf64[1], 4, TDES, FROM_PUF, DEFAULT_KEY0, DEFAULT_KEY1, DEFAULT_KEY2);
				host_ID = buf64[1];
				device_ID = buf64[2];
				private_key = buf64[3];
				shared_key_x = (uint32_t)buf64[4];
				shared_key_y = (uint32_t)(buf64[4]>>32);
				printf("device_ID %llx\n", device_ID);
				printf("host_id %llx\n", host_ID);
				printf("private_key %d\n", private_key);
				printf("shared_key_x %d\n", shared_key_x);
				printf("shared_key_y %d\n", shared_key_y);
				sw_to_rx_int(8, 110);					
				state_count = 0x81;	
				break;
			case 0x81:
					if(packet_type == TYPE_VERIFY_KEY){
						state_count = 0x82;
					}
				break;				
			case 0x82:
					if(host_ID == buf64[1]){
						recv_sha256[0] = (uint32_t)buf64[2];
						recv_sha256[1] = (uint32_t)(buf64[2]>>32);	
						recv_sha256[2] = (uint32_t)buf64[3];
						recv_sha256[3] = (uint32_t)(buf64[3]>>32);	
						recv_sha256[4] = (uint32_t)buf64[4];
						recv_sha256[5] = (uint32_t)(buf64[4]>>32);	
						recv_sha256[6] = (uint32_t)buf64[5];
						recv_sha256[7] = (uint32_t)(buf64[5]>>32);				
						buf64[2] = device_ID;
						buf64[3] = shared_key_x;
						buf64[4] = shared_key_y;		
						tmp32_ptr = (uint32_t *)&buf64[2];
						run_sha256(tmp32_ptr, 8*3*8, sha256);
						printf("sha256: %x%x%x%x%x%x%x%x\n", sha256[0], sha256[1], sha256[2], sha256[3], sha256[4], sha256[5], sha256[6], sha256[7]);	
						printf("recv_sha256: %x%x%x%x%x%x%x%x\n", recv_sha256[0], recv_sha256[1], recv_sha256[2], recv_sha256[3], recv_sha256[4], recv_sha256[5], recv_sha256[6], recv_sha256[7]);				
						printf("State 0x82\n");
						buf64[1] = host_ID;
						buf64[2] = device_ID;				
						buf64_to_buf8(buf8, buf64);
						create_packet_ser(TYPE_CHECK_OK, buf64, buf8, host_ID, 0);						
						wl1601_sw_to_TX_send_pkt(8, 110, buf8, 63);		
						state_count = 0x83;
						}
					else
						state_count = 0x00;
				break;
			case 0x83:// Set to Rx
				sw_to_rx_int(8, 110);	
				printf("GO TO STATE 0x84\n");
				state_count = 0x84;
				break;
			case 0x84:
					if(packet_type == TYPE_SEND_DATA){
						ser_decrypt(&buf64[2], 4, TDES, FROM_REG, (uint64_t)shared_key_x<<32 | shared_key_y, DEFAULT_KEY1, DEFAULT_KEY2);
						printf("BUF0: %llx\n", buf64[0]);
						printf("BUF1: %llx\n", buf64[1]);
						printf("BUF2: %llx\n", buf64[2]);
						printf("BUF3: %llx\n", buf64[3]);
						printf("BUF4: %llx\n", buf64[4]);	
						printf("BUF5: %llx\n", buf64[5]);
						state_count = 0x85;
					}				
				break;
			case 0x85:
				buf64[1] = host_ID;
				buf64[2] = device_ID;	
				buf64[3] = 0;	
			  buf64_to_buf8(buf8, buf64);
				create_packet_ser(TYPE_DATA_OK, buf64, buf8, host_ID, (uint64_t)shared_key_x<<32 | shared_key_y);
				wl1601_sw_to_TX_send_pkt(8, 110, buf8, 63);
				state_count = 0x83;
				break;
			default:
				break;
				
		}
			
	}
	
} 
void init_timer(float second){
	NVIC_ClearPendingIRQ(n10_TMR0_IRQn  );
	NVIC_EnableIRQ(n10_TMR0_IRQn );
	TMR_Clr_CNT(APB_TMR0);
	TMR_Set_CMP(APB_TMR0, OSC_CLK_FREQ*second); // NOTE : Make sure use osc clock
	TMR_Set_Op_Mode(APB_TMR0, TMR_CTL_OP_MODE_WRAPPING);
	TMR_Int_Disable(APB_TMR0);
	TMR_Disable(APB_TMR0);
}

void init_timer_for_rolling(float second){
	NVIC_ClearPendingIRQ(n11_TMR1_IRQn  );
	NVIC_EnableIRQ(n11_TMR1_IRQn );
	TMR_Clr_CNT(APB_TMR1);
	TMR_Set_CMP(APB_TMR1, OSC_CLK_FREQ*second); // NOTE : Make sure use osc clock
	TMR_Set_Op_Mode(APB_TMR1, TMR_CTL_OP_MODE_WRAPPING);
	TMR_Int_Disable(APB_TMR1);
	TMR_Disable(APB_TMR1);
}

void start_timer(void){
	irq_count = 0;
	__enable_irq();
	TMR_Int_Enable(APB_TMR0);
	TMR_Enable(APB_TMR0);	
}
	
void stop_timer(void){
	__disable_irq();
	TMR_Int_Disable(APB_TMR0);
	TMR_Disable(APB_TMR0);	
}

void start_roll(void){
	TMR_Int_Enable(APB_TMR1);
	TMR_Enable(APB_TMR1);	
}

void stop_roll(void){
	TMR_Int_Disable(APB_TMR1);
	TMR_Disable(APB_TMR1);	
}

uint64_t generate_host_id(uint8_t *buf8){
	uint64_t *tmp;
	tmp = (uint64_t *)buf8;
	ser_encrypt(tmp, 1, TDES, FROM_PUF, DEFAULT_KEY0, DEFAULT_KEY1, DEFAULT_KEY2);
	return *tmp;
}

uint32_t generate_puf_seed(uint8_t *mesg){
	uint64_t tmp;
	uint8_t i;
	tmp=0;
	for(i=0;i<8;i++, mesg++)
		tmp |= *mesg << 8*i;
	ser_encrypt(&tmp, 1, TDES, FROM_PUF, DEFAULT_KEY0, DEFAULT_KEY1, DEFAULT_KEY2);
	return (uint32_t)tmp &0xFFFFFFFF;
}

void sw_to_rx_int(uint8_t power_gain, uint8_t channel){
	wl1601_init_RF();
	wl1601_init_RX(power_gain, channel);	
	wl1601_prepare_rx(power_gain, channel); 
	GPIO_ClrIntStat(GPIOB_Pin_7);
	NVIC_ClearPendingIRQ(n15_GIO_IRQn);
	NVIC_EnableIRQ(n15_GIO_IRQn );
}

void n10_TMR0_IRQHandler()
{
	TMR_Int_Clr(APB_TMR0);
	irq_count++;
	if(irq_count==400){
		wl1601_init_spi(6);
		wl1601_init_RF();
		wl1601_init_TX(0, 110);
		WL1600_SetFlag_Mask(WL1600_Idle_Mode);	
		irq_count = 0;
	}	
}

void n11_TMR1_IRQHandler()
{
	TMR_Int_Clr(APB_TMR1);
	rand();
	rand();
	rand();
	rand();
}

void n15_GIO_IRQHandler(void){
	irq_occur = 1;
	printf("\n n15_GIO_IRQHandler occur\n");
	NVIC_ClearPendingIRQ(n15_GIO_IRQn);
	NVIC_DisableIRQ(n15_GIO_IRQn );	
	GPIO_ClrIntStat(GPIOB_Pin_7);
}

