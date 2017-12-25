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

volatile char int_rx_count;

#define RX_BUF_LEN 4096
#define TX_BUF_LEN 4096


volatile unsigned char rxbuffer[RX_BUF_LEN];
volatile unsigned char txbuffer[TX_BUF_LEN];

volatile int tx_head;
volatile int tx_tail;
volatile int tx_to_write;

volatile int rx_head;
volatile int rx_tail;
volatile int rx_to_write;


#define UART_IIR	2	/* In:  Interrupt ID Register */
#define UART_IIR_NO_INT		0x01 /* No interrupts pending */
#define UART_IIR_ID		0x0e /* Mask for the interrupt ID */ // e=1110
#define UART_IIR_MSI		0x00 /* Modem status interrupt */
#define UART_IIR_THRI		0x02 /* Transmitter holding register empty */
#define UART_IIR_RDI		0x04 /* Receiver data interrupt */
#define UART_IIR_RLSI		0x06 /* Receiver line status interrupt */

#define UART_LSR_THRE		0x20 /* Transmit-hold-register empty */
#define UART_LSR_BI		0x10 /* Break interrupt indicator */
#define UART_LSR_FE		0x08 /* Frame error indicator */
#define UART_LSR_PE		0x04 /* Parity error indicator */
#define UART_LSR_OE		0x02 /* Overrun error indicator */
#define UART_LSR_DR		0x01 /* Receiver data ready */
#define UART_LSR_BRK_ERROR_BITS	0x1E /* BI, FE, PE, OE bits */

#define PBASE 0x3F000000

#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)

extern unsigned int GET32 ( unsigned int );

void serial8250_tx_chars(){
	unsigned char status;
	while(1){
		status = GET32(AUX_MU_LSR_REG);
		while ( (status & UART_LSR_THRE) && tx_to_write > 0) {
			PUT32(AUX_MU_IO_REG,txbuffer[tx_tail]);
			tx_to_write--;
			tx_tail = (tx_tail+1) % TX_BUF_LEN;
			status = GET32(AUX_MU_LSR_REG);
		}
		vTaskDelay(500);
	}

}
void my_29_int(int nIRQ, void *pParam){

	unsigned char status;

	unsigned int rb,rc;

	//return;
	rb = GET32(AUX_MU_IIR_REG);
	if(rb & UART_IIR_NO_INT)
		return;



	/*
	while(1){ //resolve all interrupts to uart
		rb=
		if((rb&1)==UART_IIR_NO_INT) break; //no more interrupts
		if((rb&6)==UART_IIR_RDI)
			{
				//receiver holds a valid byte
				rc=GET32(AUX_MU_IO_REG); //read byte from rx fifo
				//rxbuffer[rxhead]=rc&0xFF;
				//rxhead=(rxhead+1)&RXBUFMASK;
				//SetGpio(47, irqset);
				//irqset = 1 - irqset;
				}
	}
	*/

	//this version is taken from the linux kernel code for rpi - /home/dafna/pi/pi-linux/drivers/tty/serial/8250/8250_port.c
	//function serial8250_handle_irq
	/*
	if (iir & UART_IIR_NO_INT)//No interrupt pending
		return 0;

	*/
	status = GET32(AUX_MU_LSR_REG);

	while(status & (UART_LSR_DR | UART_LSR_BI)) {//#define UART_LSR_DR		0x01 // Receiver data ready

		int_rx_count++;

		if(int_rx_count>'Z')
			int_rx_count = 'A';

		rc=GET32(AUX_MU_IO_REG);
		//rxbuffer[rxbuffer_idx] = rc;
		//rxbuffer_idx = (rxbuffer_idx+1) % RX_BUF_LEN;

		rxbuffer[rx_head] = rc;
		rx_head = (rx_head+1) % RX_BUF_LEN;
		rx_to_write++;

		status = GET32(AUX_MU_LSR_REG);

	}

	unsigned int set = 0;
	//tail is always cylclicly lower than head
	while ( (status & UART_LSR_THRE) && tx_to_write > 0) {
		PUT32(AUX_MU_IO_REG,txbuffer[tx_tail]);
		tx_to_write--;
		tx_tail = (tx_tail+1) % TX_BUF_LEN;
		set = 1 - set;
		SetGpio(47, set);

		status = GET32(AUX_MU_LSR_REG);
	}
}


/* if there is space in the buffer, that is to_write<TX_BUF_LEN then this task add sum text to the buffer starting from head*/

#define ABC_LEN 26
void tx_blabla_task() {
	int i = 0;
	char c = 0;
	portTickType dstep = 100;
	unsigned int set = 0;

	char chars[3] = {0,'\r'-'A','\n'-'A'};

	while(1){
		for(int i=0;i < 3 && tx_to_write< TX_BUF_LEN ;i++){
			txbuffer[tx_head] = 'A'+chars[i];
			if(i == 0)
				chars[i] = (chars[i] + 1)% ABC_LEN;

			tx_head = (tx_head+1) % TX_BUF_LEN;
			tx_to_write++;
		}
		for(int i=0;i< 7 && rx_to_write > 0;i++){
			txbuffer[tx_head] = rxbuffer[rx_tail];

			rx_to_write--;
			rx_tail = (rx_tail+1) % TX_BUF_LEN;

			tx_head = (tx_head+1) % TX_BUF_LEN;
			tx_to_write++;
		}
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

void task2() {
	int i = 0;
	while(1) {
		i++;
        //uart_send('2');
        //uart_send(0x0a);
        //uart_send(0x0d);

		vTaskDelay(100);
		SetGpio(47, 0);
		vTaskDelay(100);
	}
}

int main(void) {


    uart_init();
	SetGpioFunction(47, 1);			// RDY led

	initFB();
	SetGpio(47, 1);
	//videotest();

	DisableInterrupts();
	InitInterruptController();

	int_rx_count = 'A';

	RegisterInterrupt(29, my_29_int, NULL);
	EnableInterrupt(29);

	memset(rxbuffer,'X',RX_BUF_LEN);
	memset(rxbuffer,'X',TX_BUF_LEN);


	tx_head = 0;
	tx_tail = 0;
	tx_to_write = 0;

	rx_head = 0;
	rx_tail = 0;
	rx_to_write = 0;


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

	xTaskCreate(task1, "LED_0", 128, NULL, 1, NULL);
	xTaskCreate(tx_blabla_task, "LED_1", 128, NULL, 1, NULL);
	xTaskCreate(serial8250_tx_chars, "LED_1", 128, NULL, 1, NULL);
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
