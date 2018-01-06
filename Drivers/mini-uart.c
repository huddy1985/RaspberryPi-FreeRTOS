#include <stddef.h>
#include "mini-uart.h"

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

extern void PUT32 ( unsigned int, unsigned int );
extern void PUT16 ( unsigned int, unsigned int );
extern void PUT8 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern void dummy ( unsigned int );

#define ARM_TIMER_CTL   (PBASE+0x0000B408)
#define ARM_TIMER_CNT   (PBASE+0x0000B420)

#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

//in linux those are defined in serial_reg.h

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)



/* serial8250_tx_chars, serial8250_rx_chars:
 * these two functions are based on the same functions in the linux kernel code in drivers/tty/serial/8250/8250_port.c
 *
 */

//before calling this function the caller should first check if the condition (tx_to_write > 0 &&  (GET32(AUX_MU_LSR_REG) & UART_LSR_THRE)) is true
void serial8250_tx_chars(){
	do{
		PUT32(AUX_MU_IO_REG,txbuffer[tx_tail]);
		tx_to_write--;
		tx_tail = (tx_tail+1) % TX_BUF_LEN;
	}	while ( (GET32(AUX_MU_LSR_REG) & UART_LSR_THRE) && tx_to_write > 0);
}

//before calling this function the caller should first check if the condition status & (UART_LSR_DR | UART_LSR_BI) is true
void serial8250_rx_chars(){
	do{
		rxbuffer[rx_head] = GET32(AUX_MU_IO_REG);
		rx_head = (rx_head+1) % RX_BUF_LEN;
		rx_to_write++;
	}while(GET32(AUX_MU_LSR_REG) & (UART_LSR_DR | UART_LSR_BI));
}


void serial_writer_task(void* write_delay){
	while(1){
		if(tx_to_write > 0 &&  (GET32(AUX_MU_LSR_REG) & UART_LSR_THRE))
			serial8250_tx_chars();
		vTaskDelay((long)write_delay);
	}
}

int x = 0;
#define ABC_LEN 26
char chars[4] = {'A','a','\r','\n'};
void my_29_int(int nIRQ, void *pParam){

	unsigned char status;
	unsigned int rb,rc;

    //hexstring(0x87654321);
	//return;
	rb = GET32(AUX_MU_IIR_REG);
	if(rb & UART_IIR_NO_INT)
		return;

    //hexstring(0x87654321);
	//this counter is just for debugging
	int_rx_count++;
	if(int_rx_count>'Z')
		int_rx_count = 'A';

    //SetGpio(47, x);
    x = 1 - x;

	status = GET32(AUX_MU_LSR_REG);

	if(status & (UART_LSR_DR | UART_LSR_BI)) {//#define UART_LSR_DR		0x01 // Receiver data ready
		serial8250_rx_chars();


        //mini_uart_write(chars,4);
        //chars[0] = 'A' + ((chars[0]-'A' + 1)% ABC_LEN);
	}

	if( tx_to_write > 0 && (status & UART_LSR_THRE)) {
		serial8250_tx_chars();
	}
}

void uart_init ( void )
{

  int_rx_count = 'A';
  tx_head = 0;
  tx_tail = 0;
  tx_to_write = 0;

  rx_head = 0;
  rx_tail = 0;
  rx_to_write = 0;

  PUT32(AUX_ENABLES,1);

    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);//set data word length to 8
    PUT32(AUX_MU_MCR_REG,0);
    GET32(AUX_MU_LSR_REG);
    //bit 0: receive interrupt,
    //Bits 3:2 are marked as don't care, but are actually required in order to receive interrupts.
    PUT32(AUX_MU_IER_REG,0x05);//enanble rx interupts 0101

    PUT32(AUX_MU_IIR_REG,0xC6);//1100 0110
    PUT32(AUX_MU_BAUD_REG,270);

    GET32(AUX_MU_IO_REG);

    SetGpioFunction(14,0);
    SetGpioFunction(14,2);//alt5
    SetGpioConf(14,0);//disable pull-up/down

    SetGpioFunction(15,0);
    SetGpioFunction(15,2);//alt5
    SetGpioConf(15,2);// enable pull-up


    unsigned int status, tmout = 10000;
#define UART_LSR_THRE 0x20
    for (;;) {
		status = GET32(AUX_MU_LSR_REG);

		if ((status & UART_LSR_THRE) == UART_LSR_THRE)
			break;
		if (--tmout == 0)
			break;
        dummy(1);
        dummy(2);

		//touch_nmi_watchdog();
	}

    PUT32(AUX_MU_CNTL_REG,3);//enable tx and rx

    //DisableInterrupts();
	//InitInterruptController();


    RegisterInterrupt(29, my_29_int, NULL);
	EnableInterrupt(29);
}

int mini_uart_write(const char *buf, int count){

	int i =0;
	for(i=0;i < count && tx_to_write< TX_BUF_LEN ;i++){
		txbuffer[tx_head] = buf[i];
		tx_head = (tx_head+1) % TX_BUF_LEN;
		tx_to_write++;
	}
	return i;
}

int mini_uart_read(char *buf, int count){

	int i=0;
	for(i=0; i < count && rx_to_write > 0; i++){
		buf[i] = rxbuffer[rx_tail];
		rx_tail = (rx_tail+1) % TX_BUF_LEN;
		rx_to_write--;
	}
	return i;
}

//------------------------------------------------------------------------
void uart_putc ( unsigned int c )
{
    while(1)
        {
            if(GET32(AUX_MU_LSR_REG)&0x20) break;
        }
    PUT32(AUX_MU_IO_REG,c);
}
//------------------------------------------------------------------------
void hexstrings ( unsigned int d )
{
    //unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb=32;
    while(1)
        {
            rb-=4;
            rc=(d>>rb)&0xF;
            if(rc>9) rc+=0x37; else rc+=0x30;
            uart_putc(rc);
            if(rb==0) break;
        }
    uart_putc(0x20);
}
//------------------------------------------------------------------------
void hexstring ( unsigned int d )
{
    hexstrings(d);
    uart_putc(0x0D);
    uart_putc(0x0A);
}
