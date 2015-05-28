#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/ring_buf.h>
#include<linux/jiffies.h>
#include<linux/delay.h>
#include<linux/kthread.h>
#include<linux/time.h>
#include<linux/HIP.h>
MODULE_LICENSE("GPL");
int i=1;
int seq =0;
int Rx_bufid;
extern int rx_bufid;
int total=0;
int len=0;
int flag=0;
int cflag=0;
volatile unsigned long int tim=0;
volatile unsigned long int t[7];
volatile unsigned long int interval;
volatile int k=0;
struct task_struct *test_task;
extern struct task_struct *sleeping_task;
int nseq;
int loss;

int timer_handler(void * data)
{


	struct tp_voice_hdr vhdr;
	unsigned char buff[200];
	unsigned char ch;
	int r=0,ret;
	short int start,stop;
        struct frames fr;
        fr.start=START_FRAME;
        fr.stop=END_FRAME;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	while(!kthread_should_stop())
	{         

		set_current_state(TASK_INTERRUPTIBLE); 
		if(bytes_used(Rx_bufid)>=7 && len==0 && flag==0)
		{

			r=get_data(Rx_bufid,7,buff);
			if(r>0)
			{      
				if(cflag==0)
				{
					tim=jiffies;
					cflag=1;
				}

				pull(Rx_bufid,r);
				memcpy(&start,buff,sizeof(short int));
				if(start!=fr.start)
				{
					printk("START FRAME MISMATCH\n");
						flag=1;
					continue;
				}
                              label:
				if(buff[2]==16)
				{       
					
                                        memcpy(&vhdr,buff+3,4);
					if(vhdr.msgtype!=VOICE && seq!=vhdr.seqno)
					{ 
						printk("HEADER MISMATCH TP\n");
						flag=1;
						continue;
					}
					else
					{ 
						seq++;
						if(seq%4095 ==0)
							seq=0;
						len=vhdr.len;
						interval=jiffies;
						if(bytes_used(Rx_bufid)>=(len+2))
						{
							ret=get_data(Rx_bufid,len+2,buff);
                                                        pull(Rx_bufid,ret);
							memcpy(&stop,buff+len,sizeof(short int));
							if(stop!=fr.stop)
							{
	                                                        printk("END FRAME MISMATCH\n");
								flag=1;
								continue;
							}
							ret=post_data(rx_bufid,buff,len);
                                                        if(sleeping_task!=NULL)
                                                        wake_up_process(sleeping_task); 
	                                        	//printk("1post data return %d\n",ret);
							// wake_up_process(pull_task_syn);
							//	printk("PACKET NO:  %d SIZE OF PACKET:  %d\n ",i,r+ret);
							//	for(j=0;j<ret;j++)
							//		printk("RECIEVE: %d %d %d\n",buff[j],ret,i);
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
					continue;
				}

			}

		}
		else if(len!=0 && (bytes_used(Rx_bufid)>=(len+2)) && flag==0  )
		{

			ret=get_data(Rx_bufid,len+2,buff);
			pull(Rx_bufid,ret);
			memcpy(&stop,buff+len,sizeof(short int));
			if(stop!=fr.stop)
			{
			        printk("END FRAME MISMATCH\n");
				flag=1;
				continue;
			}

			ret=post_data(rx_bufid,buff,len);
                        if(sleeping_task!=NULL)
                        wake_up_process(sleeping_task); 
		//	printk("2 post data return %d\n",ret);
			// wake_up_process(pull_task_syn);

			//	printk("PACKET NO:  %d SIZE OF PACKET:  %d\n",i,r+ret);
			//	for(j=0;j<ret;j++)
			//		printk("RECIEVE: %d %d %di\n",buff[j],ret,i);
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
		else if(flag==1 && (bytes_used(Rx_bufid)>=7)) 
                       {
                         ret=get_data(Rx_bufid,1,&ch);
                         pull(Rx_bufid,ret);
                         if(ch==(START_FRAME>>8))
                         {
                           ret=get_data(Rx_bufid,1,&ch);
                           pull(Rx_bufid,ret);
                           if(ch==(START_FRAME & 0x00ff))
                           { 
                           ret=get_data(Rx_bufid,5,buff+2);
                           pull(Rx_bufid,ret);
                           start=START_FRAME;
                           memcpy(buff,&start,2);
                           memcpy(&vhdr,buff+3,4);
                           nseq=vhdr.seqno;
                           loss=nseq-seq;
                           if(loss<0)
                           loss=4095-seq+nseq;
                           printk("lost %d packets\n",loss+1);
                           seq=nseq;
                           flag=0;
                           goto label;
                           }
                           else
                              continue;
                         }
                           else
                             continue;
                       }


                else
		{
			schedule();
			__set_current_state(TASK_RUNNING);
			interval=jiffies;
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
	int z;
	printk("bytes in buffer%d\n",bytes_used(rx_bufid));
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


