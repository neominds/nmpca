#include<linux/module.h>
#include<linux/types.h>
#include<asm/io.h>
#include<linux/delay.h>
#include<linux/jiffies.h>
#include<linux/serial_reg.h>
#include<linux/fs.h>
#include<linux/kdev_t.h>
#include<linux/timer.h>
#include<linux/uart_driver_x86.h>

#define SERIAL_PORT 0x3f8 //base address
#define NO_OF_SECONDS_TO_WAIT 5
#define BUFF_SIZE 16
#define NO_DATA -1

int i=0;
int tcheck=0;
int count=0;
volatile int k=0;
char c,recev=0;
char data;
char ubuf[BUFF_SIZE];
char tmpbuf[50];// Debub
//long int static size_in=0;
struct timer_list timer1;
void initialization(void)
{
	
	outb(0x00,SERIAL_PORT+1);
	outb(0x80,SERIAL_PORT+3);
	outb(0x01,SERIAL_PORT+0);//Baud rate Regs replace 3->c for 9600 and 3->1 for 115200
	outb(0x00,SERIAL_PORT+1);
	outb(0x03,SERIAL_PORT+3);
	outb(0xC7,SERIAL_PORT+2);//enable fifo,clear them
	outb(0x0B,SERIAL_PORT+4);
}
/*
Clear the port if any data present in the buffer
*/
void clear_port()
{
	int cl=0;
        while(serial_available()==1){
	tmpbuf[cl]=inb(SERIAL_PORT);cl++;}
}

int is_transmit_empty(void)
{
//DEBUG :	printk(KERN_INFO "value of lsr reg is %d \n",inb(SERIAL_PORT +5 )&0x20);
	return inb(SERIAL_PORT + 5) & 0x20;
}

unsigned int serial_send(unsigned char *trans_buf, unsigned int len)
{

	unsigned int size=0;
	while(size!=len)
	{
		while(is_transmit_empty()==0);
		outb(*trans_buf,SERIAL_PORT);
		trans_buf++;	
		size++;	
	}
	return size;
}

unsigned char serial_available(void)
{
	return (inb(SERIAL_PORT + 5)) & 0x01;
}

int read(unsigned char *read_buf, unsigned int len)
{
	init_timer(&timer1);
	int l;
	timer1.function=func_time;
	timer1.expires=jiffies + NO_OF_SECONDS_TO_WAIT*HZ; 
	add_timer(&timer1);
	printk("timer started\n ");
	unsigned int size=0;
	while(size != len) {
		while(serial_available() == 0 && k==0);
		if(k==10) break;
		read_buf[size]=inb(SERIAL_PORT);
		size++;
		mod_timer(&timer1,jiffies + NO_OF_SECONDS_TO_WAIT*HZ);
	}

// DEBUG: printk(KERN_INFO "Read complete from serial port:\n ");
	//for(l=0;ubuf[l]!='\0';l++){ printk("%c ",ubuf[l]);}
	del_timer_sync(&timer1);
	return size;
}
int read_nowait(unsigned char *read_buf,unsigned int len)
{	
	int l=0,i=0;
	int ch=0;
	int size_in=0;
	if((inb(SERIAL_PORT + 5))& 0x02)
	{
		printk("%lu: Over Run Error::Fifo was Not cleared\n",jiffies_to_msecs(jiffies));
	//	return -1;
	}
	//while (serial_available() == 0);
	//printk("byte recvd at serial port -> %02X \n",inb(SERIAL_PORT));
	if(serial_available() == 0)
	{
		return NO_DATA;
	}
	else
	{
		while( serial_available() != 0 && size_in < len )
		{
			read_buf[size_in]=inb(SERIAL_PORT);
		//	printk("%c rcvd \n",read_buf[size_in]);
			size_in++;
		}
			//printk("%c rcvd \n",read_buf[0]);
		return size_in;
	}
}

void func_time(void)
{
	tcheck=10;//terminate while loop when timer expires
	k=10;
	printk("Inside func_time: k-%d\n",k);
}

int init_module(void)
{
	
	long int t=0;
	long int l;
	int ret_value=0;
	printk("\n-------------------Uart_Driver_Inserted--------------------------------\n");
//	memset(ubuf,0,BUFF_SIZE);
	initialization();
	clear_port();
	k=0;
	return 0;
}

void cleanup_module(void)
{
	printk("\n-------------------Uart_Driver_Removed--------------------------\n");
	clear_port();
//	int cl=0;
  //      while(serial_received()==1){
//	tmpbuf[cl]=inb(SERIAL_PORT);cl++;}
	//printk("cl value is %d \n",cl);
	//for(;cl!=0;cl--){ printk("%c ",tmpbuf[cl]);}
	
//	inb(SERIAL_PORT);
//	rbr=inb(SERIAL_PORT);
}

EXPORT_SYMBOL(tcheck);
EXPORT_SYMBOL(timer1);
EXPORT_SYMBOL(initialization);
EXPORT_SYMBOL(is_transmit_empty);
EXPORT_SYMBOL(serial_send);
EXPORT_SYMBOL(serial_available);
EXPORT_SYMBOL(read);
EXPORT_SYMBOL(read_nowait);
EXPORT_SYMBOL(func_time);
EXPORT_SYMBOL(init_module);
EXPORT_SYMBOL(cleanup_module);

