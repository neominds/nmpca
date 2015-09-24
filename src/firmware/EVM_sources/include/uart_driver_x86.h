
#ifndef UART_DRIVER_X86_H
#define UART_DRIVER_X86_H

void initialization(void);
int is_transmit_empty(void);
//char serial_send(unsigned char *trans_buf, unsigned int len);
unsigned int serial_send(unsigned char *trans_buf, unsigned int len);
unsigned char serial_available(void);
int serial_received(void);
int read(unsigned char *read_buf, unsigned int len);
void func_time(void);
int init_module(void);
void cleanup_module(void);
int read_nowait(unsigned char *read_buf,unsigned int len);
#endif
