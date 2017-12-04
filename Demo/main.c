//main.c
//authored by Jared Hull
//
//tasks 1 and 2 blink the ACT LED
//main initialises the devices and IP tasks

#include <FreeRTOS.h>
#include <task.h>

#include "interrupts.h"
#include "gpio.h"
#include "video.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"


//Only for debug, normally should not 
//   include private header
#include "FreeRTOS_IP_Private.h"


typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef long int32_t;

void task1() {
	int i = 0;
	while(1) {
		i++;
        uart_send('1');
        uart_send(0x0a);
        uart_send(0x0d);

		SetGpio(47, 1);
		vTaskDelay(200);
	}
}

void task2() {
	int i = 0;
	while(1) {
		i++;
        uart_send('2');
        uart_send(0x0a);
        uart_send(0x0d);

		vTaskDelay(100);
		SetGpio(47, 0);
		vTaskDelay(100);
	}
}

int main(void) {


    uart_init();
    hexstring(0x12345678);
	SetGpioFunction(47, 1);			// RDY led

	initFB();
	SetGpio(47, 1);
	//videotest();

	DisableInterrupts();
	InitInterruptController();

	//ensure the IP and gateway match the router settings!
	//const unsigned char ucIPAddress[ 4 ] = {192, 168, 1, 42};
	const unsigned char ucIPAddress[ 4 ] = {192, 168, 1, 9};
	const unsigned char ucNetMask[ 4 ] = {255, 255, 255, 0};
	const unsigned char ucGatewayAddress[ 4 ] = {192, 168, 1, 1};
	const unsigned char ucDNSServerAddress[ 4 ] = {8, 8, 8, 8};
	//const unsigned char ucMACAddress[ 6 ] = {0xB8, 0x27, 0xEB, 0x19, 0xAD, 0xA7};
	const unsigned char ucMACAddress[ 6 ] = {0xB8, 0x27, 0xEB, 0xa5, 0x35, 0xC1};
	FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

	//xTaskCreate(serverTask, "server", 128, NULL, 0, NULL);
	//xTaskCreate(serverListenTask, "server", 128, NULL, 0, NULL);

	xTaskCreate(task1, "LED_0", 128, NULL, 0, NULL);
	xTaskCreate(task2, "LED_1", 128, NULL, 0, NULL);

	//set to 0 for no debug, 1 for debug, or 2 for GCC instrumentation (if enabled in config)
	loaded = 1;

	vTaskStartScheduler();

	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while(1) {
		;
	}
}
