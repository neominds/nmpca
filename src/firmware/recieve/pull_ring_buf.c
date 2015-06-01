//#include <linux/spinlock.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/ring_buf.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include<linux/uart_driver_x86.h>
//#include<linux/ring_buf.h>
#include<linux/slab.h>//kmalloc
#include"app.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHOYAB");
MODULE_DESCRIPTION("A Ring buffer module");

#define FIRST_MINOR 1
#define MINOR_CNT 1

#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)
#define READ_IOCTL_SYN _IOR(MY_MACIG,2,int)
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

extern int bufid;
//extern int tx_bufid;
int rx_bufid;
int size=1;
struct task_struct *pull_task_syn;
struct task_struct *sleeping_task;
struct pull_st *buf;
unsigned char get_buf[NO_OF_BYTES_TO_PULL];


#ifdef KTHREADS
extern struct task_struct *pull_task;
extern struct task_struct *post_task;
unsigned char out_buf[NO_OF_BYTES_TO_PULL];
extern struct data_ring_buff rings[RING_BUFFER_SLOTS];
int pull_data_from_ring_buffer(void *buff)
{
	int ret, i;
	unsigned char *get_buf = buff;

//	printk(KERN_INFO "\n\nKernel Thread : %s [post_task:%p, &post_task:%p]\n\n",post_task->comm, post_task, &post_task);
	set_current_state(TASK_INTERRUPTIBLE);
	wake_up_process(post_task);
	schedule();

	while(!kthread_should_stop()) {
//		spin_lock(&p_lock);
		set_current_state(TASK_RUNNING);
		ret = get_data(bufid, NO_OF_BYTES_TO_PULL,get_buf);
		printk(KERN_INFO "\n\nget_data return: %d\n\n",ret);
		if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) {
			printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
//			spin_unlock(&p_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			wake_up_process(post_task);
			schedule();
			//spin_lock(&p_lock);
			continue;
			//return 0;
		}
		ret = pull(bufid, NO_OF_BYTES_TO_PULL);
		if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) {
			printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
//			spin_unlock(&p_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			wake_up_process(post_task);
			schedule();
			//spin_lock(&p_lock);
			continue;
			//return 0;
		}

	//	printk(KERN_EMERG "\n\npull buf:");
		for(i=0;i<NO_OF_BYTES_TO_PULL;i++) {
			if(get_buf[i] != 0xff)
				printk(KERN_INFO "\n\nBuffer Corrupted: get_buf[%d]=%d\n\n",i,get_buf[i]);
		}
	//		printk(KERN_EMERG "%02x ",get_buf[i]);
	//	printk(KERN_EMERG "\n\n");

//		printk(KERN_INFO "\n\npull return: %d\n\n",ret);

		/*while(!kthread_should_stop())
		  {
		  schedule();
		  }*/

//		spin_unlock(&p_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		wake_up_process(post_task);
		schedule();
	}

	return 0;
}
#endif




int pull_thread_syn(void *data)
{
        int ret;
        int r;
        int f;
        int flag=0;
        printk("pull thread started\n");
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	while(!kthread_should_stop())
	{

		set_current_state(TASK_INTERRUPTIBLE);
		if(size==0 && (bytes_used(rx_bufid)>=39))
		{
                        
			ret = get_data(rx_bufid,117, get_buf);
                        printk(KERN_INFO "\n\nget_data return: %d  \n\n",ret);
			if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) {
				printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
                                flag=1;
	                        break;
				
			}
			r=ret;
			ret = pull(rx_bufid, ret);
			if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) {
				printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
                                flag=1;
				break;
			}

		        f=copy_to_user((unsigned char *)buf->buf, get_buf,r);                       
                        printk("return value of copy to userbuf %d\n",f);
                        f=copy_to_user((int *)&buf->len,&r,4);
                        printk("return value of copy to userlen %d\n",f);
                        size=r;
                        printk("copied to user\n");
		}
		schedule();
		__set_current_state(TASK_RUNNING);
	}
     if(flag==1)
    {
       while(!kthread_should_stop())
       {
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
       }
    }
      return(0);

}


static int pull_open(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "\n Open in pull \n");
	return 0;
}

struct pull_st *buf;
	
unsigned char get_buf[NO_OF_BYTES_TO_PULL];
//static int device_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long args)
static long pull_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int ret,r,len,num;

	buf=(struct pull_st *)args;

	switch(cmd)
	{
		case READ_IOCTL:
			memset(get_buf, 0, NO_OF_BYTES_TO_PULL);  
			copy_from_user(&len,(int *)&buf->len,4);

			{

				if( bytes_used(rx_bufid) >= len)
				{

					ret = get_data(rx_bufid,len, get_buf);

					//printk(KERN_INFO "\n\nget_data return: %d  \n\n",ret);
					if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) {
						printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
						return -1;
					}
					r=ret;
					ret = pull(rx_bufid, ret);
					if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) {
						printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
						return -1;
					}

					copy_to_user((unsigned char *)buf->buf, get_buf,len);
					return r;

				}
				else
				{  //printk("not enough data in ringbuffer\n");
					return -1;
				}
			}

		case READ_IOCTL_SYN:
			copy_from_user(&len,(int *)&buf->len,4);
			if(bytes_used(rx_bufid)<len)
			{
				sleeping_task=current;
				set_current_state(TASK_INTERRUPTIBLE);
				r=schedule_timeout(25);
			//	printk("timeout return value %d\n",r);   
			}
			
			if(len<=(bytes_used(rx_bufid)))
			{
				ret = get_data(rx_bufid,len, get_buf);

				//printk(KERN_INFO "\n\nget_data return: %d  \n\n",ret);
				if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) 
				{

					printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
					return -1;
				}
				r=ret;
				ret = pull(rx_bufid, ret);
				if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) 
				{
					printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
					return -1;
				}
				copy_to_user((unsigned char *)buf->buf, get_buf,len);
				return r;
			}
                        else if(len>=(bytes_used(rx_bufid)) && bytes_used(rx_bufid)>0)
                        {
                          num=bytes_used(rx_bufid);
                          
				ret = get_data(rx_bufid,num, get_buf);

				//printk(KERN_INFO "\n\nget_data return: %d  \n\n",ret);
				if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) 
				{

					printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
					return -1;
				}
				r=ret;
				ret = pull(rx_bufid, ret);
				if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) 
				{
					printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
					return -1;
				}
                                
				copy_to_user((unsigned char *)buf->buf, get_buf,r);
				copy_to_user((int *)&buf->len, &r,4);
                                return r;
                        }
                        else
                          return -1;
                         
		default:
			printk("wrong command\n");
			return -1;
	}
}

static int pull_release(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "\n Pull Release \n");
	return 0;
}

static struct file_operations fiops = {
	.open = pull_open,
	.release = pull_release,
	.unlocked_ioctl = pull_ioctl,
};

static int __init pull_main(void)
{
#ifdef KTHREADS
	pull_task = kthread_run(&pull_data_from_ring_buffer, (void *)out_buf,"pull_data");
	printk(KERN_INFO "Kernel Thread : %s: pull_task:%p, &pull_task:%p\n",pull_task->comm, pull_task, &pull_task);
#endif
	int ret;
	struct device *dev_ret;
        //rx_bufid=ring_buf_init();
	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "my_cdev")) < 0)
	{
		return ret;
	}

	cdev_init(&c_dev, &fiops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "char1")))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "my_cdev")))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}
	
  //      pull_task_syn = kthread_run(&pull_thread_syn,NULL,"pull_data_syn_thread");

	printk(KERN_ALERT "\n PUll Device is initialize \n");

	return 0;
}

static void __exit pull_cleanup(void)
{
#ifdef KTHREADS
	kthread_stop(pull_task);
#endif
//	kthread_stop(pull_task_syn);
	printk(KERN_ALERT "\n Pull Device is Released or closed \n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
	printk(KERN_INFO "Cleaning up pull_ring_buf module.\n\n");
        printk("%d\n",rx_bufid);
}

#ifdef KTHREADS
//EXPORT_SYMBOL(pull_task);
EXPORT_SYMBOL(pull_data_from_ring_buffer);
#endif
EXPORT_SYMBOL(sleeping_task);
EXPORT_SYMBOL(pull_task_syn);
EXPORT_SYMBOL(rx_bufid);
module_init(pull_main);
module_exit(pull_cleanup);
