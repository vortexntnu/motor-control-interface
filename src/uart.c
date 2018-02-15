#include "../include/uart.h"

#include "efm32gg990f1024.h"

void initUart(void)
{
	USART_Reset(UART);

	CMU->CTRL |= (1 << 14); 	                    	// Set HF clock divider to /2 to keep core frequency <32MHz
	CMU->OSCENCMD |= 0x4;                           	// Enable XTAL Oscillator

	//CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_2);
	//CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

	while(! (CMU->STATUS & 0x8) );                  // Wait for XTAL osc to stabilize
	CMU->CMD = 0x2;                                 // Select HF XTAL osc as system clock source. 48MHz XTAL, but we divided the system clock by 2, therefore our HF clock should be 24MHz

	//CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

	CMU->HFPERCLKEN0 = (1 << 13) | (1 << 1);        // Enable GPIO, and USART1 peripheral clocks

	//CMU_ClockEnable(cmuClock_GPIO, true);            // Enable GPIO peripheral clock
	//CMU_ClockEnable(cmuClock_USART1, true);          // Enable USART1 peripheral clock

	GPIO->P[COM_PORT].MODEL = (1 << 4) | (4 << 0);  // Configure PD0 as digital output and PD1 as input
	GPIO->P[COM_PORT].DOUTSET = (1 << UART_TX_pin); // Initialize PD0 high since UART TX idles high (otherwise glitches can occur)

	//GPIO_PinModeSet(COM_PORT, UART_TX_pin, gpioModePushPull, 1);
	//GPIO_PinModeSet(COM_PORT, UART_RX_pin, gpioModeInput, 0);

	// Use default value for USART1->CTRL: asynch mode, x16 OVS, lsb first, CLK idle low
	// Default frame options: 8-none-1-none
	USART1->CLKDIV = (152 << 6);                               // 152 will give 38400 baud rate (using 16-bit oversampling with 24MHz peripheral clock)
	USART1->CMD = (1 << 11) | (1 << 10) | (1 << 2) | (1 << 0); // Clear RX/TX buffers and shif regs, Enable Transmitter and Receiver
	USART1->IFC = 0x1FF9;                                      // clear all USART interrupt flags
	USART1->ROUTE = 0x103;                                     // Enable TX and RX pins, use location #1 (UART TX and RX located at PD0 and PD1, see EFM32GG990 datasheet for details)

	/*USART_InitAsync_TypeDef uartInit =
	{
		.enable       = usartDisable,   // Wait to enable the transmitter and receiver
		.refFreq      = 0,              // Setting refFreq to 0 will invoke the CMU_ClockFreqGet() function and measure the HFPER clock
		.baudrate     = 115200,         // Desired baud rate
		.oversampling = usartOVS16,     // Set oversampling value to x16
		.databits     = usartDatabits8, // 8 data bits
		.parity       = usartNoParity,  // No parity bits
		.stopbits     = usartStopbits1, // 1 stop bit
		.mvdis        = false,          // Use majority voting
		.prsRxEnable  = false,          // Not using PRS input
		.prsRxCh      = usartPrsRxCh0,  // Doesn't matter which channel we select
	};

	USART_InitAsync(UART, &uartInit);

	UART->ROUTE = UART_ROUTE_RXPEN | UART_ROUTE_TXPEN | UART_ROUTE_LOCATION_LOC1;

	USART_IntClear(UART, _UART_IF_MASK); 	// Clear any USART interrupt flags
	NVIC_ClearPendingIRQ(UART1_RX_IRQn);   	// Clear pending RX interrupt flag in NVIC
	NVIC_ClearPendingIRQ(UART1_TX_IRQn);   	// Clear pending TX interrupt flag in NVIC

	USART_Enable(UART, usartEnable);     	// Enable transmitter and receiver
	*/
}