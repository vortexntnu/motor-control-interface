#include "efm32gg990f1024.h"
#include <string.h>

#include "../include/uart.h"
#include "../include/pwm.h"

int main() {

	CHIP_Init();

	initUart();
	//initPwm();
	initPwm2();

	uint32_t i;
	char hello_world[] = "\n\rHello World!\n\r";
	char receive_buff;

	for(i=0; i<strlen(hello_world); i++) USART_Tx(UART, hello_world[i]);

	i = 0;

	while(1){

		receive_buff = USART_Rx(UART);
		USART_Tx(UART, receive_buff);
		//generate_PWM();

	}

}


