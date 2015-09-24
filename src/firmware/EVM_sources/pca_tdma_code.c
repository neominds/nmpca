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
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/jiffies.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("BHARGAV");
MODULE_DESCRIPTION("TDMA module");


#define MS_TO_NS(x)     (x * 1E6L)

#define FRAME_SIZE 1000
static struct hrtimer hr_timer;
static ktime_t kt_periode;
ktime_t ktime;
static struct hrtimer hr_timer1;
static ktime_t kt_periode1;
ktime_t ktime1;
ktime_t ktime2;
unsigned long int previous;

unsigned long int previous1;
spinlock_t b_lock;

char event_queue[10000];
unsigned int evt_head=0,evt_tail=0;
unsigned int twheel_counter=0;
//unsigned int rx_bufid,tx_bufid;
uint16 current_slots_index = 0;
extern int Tx_bufid;
int slot;
//int flag;
atomic_t flag; 
struct task_struct *tdma_main_thread;
int rxstate,txstate;
unsigned long get_ms(void);
void tdma_sync_frame(void);
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

struct tdma_slot_entry  current_slots[PCA_TDMA_MAX_SLOTS] =   { {1 , TDMA_ACTION_RX_START },
	{2 , TDMA_ACTION_TX_START },
	{3 , TDMA_ACTION_RX_START },
	{4 , TDMA_ACTION_TX_START },
	{5 , TDMA_ACTION_RX_START },
	{6 , TDMA_ACTION_TX_START },
	{7 , TDMA_ACTION_RX_START },
	{8 , TDMA_ACTION_TX_START },
	{9 , TDMA_ACTION_RX_START },
	{10 , TDMA_ACTION_TX_START },
};  //only for test - to be initialized by algorithm

struct timeval tm;
unsigned long int msec;
unsigned int syn_count=0;
int syn_flag=0;
int count=10;

enum hrtimer_restart pca_tdma_timer_event_handler(  struct hrtimer *timer )
{

	int flag1;
	syn_count++;
        
         if(syn_count>=count||syn_flag<5)
        {
	        atomic_set((atomic_t *)&flag,30); 
		wake_up_process(tdma_main_thread);
		syn_count=0;
                if(syn_flag<5)
                syn_flag++;
                else
                count=100;
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
                        
			atomic_set((atomic_t *)&flag , 1);
                        wake_up_process(tdma_main_thread);

        //                wake_up_interruptible(tdma_main_thread);
//flag =1;
	//		ret=wake_up_process(tdma_main_thread);
//printk("wakeup process return value is %d \n",ret);
			break;
		case TDMA_ACTION_RX_STOP:   
	atomic_set((atomic_t *)&flag , 0);
//flag =0 ;
			//printk( " action  %d\n",action);
	//		printk( "rx stop action\n");
	//printk( "rx stop action %d \n",slot);
//printk("rxstop\n");
			rxstate = TDMA_ACTION_RX_STOP;
			//atomic_set(&rxstate, TDMA_ACTION_RX_STOP);
			break;
		case TDMA_ACTION_TX_START:
                  
               
		atomic_set((atomic_t *)&flag , 2);
                        wake_up_process(tdma_main_thread);
               
//flag =2 ;
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
			atomic_set((atomic_t *)&flag , 0);
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
struct timeval tq;

unsigned long get_ms(void)
{
do_gettimeofday(&tm);
return( (tm.tv_sec)*1000000 + (tm.tv_usec));
}

int cflag=0;
int guard_flag=0;
int num=0;
unsigned long t[7];
unsigned long tim;
int slot_cnt=0;
int slot_num;



unsigned long get_sec(void)
{
do_gettimeofday(&tq);
return(tm.tv_sec);
}

void pca_tdma_main_loop(void ) 
{
	atomic_set((atomic_t *)&guard_flag,0);
	int y;
	int tflag1=0,tflag2=0;
	int tiflag1,tiflag2;
	unsigned long int interval1=0,interval2=0;
	int len=0,i;
	unsigned char buf[1000];
	int evt,ret=0;
	char ch;
	unsigned long int time;
	unsigned long int inter2=0;
        unsigned long int sync_time;
        unsigned long int sync_time1;
	int count=0;
	int cnt=0;
	int z=0;
	ch=100;
	int fl=0;
        int sflag=0;

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
				printk("recieved 100 sending 101\n"); 
				ch=101;
				interval1=get_ms();
				serial_send(&ch,1);
				ch=1;
			}
			else if(ch==101)
			{ 
				printk("recieved 101 sending 102\n");
				ch=102;
				serial_send(&ch,1);
				ch=1;
				break;
			}
			else if(ch==102)
			{      
				interval2=get_ms()-interval1;
				printk("recieved 102\n");
				break;
			}


		}

		msleep_interruptible(5);
	}


	mdelay(20-interval2/2000);
	printk("interval %lu\n",interval2/2);
        interval1=0;
        interval2=0;
	previous1=0;
	pca_tdma_start_timer();
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
	while(!kthread_should_stop())
	{
		
		tiflag1=0;
		tiflag2=0;
                set_current_state(TASK_INTERRUPTIBLE);
		while(atomic_read(&flag)==1)
		{
			if(kthread_should_stop()) return 0;
			
                        if(tflag1!=0&&tiflag1==0)
			{
				interval1=get_ms();
				tiflag1=1;
			}

			if(tflag1==0)
			{   

				interval1=get_ms();
				tflag1=1;
				tiflag1=1;
                                printk("HELLO TX\n");

			} 

		}
		cnt=0;
		unsigned int count1=0;
		while(atomic_read(&flag)==2)

		{

			if(tflag2!=0&&tiflag2==0)
			{

				slot_num++;     
				interval2=get_ms()-previous1;
				previous1=get_ms();
				tiflag2=1;
		        	mdelay(10);
			}
			if(tflag2==0)
			{
				slot_num++;
				previous1=get_ms();
				tflag2=1;
				tiflag2=1;
		         	mdelay(10);
                                printk("HELLO RX\n");
			} 


			if(kthread_should_stop()) return 0;

			ret=get_data(Tx_bufid,1,buf);
			pull(Tx_bufid,ret);

			if(ret<=0)
			{

			}
			else
			{
				if(ret>0)
				{       
					if(cflag==0)
					{
						tim=get_sec();
						cflag=1;
					}
					previous=get_ms();    
					ret = serial_send(buf,ret);
					count1++;
					cnt=cnt+ret;
					slot_cnt=slot_cnt+ret;
                                  if(slot_cnt==20000||slot_cnt==40000||slot_cnt==60000||slot_cnt==80000||slot_cnt==100000)
                                   {
                                     t[num]=get_sec()-tim;
                                     num++;
                                   }
				}
			}

			

			if(get_ms()>=(previous1+90000)||cnt>=1030)
			{
                                inter2=get_ms()-previous1;
				if(count1!=0)		
					printk("slot no: %d  time interval: %lu guard interval %lu count: %d bytes_slot: %d total_bytes: %d\n",slot_num,interval2,inter2,count1,cnt,slot_cnt);
				atomic_set((atomic_t *)&flag,20);
			} 
		}

   sflag=0;
		while(atomic_read(&flag)==30)
		{      
                       
                       if(sflag==0)
                       {
                        sync_time=get_ms();
                        printk("sync start\n");
                        sflag=1;
                       }


                        if(kthread_should_stop()) return 0;

                        ret = read_nowait(&ch,1);
                        if(ret>0)
                        {
                                if(ch==100)
                                {
                                        ch=101;
                                        serial_send(&ch,1);
                                        printk("recieved 100 sending 101\n");
                                        ch=1;
                                 }
                                    
                               else  if(ch==102)
                                {
                                        ch=103;
                                        serial_send(&ch,1);
                                        atomic_set((atomic_t *)&flag,20);
                                        sync_time1=get_ms()-sync_time;
                                        pca_tdma_start_timer();
                                        printk("recieved 102 sending 103\n");
                                        printk("\nsync time: %lu\n\n",sync_time1);
                                        tflag2=0;
                                        break;
                                }
                        }
                         if(get_ms()-sync_time>=200000)
                          {
                            printk("sync mismatch\n");
                            atomic_set((atomic_t *)&flag,20);
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
	kt_periode = ktime_set(0, 100000000); //seconds,nanoseconds
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

int tot=0;
enum hrtimer_restart pca_tdma_timer_event_handler1(  struct hrtimer *timer )
{
       
	if(tot==0)
	{
                printk("hello\n");
		ktime2=ktime_set(0,150000000);
		hrtimer_forward_now(timer, ktime2);
		tot=1;
                previous1=get_ms();
		return HRTIMER_RESTART;
	}
 
//	atomic_set(&guard_flag,1);
      //  printk("first time interval %lu",get_ms()-previous1);
	kt_periode1=ktime_set(0,200000000);
	hrtimer_forward_now(timer, kt_periode1);
	return HRTIMER_RESTART;
}


	


int pca_tdma_set_timer1(unsigned long interval)
{

//	ktime_t ktime1;


	//kt_periode = ktime_set(0, 204167); //seconds,nanoseconds
	kt_periode1 = ktime_set(0, 200000000); //seconds,nanoseconds
	ktime2 = ktime_set(0,190000000); //seconds,nanoseconds
//	kt_periode = ktime_set(1, 0); //seconds,nanoseconds
	printk("HR Timer module1 installing\n");

	

	hrtimer_init( &hr_timer1, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	hr_timer1.function = &pca_tdma_timer_event_handler1;

	//this function will set timer.
	//return success or fail
	return  SUCCESS;
}

void pca_tdma_start_timer1(void )
{
        
//	unsigned long delay_in_ms = 190L;
//	ktime1 = ktime_set( 0, MS_TO_NS(delay_in_ms) );
	//printk( "Starting timer to fire in %ldms (%ld)\n", delay_in_ms, jiffies );
	printk( "Starting timer to fire in ms \n");
//msleep(58);
	hrtimer_start( &hr_timer1, ktime1, HRTIMER_MODE_REL );
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
	//print_rbuf(Tx_bufid);
//ret=get_data(Tx_bufid,buf,len);
//printk("ret = %d\n",ret);
        //pca_tdma_set_timer1(interval);
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
        int i;
        int ret;
ret = hrtimer_cancel( &hr_timer );
//ret = hrtimer_cancel( &hr_timer1);
                kthread_stop(tdma_main_thread);
printk("Total BYTes SEND: %d\n",slot_cnt);
for(i=0;i<num;i++)
printk("BYTES: %d TIME: %lu\n",(i+1)*20000,t[i]);
        if (ret) printk("The timer was still in use...\n");
        printk("HR Timer module uninstalling\n");
        printk(KERN_INFO "exiting pca_tdma_module\n");

}


module_init(pca_tdma_init);
module_exit(pca_tdma_exit);



                                                                                
