
#ifndef UART_DRIVER_X86_H
#define UART_DRIVER_X86_H

void initialization(void);
int is_transmit_empty(void);
char serial_send(unsigned char *trans_buf, unsigned int len);
int serial_received(void);
char read(void);
void func_time(void);
int init_module(void);
void cleanup_module(void);
#endif
