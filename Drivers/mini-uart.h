#ifndef _MINI_UART_H_
#define _MINI_UART_H_

volatile char int_rx_count;

void serial_writer_task();

int mini_uart_write(const char *buf, int count);

int mini_uart_read(char *buf, int count);

#endif
