#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/ring_buf.h>
#include<linux/jiffies.h>
#include<linux/HIP.h>
#include<linux/delay.h>
#include<linux/kthread.h>
#include<linux/time.h>

MODULE_LICENSE("GPL");
int i=1;
int seq =0;
int Rx_bufid;
int rx_bufid;
int total=0;
int len=0;
int flag=0;
int cflag=0;
struct timer_list my_timer;
volatile unsigned long int tim=0;
volatile unsigned long int t[7];
volatile int k=0;
struct task_struct *test_task;

int timer_handler(void * data)
{


	struct tp_voice_hdr vhdr;
	unsigned char buff[200];
	int r=0,ret;
	int j;
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        
        while(!kthread_should_stop())
      {         
           
       set_current_state(TASK_INTERRUPTIBLE); 
	if(bytes_used(Rx_bufid)>=5 && len==0 && flag==0)
	{
	
 
        
       	r=get_data(Rx_bufid,5,buff);
		if(r>0)
		{      
			if(cflag==0)
			{
				tim=jiffies;
				cflag=1;
			}

			pull(Rx_bufid,r);
			if(buff[0]==16)
			{
				memcpy(&vhdr,buff+1,4);
				if(vhdr.msgtype!=VOICE||vhdr.seqno!=seq)
				{ 
					printk("HEADER MISMATCH TP\n");
					flag=1;
				}
				else
				{ 
					seq++;
					if(seq%4095 ==0)
						seq=0;
					len=vhdr.len;
					if(bytes_used(Rx_bufid)>=len)
					{
						ret=get_data(Rx_bufid,len,buff);

						pull(Rx_bufid,ret);
						ret=post_data(rx_bufid,buff,len);
						printk("PACKET NO:  %d SIZE OF PACKET:  %d\n ",i,r+ret);
					//	for(j=0;j<ret;j++)
							//printk("RECIEVE: %d %d %di\n",buff[j],ret,i);
						i++;
						total=total+ret;
						len=0;
						if(total==20000||total==40000||total==60000||total==80000||total==100000)
						{
							t[k]=jiffies-tim;
							k++;
                                                        printk("Total %d\n", total);
                                                }
					}

				}
			}
			else
			{
				printk("HEADER MISMATCH IC\n");
				flag=1;
			}

		}
         
	}
	else if(len!=0 && flag==0)
	{
                
		if(bytes_used(Rx_bufid)>=len)
		{
			ret=get_data(Rx_bufid,len,buff);

			pull(Rx_bufid,ret);
			ret=post_data(rx_bufid,buff,len);
			printk("PACKET NO:  %d SIZE OF PACKET:  %d\n",i,r+ret);
		//	for(j=0;j<ret;j++)
			//	printk("RECIEVE: %d %d %di\n",buff[j],ret,i);
			i++;
			total=total+ret;
			len=0;

			if(total==100000||total==80000||total==20000||total==40000||total==60000)
			{
				t[k]=jiffies-tim;
				k++;
                                printk("Total %d\n", total);
			}
		}


	}
         else
             {
                
                  schedule();
               __set_current_state(TASK_RUNNING);
                
              }

        msleep_interruptible(5);
	if(flag==1)
	    {
              set_current_state(TASK_INTERRUPTIBLE);
              schedule();
             }
    
  
    }
  return(0);
}


static int start(void)
{

/*	init_timer(&my_timer);
	my_timer.expires=jiffies+5;
	my_timer.function=&timer_handler;
	my_timer.data=0;
	add_timer(&my_timer);*/
        rx_bufid =ring_buf_init();
        Rx_bufid =ring_buf_init();
        printk("Rx_bufid %d rx_bufid %d",Rx_bufid,rx_bufid);
        test_task=kthread_run(&timer_handler,NULL,"TEST_THREAD_PROCESSING");
	return(0);
}

void  exit(void)
{
        printk("bytes in buffer%d\n",bytes_used(rx_bufid));
	int z;
	printk("TOTAL RECIEVED DATA:  %d\n",total+total/100*5);
	for(z=0;z<k;z++)
		printk("BYTES: %d TIME: %lu\n",(z+1)*20000,t[z]/HZ);
//	del_timer(&my_timer);
          kthread_stop(test_task);
        uninit(Rx_bufid);
        uninit(rx_bufid);
}


module_init(start);
module_exit(exit);


EXPORT_SYMBOL(test_task);
EXPORT_SYMBOL(Rx_bufid);
EXPORT_SYMBOL(rx_bufid);
