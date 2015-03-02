#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/ring_buf.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>//kmalloc
#include <asm/uaccess.h>
#include <linux/uart_driver_x86.h>
#include <linux/pca_tdma_code.h>

#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BHARGAV");
MODULE_DESCRIPTION("TDMA module");


#define MS_TO_NS(x)     (x * 1E6L)
#define FRAME_SIZE 1000

static struct hrtimer hr_timer;
static ktime_t kt_periode;
ktime_t ktime;

spinlock_t b_lock;

char event_queue[10000];
unsigned int evt_head=0,evt_tail=0;
unsigned int twheel_counter=0;
unsigned int rx_bufid,tx_bufid;
uint16 current_slots_index = 0;
extern int Tx_bufid;
extern int Rx_bufid;
int flag,slot;

struct task_struct *tdma_main_thread;

void tdma_sync_frame(void);
int rxstate,txstate;

struct internal_buffer rx_ibuf,tx_ibuf;
struct tdma_slot_entry next_slots[ PCA_TDMA_MAX_SLOTS];
//struct slot current_slots[PCA_TDMA_MAX_SLOTS]={{1000,1},{2000,2},{3000,4},{4000,8},{5000,5},{6000,10},{7000,0}};

#if 0
struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_TX_STOP_RXSTART },
	{2 , TDMA_ACTION_RX_STOP_TXSTART },
	{3 , TDMA_ACTION_TX_STOP_RXSTART },
	{4 , TDMA_ACTION_RX_STOP_TXSTART },
	{5 , TDMA_ACTION_TX_STOP_RXSTART },
	{6 , TDMA_ACTION_RX_STOP_TXSTART },
	{7 , TDMA_ACTION_TX_STOP_RXSTART },
	{8 , TDMA_ACTION_RX_STOP_TXSTART },
	{9 , TDMA_ACTION_TX_STOP_RXSTART },
	{10, TDMA_ACTION_RX_STOP_TXSTART }
};  //only for test - to be initialized by algorithm

#endif
#if 0
struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_RX_STOP_TXSTART },
	{2 , TDMA_ACTION_TX_STOP_RXSTART },
	{3 , TDMA_ACTION_RX_STOP_TXSTART },
	{4 , TDMA_ACTION_TX_STOP_RXSTART },
	{5 , TDMA_ACTION_RX_STOP_TXSTART },
	{6 , TDMA_ACTION_TX_STOP_RXSTART },
	{7 , TDMA_ACTION_RX_STOP_TXSTART },
	{8 , TDMA_ACTION_TX_STOP_RXSTART },
	{9 , TDMA_ACTION_RX_STOP_TXSTART },
	{10, TDMA_ACTION_TX_STOP_RXSTART }
};  //only for test - to be initialized by algorithm


struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_RX_START },
	{2 , TDMA_ACTION_RX_STOP },
	{3 , TDMA_ACTION_TX_START },
	{4 , TDMA_ACTION_TX_STOP },
	{5 , TDMA_ACTION_RX_START },
	{6 , TDMA_ACTION_RX_STOP },
	{7 , TDMA_ACTION_TX_START },
	{8 , TDMA_ACTION_TX_STOP },
	{9 , TDMA_ACTION_RX_START },
	{10, TDMA_ACTION_RX_STOP }
};  //only for test - to be initialized by algorithm
#endif

struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_TX_START },
	{2 , TDMA_ACTION_RX_START },
	{3 , TDMA_ACTION_TX_START },
	{4 , TDMA_ACTION_RX_START },
	{5 , TDMA_ACTION_TX_START },
	{6 , TDMA_ACTION_RX_START },
	{7 , TDMA_ACTION_TX_START },
	{8 , TDMA_ACTION_RX_START },
	{9 , TDMA_ACTION_TX_START },
	{10 , TDMA_ACTION_RX_START },
};  //only for test - to be initialized by algorithm


#if 0
struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_RX_START },
	{2 , TDMA_ACTION_RX_START },
	{3 , TDMA_ACTION_RX_START },
	{4 , TDMA_ACTION_RX_START },
	{5 , TDMA_ACTION_RX_START },
	{6 , TDMA_ACTION_RX_START },
	{7 , TDMA_ACTION_RX_START },
	{8 , TDMA_ACTION_RX_START },
	{9 , TDMA_ACTION_RX_START },
	{10 , TDMA_ACTION_RX_START },
};  //only for test - to be initialized by algorithm

#endif

struct timeval tm;
unsigned long int msec;
enum hrtimer_restart pca_tdma_timer_event_handler(  struct hrtimer *timer )
{
do_gettimeofday(&tm);
msec = (tm.tv_sec%1000)*1000 +(tm.tv_usec/1000);
printk("msec slot %ld\n",msec);
//	printk( "my_hrtimer_callback called (%ld)  msecs  %lu .   usecs  %lu \n", jiffies,jiffies_to_msecs(jiffies),jiffies_to_usecs(jiffies));
	int flag;
	// this function will be called when timer expires.
	if (++twheel_counter == current_slots[current_slots_index].time)
	{
		flag = current_slots[current_slots_index].action;
		//printk("slot number --- %d ------%08x\n",current_slots_index,flag);
		// based on slot index and flag corresponding event will be posted.
		tdma_event_callback(flag);	
	}

               current_slots_index = (current_slots_index +1) % PCA_TDMA_MAX_SLOTS ;
               slot = current_slots_index ;
               if (current_slots_index == 0)
               {
                    twheel_counter = 0;
               }

	hrtimer_forward_now(timer, kt_periode);
	return HRTIMER_RESTART;
}

void tdma_event_callback(char action)
{
int ret;
	switch(action)
	{
		case TDMA_ACTION_RX_START:
//flag =1 ;
			//printk( " action  %d\n",action);
			rxstate = TDMA_ACTION_RX_START; //this should be atomic set ?
//			atomic_set(&rxstate, action);
  			post_event_queue(rxstate);
//printk("waking up the process %08x\n",tdma_main_thread);
	printk(" rx slot %d:  msecs %lu \n",slot,jiffies_to_msecs(jiffies));
			//printk( "  %d rx start action\n",slot);
			atomic_set(&flag , 1);
//flag =1;
	//		ret=wake_up_process(tdma_main_thread);
//printk("wakeup process return value is %d \n",ret);
			break;
		case TDMA_ACTION_RX_STOP:   
	atomic_set(&flag , 0);
//flag =0 ;
			//printk( " action  %d\n",action);
	//		printk( "rx stop action\n");
			//printk( "  %d rx stop action\n",slot);
//printk("rxstop\n");
			rxstate = TDMA_ACTION_RX_STOP;
			//atomic_set(&rxstate, TDMA_ACTION_RX_STOP);
			break;
		case TDMA_ACTION_TX_START:
		atomic_set(&flag , 2);
			//printk( "  %d tx start action\n",slot);
//flag =2 ;
			//printk( " action  %d\n",action);
	//		printk( "tx start action\n");
			txstate = TDMA_ACTION_TX_START; //this should be atomic set ?
	printk( " tx slot %d:  msecs %lu \n",slot,jiffies_to_msecs(jiffies));
			//atomic_set(&txstate, TDMA_ACTION_TX_START);
			post_event_queue(txstate);
//printk("waking up the process %08x\n",tdma_main_thread);
//			ret=wake_up_process(tdma_main_thread);
//printk("wakeup process return value is %d \n",ret);
			break;
		case TDMA_ACTION_TX_STOP:
			atomic_set(&flag , 0);
//flag =0 ;
			//printk( " action  %d\n",action);
	//		printk( "tx stop action\n");
			//atomic_set(&txstate, TDMA_ACTION_TX_STOP);
			txstate = TDMA_ACTION_TX_STOP;
			//printk( "  %d tx stop action\n",slot);
//printk("txstop\n");
			break;
		case TDMA_ACTION_RX_STOP_TXSTART:
	//		printk( " action  %d\n",action);
	//		printk( "rxstop tx start action\n");
			rxstate = TDMA_ACTION_RX_STOP;
//printk("rxstop\n");
			txstate = TDMA_ACTION_TX_START; //this should be atomic set ?
			//atomic_set(&rxstate, TDMA_ACTION_RX_STOP);
			//atomic_set(&txstate, TDMA_ACTION_TX_START);
			post_event_queue(txstate);
//printk("waking up the process %08x\n",tdma_main_thread);
//printk("callback :tdma_main_thread %08x\n",tdma_main_thread);
//			ret=wake_up_process(tdma_main_thread);
//printk("wakeup process return value is %d \n",ret);
			break;
		case TDMA_ACTION_TX_STOP_RXSTART:
	//		printk( " action  %d\n",action);
	//		printk( "txstop rx start action\n");
			txstate = TDMA_ACTION_TX_STOP;
//printk("txstop\n");
			rxstate = TDMA_ACTION_RX_START; //this should be atomic set ?
			//atomic_set(&txstate, TDMA_ACTION_TX_STOP);
			//atomic_set(&rxstate, TDMA_ACTION_RX_START);
			post_event_queue(rxstate);
//printk("waking up the process %08x\n",tdma_main_thread);
			wake_up_process(tdma_main_thread);
			break;
	        default: break;

	}
	return;
}

int post_event_queue(int action)
{
	spin_lock(&b_lock);
	if(evt_tail == 9999)
		evt_tail = 0;
	event_queue[evt_tail] = action;
	evt_tail++;
	spin_unlock(&b_lock);
	//printk("posted event %08x\n",action);
	return SUCCESS;
}

int read_event_queue(void)
{
	//printk("read event called \n");
	int ret;
	spin_lock(&b_lock);
	/***************lock**********************/
	ret = event_queue[evt_head];
	evt_head++;
	if(evt_head == 9999)
		evt_head = 0;
	/***************lock**********************/
	spin_unlock(&b_lock);
//	printk("read event %08x\n",ret);
	return ret;
	//delete the event to be returned 
	//returns the event 
}

void pca_tdma_main_loop(void ) // this is the thread function..
{
unsigned int c;
char time_string[8],ch;
	int len=0;
	unsigned char buf[1000],i;
	int evt,ret,cnt=0,slot_cnt=0;
unsigned long int time;
while(1)
{

len = read_nowait(&ch,1);
	if(len >0)
	{
	time_string[0]=ch;
	for(i=1;i<8;i++)
		{
		len = read_nowait(&ch,1);
			if(len > 0)
			{		
			time_string[i]=ch;		
			}
		}
	printk("recieved time stamp = %ld \n",time);
//send present time stamp;
	break;
	}

//send present time stamp;
msleep(1);
}


	while(!kthread_should_stop())
	{

	//printk("before sleeping\n");
if(kthread_should_stop()) return 0;	
		set_current_state(TASK_INTERRUPTIBLE);
	//	schedule();  // waiting here ....
	//printk("after wake up\n");
//if(kthread_should_stop()) return 0;	
//evt_jump:
	//	evt = read_event_queue();
////printk("evt in main loop %d\n",evt);
	//	switch(evt)
	//	{
	//		case  TDMA_ACTION_RX_START:
	//			while(txstate !=  TDMA_ACTION_TX_STOP )
//				while(rxstate ==  TDMA_ACTION_RX_START )
slot_cnt = 0;
		while(flag==1)
				{
			//	printk(" inside Rxstart rxstate= %d  evt = %d case = %d \n",rxstate, evt,TDMA_ACTION_RX_START );
				if(kthread_should_stop()) return 0;	
//						 printk("no data recvd\n");
				//	pca_tdma_recv(buf,len);
				//	pca_tdma_internal_post(RX,buf,len);

				len = read_nowait(buf,100);
			if(len>0)
				{
cnt= cnt+len;	        
slot_cnt= slot_cnt+len;	        
	//		printk("%ld: recvd %d \n",jiffies_to_msecs(jiffies),len);
				
//				post_data(Rx_bufid,buf,len);
				//for(c=0;c<len;c++)  printk("%c",buf[c]);
//				ret = bytes_used(Rx_bufid);
//			        printk("bytes used %d \n",ret);
				}
			//handle if internal buffer reaches threshold value - if so move data to ringbuf
				}
				//rxstate=-1;
				//break;
//printk("in %d slot %d bytes total rcvd %d \n ",slot,slot_cnt,cnt);
printk(" in this slot %d bytes total rcvd %d \n ",slot_cnt,cnt);
	//		case  TDMA_ACTION_TX_START:
		//		while(rxstate != TDMA_ACTION_RX_STOP )
while(flag==2)
//		while(txstate == TDMA_ACTION_TX_START)
				{
				//printk(" inside Txstart txstate= %d  evt = %d case =%d \n",txstate, evt ,TDMA_ACTION_TX_START );
				//printk(" inside Txstart \n" );
				if(kthread_should_stop()) return 0;
		//		ret=pca_tdma_get(Tx_bufid,buf,100);
	//			ret=get_data(Tx_bufid,100,buf);
	//			pull(Tx_bufid,ret);
	//					 printk("bytes used %d \n",ret);
	//		if(ret<=0)
	//					{
					//	 printk("no data in ring buf\n");
	//					}
	//				else
	//					{
					//	pca_tdma_send(buf,len);
	//						//printk("send return value %d \n",ret);
	//						if(ret>0)
	//						{
	//						ret = serial_send(buf,ret);
	//						 printk("bytes sent %d \n",ret);
	//						}
	//					}
					//pca_tdma_internal_get(TX,buf,len);
					//pca_tdma_internal_pull(TX,len); //compile
					//note: guard_time,
				}
				//txstate=-1;
				//break;
	//	}
//printk("while loop\n");
#if 0
	//while(internal rx buffer not empty)
		while(rx_ibuf.free!=10000)
		{ 
			pca_tdma_internal_get(RX,buf,len);
			pca_tdma_post(rx_bufid,buf,len);
			//post to RX_ring buf from internal buffer.
		}		
		while(tx_ibuf.used!=10000)
		{
			if(kthread_should_stop()) return 0;
			pca_tdma_get(tx_bufid,buf,len);
			pca_tdma_pull(tx_bufid,len);
			pca_tdma_internal_post(TX,buf,len);
			// fill the local tx buffer.
		}
#endif
msleep_interruptible(5);
	}
return 0;
}

void pca_tdma_create_timer(void)
{
	//this function will create timer 
	//register callback function.
	return;
}


int pca_tdma_set_timer(unsigned long interval)
{

	ktime_t ktime;
	unsigned long delay_in_ms = 100L;

	//kt_periode = ktime_set(0, 204167); //seconds,nanoseconds
	kt_periode = ktime_set(0, 100000000); //seconds,nanoseconds
	//kt_periode = ktime_set(1, 0); //seconds,nanoseconds
	printk("HR Timer module installing\n");

	ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );

	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	hr_timer.function = &    pca_tdma_timer_event_handler;

	//this function will set timer.
	//return success or fail
	return  SUCCESS;
}

void pca_tdma_start_timer(void )
{
	//printk( "Starting timer to fire in %ldms (%ld)\n", delay_in_ms, jiffies );
	printk( "Starting timer to fire in ms \n");
	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
	//this function will start timer.
	return;
}

int pca_tdma_recv(unsigned char *buf,int len)
{
	int n=0;
	while (1)
	{
		//bzero(recvline,SIZE);
		//		n = read(buf,len);  //compile
		if (n < 0) {
			//			if (errno == EINTR) { 
			printk(KERN_INFO "EINTR-read \n");
			//				errno =0; continue; }
			return n;
		}
	}       

}

int pca_tdma_post(char bufid, unsigned char *buf,int len)
{
	int ret;
	ret = post_data(bufid,buf,len);
	return ret;
}

int pca_tdma_get(char bufid, unsigned char *buf,int len)
{
	int ret;
	ret = get_data(bufid, len ,buf);
	return ret;
}

int pca_tdma_pull(char bufid,int len)
{
	int ret;
	ret = pull(bufid, len );
	return ret;
}

int pca_tdma_send(unsigned char *buf,int len)
{
	int ret=0;
	static unsigned int bcount=0;
	while(1)
	{
			ret = serial_send(buf,len);//compile
		if (ret == -1) {
			//	if (errno == EINTR) { errno = 0; continue; }
			//	printk(" write Error: %s (%d)\n", strerror(errno), errno);
			return ret;
		}
		bcount+=ret;
		//	printk("%s:Wrote-%p: %d bytes. [total %d]\n",__func__,sendline,ret,bcount);
		return ret;
	}       

	return FAIL;
}


static int __init pca_tdma_init(void)
{
	int interval,ret,len=100;
char buf[1000];
	// initialize the timer and ring buffer
	//pca_tdma_create_timer();
//	Tx_bufid = ring_buf_init();
//	Rx_bufid = ring_buf_init();
	//print_rbuf(Tx_bufid);
//ret=get_data(Tx_bufid,buf,len);
//printk("ret = %d\n",ret);
	pca_tdma_set_timer(interval);
	spin_lock_init(&b_lock);
	//init internal buffers..
	// create a thread and save id into global variable.
//printk("init :tdma_main_thread %08x\n",tdma_main_thread);
//	time_sync();
//time_to_tm(tv.tv_sec,0,&broken);
//printk("time in seconds msec = %ld   sec= %ld   %d \n",msec,tv.tv_sec,tv.tv_sec%100);
//printk("%d:%d:%d:%ld\n",broken.tm_hour,broken.tm_min,broken.tm_sec,tv.tv_usec);
//while(jiffies_to_msecs(jiffies)%1000);
//msleep(43);
//while(jiffies_to_msecs(jiffies)%11);

	tdma_main_thread = kthread_run(&pca_tdma_main_loop,NULL,"main_loop");
tdma_sync_frame();
	pca_tdma_start_timer();
	return 0 ;
}

void tdma_sync_frame(void)
{
struct timeval tv;
struct tm broken;
unsigned long int msec;

do_gettimeofday(&tv);
//msec = (tv.tv_sec % 100)*1000000+tv.tv_usec;
msec = (tv.tv_sec %1000 )*1000+((tv.tv_usec)/1000);
printk("msec = %ld \n",msec);
//while(msec%1000);
//msleep_interruptible(3);
msleep(1000-(msec%1000));
printk("sleep is  %d \n",1000-(msec%1000));
#if 0
        if(msec%FRAME_SIZE)
               // msleep((FRAME_SIZE-(msec%FRAME_SIZE))*1000);
                msleep((FRAME_SIZE-(msec%FRAME_SIZE)));
#endif     
   return 0;
}


int pca_tdma_internal_post(int bufid,unsigned char *buf,int len)
{
	if(bufid == RX)
	{
		if(rx_ibuf.free >= len)
		{
			if(rx_ibuf.tail+len <= 10000) 
			{
				memcpy(rx_ibuf.buf[rx_ibuf.tail],buf,len);
				rx_ibuf.tail += len;
				return len;	
			}
			else if((rx_ibuf.tail+len > 10000) && ( rx_ibuf.head > rx_ibuf.tail))
			{
				memcpy(rx_ibuf.buf + rx_ibuf.tail,buf,10000-rx_ibuf.tail);
				memcpy(rx_ibuf.buf,buf+(10000-rx_ibuf.tail),len-(10000-rx_ibuf.tail));

				rx_ibuf.tail = len-(10000-rx_ibuf.tail);			
				return len;
			}
		}
		else
		{
			printk("not enough memory \n");

		}
	}
	return FAIL;
}

int pca_tdma_internal_get(int bufid,unsigned char *buf,int len)
{
	int ret_len=0;
	if(bufid == RX)
	{
		if(rx_ibuf.used == 0) 
		{
			printk(KERN_INFO "no bytes in internal buffer\n");
			return -1; // INT_BUF_EMPTY;
		}

		if(len > rx_ibuf.used)
			ret_len = rx_ibuf.used;

		else if(len < rx_ibuf.used)
			ret_len = len;

		if(rx_ibuf.head < rx_ibuf.tail)
		{
			memcpy(buf,rx_ibuf.buf+rx_ibuf.head,ret_len);
			rx_ibuf.head=rx_ibuf.head+ret_len;
			return ret_len;
		}
		else if(rx_ibuf.head > rx_ibuf.tail)
		{
			memcpy(buf,rx_ibuf.buf+rx_ibuf.head,10000-rx_ibuf.head);
			memcpy(buf+(10000-rx_ibuf.head),rx_ibuf.buf,(ret_len-10000-rx_ibuf.head));
			rx_ibuf.head=ret_len-10000-rx_ibuf.head;;
			return ret_len;
		}

	}
	return FAIL;
}



static void __exit pca_tdma_exit(void)
{
	int ret;
ret = hrtimer_cancel( &hr_timer );
		kthread_stop(tdma_main_thread);
	if (ret) printk("The timer was still in use...\n");
	printk("HR Timer module uninstalling\n");
	printk(KERN_INFO "exiting pca_tdma_module\n");

}


//EXPORT_SYMBOL(Tx_bufid);
//EXPORT_SYMBOL(Rx_bufid);

module_init(pca_tdma_init);
module_exit(pca_tdma_exit);


