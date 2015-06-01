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
//unsigned int rx_bufid,tx_bufid;
uint16 current_slots_index = 0;
extern int Tx_bufid;
extern int Rx_bufid;
int flag,slot;

struct task_struct *tdma_main_thread;
int rxstate,txstate;
extern struct task_struct *test_task;
void tdma_sync_frame(void);
struct internal_buffer rx_ibuf,tx_ibuf;
struct tdma_slot_entry next_slots[ PCA_TDMA_MAX_SLOTS];
unsigned long int get_ms(void);
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

struct timeval tm;
unsigned long int msec;
unsigned int syn_count=0;
int syn_flag=0;
enum hrtimer_restart pca_tdma_timer_event_handler(  struct hrtimer *timer )
{

	int flag1;
	syn_count++;
        
	if(syn_count>=1000||syn_flag<3)
	{
	        atomic_set(&flag,30);
		wake_up_process(tdma_main_thread);
		syn_count=0;
                syn_flag++;
                return HRTIMER_NORESTART;
         }
	else 
        {
                
		if (++twheel_counter == current_slots[current_slots_index].time)
		{
			flag1 = current_slots[current_slots_index].action;
			tdma_event_callback(flag1);	
		}

		current_slots_index = (current_slots_index +1) % PCA_TDMA_MAX_SLOTS ;
		slot = current_slots_index; 
		if (current_slots_index == 0)
		{
			twheel_counter = 0;
		}

		hrtimer_forward_now(timer, kt_periode);
		return HRTIMER_RESTART;
	}
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
	//printk( "rx slot %d -  %lu  \n",slot,jiffies_to_msecs(jiffies));
//			atomic_set(&rxstate, action);
  			post_event_queue(rxstate);
//printk("waking up the process %08x\n",tdma_main_thread);
//			printk( "rx start action %d \n",slot);
                        
			atomic_set(&flag , 1);
                        wake_up_process(tdma_main_thread);
//flag =1;
	//		ret=wake_up_process(tdma_main_thread);
//printk("wakeup process return value is %d \n",ret);
			break;
		case TDMA_ACTION_RX_STOP:   
	atomic_set(&flag , 0);
//flag =0 ;
			//printk( " action  %d\n",action);
	//		printk( "rx stop action\n");
	//printk( "rx stop action %d \n",slot);
//printk("rxstop\n");
			rxstate = TDMA_ACTION_RX_STOP;
			//atomic_set(&rxstate, TDMA_ACTION_RX_STOP);
			break;
		case TDMA_ACTION_TX_START:
                      
             		atomic_set(&flag , 2);
//flag =2 ;              
                        wake_up_process(tdma_main_thread);
			//printk( " action  %d\n",action);
	//		printk( "tx start action\n");
			txstate = TDMA_ACTION_TX_START; //this should be atomic set ?
	//printk( "tx slot %d -  %lu \n",slot,jiffies_to_msecs(jiffies));
//			printk( "tx start action %d \n",slot);
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
//			printk( "tx stop %d \n",slot);
			txstate = TDMA_ACTION_TX_STOP;
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
int cnt;
int tot=0;
struct timeval tm;

unsigned long get_ms(void)
{
do_gettimeofday(&tm);
return( tm.tv_sec*1000000+(tm.tv_usec));
}





void pca_tdma_main_loop(void ) // this is the thread function..
{
	int len=0,i;
	unsigned char buf[1000];
	int evt,ret=0,slot_cnt=0;
	int z;
	int tflag=0;
	int tiflag=0;
	int tflag1=0;
	int tiflag1=0;
        int sflag=0;
	int num=0;
	unsigned long int prev=0;
	char ch;
	unsigned long int time;
	int slot_tot=0;
	int slot_num=0;
	unsigned long int previous=0;
	unsigned long int interval=0; 
	unsigned long int inter=0; 
        unsigned long int sync_time;
        unsigned long int sync_time1;
	ch=100;
	serial_send(&ch,1);
	ch=0;
	while(!kthread_should_stop())
	{

		if(kthread_should_stop()) return 0;	
		ret = read_nowait(&ch,1);
		if(ret>0)
		{
			if(ch==100)
			{
				printk("recieved 100 sending 101 \n"); 
				ch=101;
				previous=get_ms();
				serial_send(&ch,1);
				ch=1;

			}
			else if(ch==101)
			{ 
				printk("recieved 101 sending 102 \n");
				ch=102;

				serial_send(&ch,1);
				ch=1; 
				break;
			}
			else if(ch==102)
			{   
				interval=get_ms()-previous;
				printk("recieved 102\n");
				break;
			}
		}	

		msleep_interruptible(5);
	}


	mdelay(20-interval/2000);
	printk("interval %lu\n",interval/2);
	interval=0;
	previous=0;

	pca_tdma_start_timer();
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        while(!kthread_should_stop())
	{

		tiflag=0;
		tiflag1=0;	
		num=0;
                set_current_state(TASK_INTERRUPTIBLE);
		while(flag==1)
		{
		        if(kthread_should_stop()) return 0;	
			num++;
			if(tflag!=0&&tiflag==0)
			{
				tiflag=1;
				interval =get_ms()-previous;
				previous=get_ms();
				if(slot_tot!=0)
				//	printk("slot no: %d interval: %lu total bytes: %d slot bytes: %d\n",slot_num,interval,tot,slot_tot);
				slot_num++;

				slot_tot=0;
			}
			if(tflag==0)
			{
				tflag=1;
				tiflag=1;
				slot_num++;
				printk("HELLO RX\n");
				previous=get_ms();
			}
		
			len =read_nowait(buf,1);
			if(len>0)
			{ 
				tot+=len;
				slot_tot+=len;
				post_data(Rx_bufid,buf,len);
				wake_up_process(test_task);
                                
                        }
			slot_cnt = 0; 
		}
                
	
                
                while(flag==2)
		{
			if(tflag1!=0&&tiflag1==0)
			{
				tiflag1=1;
				prev=get_ms();
				mdelay(10);

			}
			if(tflag1==0)
			{
				tflag1=1;
				tiflag1=1;
				prev=get_ms();
				printk("HELLO TX\n");
				mdelay(10);
			}


			if(kthread_should_stop()) return 0;
			if(get_ms()-prev>=80000)
			{

				atomic_set(&flag,20);

			}
		}
               sflag=0;
		while(flag==30)
		{    
                       
                       if(sflag==0)
                      {
                        sync_time=get_ms();
                        mdelay(2);
                       do
                        { 
                        ret=read_nowait(&ch,1);
                        if(ret>0)
                        {
                        printk("some missed data\n");
                        post_data(Rx_bufid,&ch,1);
                        }
                        }while(ret>0);
                        
                        printk("sync started\n");
                        ch=100;
                        serial_send(&ch,1);
                        ch=1;
                        sflag=1;
                      }
                        if(kthread_should_stop()) return 0;
                        ret=read_nowait(&ch,1);
                        if(ret>0)
                        {

                                  if(ch==101)
                                {
                                        atomic_set(&flag,20);
                                        sync_time1=get_ms()-sync_time;
                                        pca_tdma_start_timer();
                                        printk("recieved 101\n");
                                        ch=1;
                                        tflag=0;
                                        printk("\nsync time: %lu\n",sync_time1);
                                        break;
                                }

                        }
                        if(get_ms()-sync_time>=200000)
                          {
                            printk("sync mismatch\n");
                            atomic_set(&flag,20);
                            break;
                          }
                     }
                   schedule();
                __set_current_state(TASK_RUNNING);
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
	kt_periode = ktime_set(0,100000000); //seconds,nanoseconds
//	kt_periode = ktime_set(1, 0); //seconds,nanoseconds
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
//msleep(58);
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
	//pca_tdma_create_timer();
//	Tx_bufid = ring_buf_init();

	//print_rbuf(Tx_bufid);
//ret=get_data(Tx_bufid,buf,len);
//printk("ret = %d\n",ret);
	pca_tdma_set_timer(interval);
	spin_lock_init(&b_lock);
	//init internal buffers..
	// create a thread and save id into global variable.
//printk("init :tdma_main_thread %08x\n",tdma_main_thread);
//while(jiffies_to_msecs(jiffies)%1000);
//msleep(65);
//do_gettimeofday(&tv);
//time_to_tm(tv.tv_sec,0,&broken);
//printk("micro seconds are %ld \n",tv.tv_sec);

//printk("%d:%d:%d:%ld\n",broken.tm_hour,broken.tm_min,broken.tm_sec,tv.tv_usec);

	tdma_main_thread = kthread_run(&pca_tdma_main_loop,NULL,"main_loop");
//tdma_sync_frame();
//	pca_tdma_start_timer();
	return 0 ;
}

void tdma_sync_frame(void)
{
struct timeval tv;
struct tm broken;
unsigned long int msec;

do_gettimeofday(&tv);
msec = (tv.tv_sec % 1000)*1000+(tv.tv_usec/1000);
msleep(1000-(msec%1000));


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
          printk(KERN_INFO"TOTAL SEND: %d",cnt);
	int ret;
	ret = hrtimer_cancel( &hr_timer );
		kthread_stop(tdma_main_thread);
	if (ret) printk("The timer was still in use...\n");
	printk("HR Timer module uninstalling\n");
	printk(KERN_INFO "exiting pca_tdma_module\n");

}




module_init(pca_tdma_init);
module_exit(pca_tdma_exit);


