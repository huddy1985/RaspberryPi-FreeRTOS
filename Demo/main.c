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
#include "mini-uart.h"


//Only for debug, normally should not 
//   include private header
#include "FreeRTOS_IP_Private.h"

/* if there is space in the buffer, that is to_write<TX_BUF_LEN then this task add sum text to the buffer starting from head*/

#define ABC_LEN 26
void tx_blabla_task() {

	char chars[4] = {'A','a','\r','\n'};

	while(1){
		mini_uart_write(chars,4);
		chars[0] = 'A' + ((chars[0]-'A' + 1)% ABC_LEN);
		mini_uart_read(chars+1,1);
		vTaskDelay(300);
	}
}


void task1() {
	int i = 0;
	char c = 'A';
	portTickType dstep = 100;
	unsigned int set = 0;

	while(1) {

		if(int_rx_count >= 'A' && int_rx_count < 'H'){
			dstep = 50;
		}
		else if(int_rx_count >= 'H' && int_rx_count < 'N'){
			dstep = 100;
		}
		else if(int_rx_count >= 'N' && int_rx_count < 'U'){
			dstep = 200;
		}
		else{
			dstep = 400;
		}

		set = 1 - set;
		SetGpio(47, set);

		vTaskDelay(dstep);
	}
}

//extern volatile mytask * volatile pxCurrentTCB;
//extern PRIVILEGED_DATA tskTCB * volatile pxCurrentTCB;


int main(void) {

	//the green led is easily writble only on pi2,
	//see https://raspberrypi.stackexchange.com/questions/67431/cant-turn-on-act-led-on-baremetal-pi3

	SetGpioFunction(47, 1);			// RDY led
	SetGpio(47, 0);

	DisableInterrupts();
	InitInterruptController();

	uart_init();

	//xTaskCreate(task1, "LED_0", 128, NULL, 1, NULL);
	//xTaskCreate(tx_blabla_task, "LED_1", 128, NULL, 1, NULL);
	//xTaskCreate(serial_writer_task, "LED_1", 128,(void*)write_delay , 1, NULL);

	long* top  = xInitPxCurrentTCB();

	hexstring(top);//write the address top to uart

	//vTaskStartScheduler();
	EnableInterrupts();


	/*
	__asm volatile("mrs 	r0,cpsr");		// Read in the cpsr register.
	__asm volatile("bic		r0,r0,#0x80");	// Clear bit 8, (0x80) -- Causes IRQs to be enabled
	__asm volatile("msr		cpsr_c, r0");	// Write it back to the CPSR register
	*/




	__asm volatile("nop");
	__asm volatile ("push {r0}");
	__asm volatile ("mov r0, #31");
	__asm volatile ("msr spsr_cxsf, r0");
	__asm volatile ("pop {r0}");


	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	volatile unsigned int i = 0;

	while(1){
		for(i=0;i<5000000;i++)
			;
		SetGpio(47,0);
		//hexstring (0x12345678);
		for(i=0;i<5000000;i++)
			;
		SetGpio(47,1);
		//hexstring (0xabcdef12);

	}

	while(1) {
		;
	}
}
