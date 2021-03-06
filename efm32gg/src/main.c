#include <string.h>

#include "uart.h"
#include "pwm.h"
#include "crc.h"
#include "watchdog.h"
#include "rov_utilities.h"

void clockSetup(void);
void gpioSetup(void);

int main()
{
	CHIP_Init();

	clockSetup();

	initPwm();
	initUart();
	initWdog();
	initLeTimer();

	gpioSetup();

	volatile uint32_t hz_hfper = CMU_ClockFreqGet(cmuClock_HFPER);
	volatile uint32_t hz_hf = CMU_ClockFreqGet(cmuClock_HF);
	volatile uint32_t br = USART_BaudrateGet(UART);

	unsigned long resetCause = RMU_ResetCauseGet();
	RMU_ResetCauseClear();

	char uart_msg[50] = {0};
	char* uart_msg_ptr = &uart_msg[0];

	if (resetCause & RMU_RSTCAUSE_WDOGRST)
	{
		strcpy(uart_msg_ptr, "Please pet the watchdog, start initialization\n\r");
	}
	else
	{
		strcpy(uart_msg_ptr, "MCU reset normally, start initialization\n\r");
	}

	USART_PutData((uint8_t*)uart_msg_ptr, strlen(uart_msg));

	uint8_t receive_data[VORTEX_MSG_MAX_SIZE] = {0};
	uint8_t *receive_data_ptr = &receive_data[0];
	uint8_t msg_type = MSG_TYPE_NOTYPE;

	start_sequence();

	while (1)
	{
		switch (receive_vortex_msg(receive_data_ptr))
		{
			case MSG_STATE_RECEIVE_OK:
				if (crc_passed(&receive_data[0]) == true)
				{
					msg_type = receive_data[VORTEX_MSG_TYPE_INDEX];
					WDOGn_Feed(WDOG);
				}
				else
				{
					msg_type = MSG_TYPE_NOACK;
					send_vortex_msg(MSG_TYPE_NOACK);
				}
				break;

			case MSG_STATE_RECEIVE_FAIL:
				msg_type = MSG_TYPE_NOACK;
				break;

			default:
				msg_type = MSG_TYPE_NOTYPE;
				break;
		}

		switch (msg_type)
		{
			case MSG_TYPE_THRUSTER:
				GPIO_PinOutToggle(LED1_PORT, LED1_PIN);
				send_vortex_msg(MSG_TYPE_THRUSTER);
				if (update_thruster_pwm(&receive_data[VORTEX_MSG_START_DATA_INDEX]) != PWM_UPDATE_OK)
				{
					strcpy(uart_msg_ptr, "ROV NOT ARMED\n\r");
					USART_PutData((uint8_t*)uart_msg_ptr, strlen(uart_msg));
				}
				break;

			case MSG_TYPE_LIGHT:
				GPIO_PinOutToggle(LED1_PORT, LED1_PIN);
				send_vortex_msg(MSG_TYPE_LIGHT);
				if (update_light_pwm(&receive_data[VORTEX_MSG_START_DATA_INDEX]) != PWM_UPDATE_OK)
				{
					//error handling
				}
				break;

			case MSG_TYPE_ARM:
				arm_sequence();
				break;

			case MSG_TYPE_DISARM:
				disarm_sequence();
				break;

			case MSG_TYPE_HEARTBEAT:
				GPIO_PinOutToggle(LED2_PORT, LED2_PIN);
				send_vortex_msg(MSG_TYPE_HEARTBEAT);
				WDOGn_Feed(WDOG);
				break;

			case MSG_TYPE_NOTYPE:
				send_vortex_msg(MSG_TYPE_NOTYPE);
				break;

			case MSG_TYPE_NOACK:
				break;

			default:
				break;
		} // switch

		msg_type = MSG_TYPE_NOTYPE;
		memset(&receive_data[0], 0, sizeof(receive_data));

	} // while
} // main

void gpioSetup(void)
{
	// set LED1 and LED2 as outputs
	GPIO_PinModeSet(LED1_PORT, LED1_PIN, gpioModePushPullDrive, 0);
	GPIO_PinModeSet(LED2_PORT, LED2_PIN, gpioModePushPullDrive, 1);

	// Leak sensor setup, enable interrupt
	GPIO_IntClear(0x5555);
	GPIO_IntConfig(LEAK_SENSOR_PORT, LEAK_SENSOR_PIN, true, true, true);
	GPIO_PinModeSet(LEAK_SENSOR_PORT, LEAK_SENSOR_PIN, gpioModeInput, 0);
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void clockSetup(void)
{

	// Set clock dividers
	CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_1);
	CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);

	CMU_HFXOInit_TypeDef hfxoInit =
	{
	   _CMU_CTRL_HFXOBOOST_100PCENT,   // 100% HFXO boost
	   _CMU_CTRL_HFXOTIMEOUT_16KCYCLES,// 16k startup delay
	   false,                          // Enable glitch detector
	   cmuOscMode_Crystal,             // Crystal oscillator
	};

	CMU_HFXOInit(&hfxoInit);

	// Select HFXO  as clock source for HFPER
	CMU_ClockSelectSet(cmuClock_HFPER, cmuSelect_HFXO);

	 // Select HFXO  as clock source for HFCORECLOCK
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

	// Enable HFPER (High Frequency Peripheral Clock)
	CMU_ClockEnable(cmuClock_HFPER, true);

	// Disable HFRCO
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

	// Enable clock for USART module
	CMU_ClockEnable(cmuClock_USART1, true);

	// Enable clock for GPIO module
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Enable clock for TIMERn modules
	CMU_ClockEnable(cmuClock_TIMER0, true);
	CMU_ClockEnable(cmuClock_TIMER1, true);
	CMU_ClockEnable(cmuClock_TIMER2, true);
	CMU_ClockEnable(cmuClock_TIMER3, true);

	// Low energy timer used in start/arm/disarm sequence
	CMU_ClockEnable(cmuClock_CORELE, true);
	CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
	CMU_ClockEnable(cmuClock_LETIMER0, true);

	// Watchdog
	CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
	CMU_ClockSelectSet(cmuClock_CORELE, cmuSelect_LFRCO);

}
