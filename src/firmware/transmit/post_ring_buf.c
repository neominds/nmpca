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
#include <linux/slab.h>//kmalloc
#include <asm/uaccess.h>
#include<linux/uart_driver_x86.h>
#include "app.h"
//#include "linux/ring_buf.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHOYAB");
MODULE_DESCRIPTION("A Post Ring buffer module");

#define FIRST_MINOR 0
#define MINOR_CNT 1

#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

int bufid;
extern int tx_bufid;
extern struct task_struct *voc_task;
#ifdef KTHREADS
extern struct task_struct *post_task;
extern struct task_struct *pull_task;
unsigned char in_buf[NO_OF_BYTES_TO_POST];

int post_data_to_ring_buffer(void *buff)
{
	int ret;
	unsigned char *buf = buff;

	//printk(KERN_INFO "Kernel Thread : %s pull_task:%p &pull_task:%p\n",post_task->comm, pull_task, &pull_task);
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	bufid = ring_buf_init();
	while(!kthread_should_stop()) {
//		spin_lock(&p_lock);
		set_current_state(TASK_RUNNING);
		ret = post_data(bufid, buf, NO_OF_BYTES_TO_POST);
	//	printk(KERN_INFO "\n\npost_data return: %d\n\n",ret);
		if(ret == 0) {
			printk(KERN_ALERT "\nFailed to post data to ring buffer\n");
//			spin_unlock(&p_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			wake_up_process(pull_task);
			schedule();
			//spin_lock(&p_lock);
			continue;
			//return 0;
		}

//		spin_unlock(&p_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		wake_up_process(pull_task);
		schedule();
		/*while(!kthread_should_stop())
		  {
		  schedule();
		  }*/
	}

	return 0;
}
#endif

static int post_open(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "\n Post Open \n");
	return 0;
}
int total=0;
// j unsigned char buf[NO_OF_BYTES_TO_POST];
unsigned char dummy[NO_OF_BYTES_TO_POST];
//static int device_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long args)
static long post_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
		int ret;//i;
		unsigned char r;
		int len;
//                int total=0;
		//unsigned char in_buffer[NO_OF_BYTES_TO_POST];
		struct post_st *buf;
		buf=(struct post_st *)args;
		//unsigned char * in_buffer;
	switch(cmd)
	{
		case WRITE_IOCTL:
			copy_from_user(&r,(unsigned char *)&buf->type,1);
			copy_from_user(&len,(int *)&buf->len,4);
			unsigned char *in_buffer=(unsigned char *)kmalloc(len,GFP_KERNEL);
			copy_from_user(in_buffer,(unsigned char *)buf->buf, len);
		//	Tx_bufid=r;
		

		//	if(Tx_bufid==Tx)
		//	{
			 ret = post_data(tx_bufid, in_buffer,len);
                         total=total+ret;
		       	//printk(KERN_INFO "\nposted %d bytes \n",ret);
		//	}
		//	 else{
	//		 ret = get_data(Tx_bufid, NO_OF_BYTES_TO_PULL, in_buffer);
	//	       	printk(KERN_INFO "\n\nget_data return: %d\n\n",ret);
	//		if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) {
	//			printk(KERN_ALERT "\nFailed to get data from ring buffer\n");
	//			return -1;
	//		}	
	//		ret = pull(Tx_bufid, ret);
	//		if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) {
	//			printk(KERN_ALERT "\nFailed to pull data from ring buffer\n");
	//			return -1;
	//		}
		//	}// end of else
			 // get_data;
			 printk(KERN_INFO "\n\npost_data return: %d %d\n\n",ret,total);
                         
                       //  wake_up_process(voc_task);
			if(ret == 0) {
				printk(KERN_ALERT "\nFailed to post data to ring buffer\n");
				return -1;
			}
			 
                      
                       wake_up_process(voc_task);
                   break;
	}
	return 0;
}

static int post_release(struct inode *inode, struct file *file)
{
//	printk(KERN_ALERT "\n Post Release \n");
	return 0;
}

static struct file_operations fops = {
	.open = post_open,
	.release = post_release,
	.unlocked_ioctl = post_ioctl,
};


static int __init post_main(void)
{
#ifdef KTHREADS
	int i;

	for(i=0;i<NO_OF_BYTES_TO_POST;i++)
		in_buf[i]=0xff;
	post_task = kthread_run(&post_data_to_ring_buffer, (void *)in_buf,"post_data");
	printk(KERN_INFO "Kernel Thread : %s post_task:%p &post_task:%p\n",post_task->comm, post_task, &post_task);
#endif
	int ret;
	struct device *dev_ret;

	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "my_dev")) < 0)
	{
		return ret;
	}

	cdev_init(&c_dev, &fops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "my_dev")))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

	//bufid = ring_buf_init();
	tx_bufid = ring_buf_init();
//	rx_bufid = ring_buf_init();
	//printk(KERN_ALERT "\n Post Device is initialize \n");
	//printk(KERN_ALERT "\n Tx_bufid is %d \n",Tx_bufid);
	//printk(KERN_ALERT "\n bufid is %d \n",bufid);
	//printk(KERN_ALERT "\n Rx_bufis is %d \n",Rx_bufid);

	return 0;
}

static void __exit post_cleanup(void)
{
#ifdef KTHREADS
	kthread_stop(post_task);
#endif
	printk(KERN_ALERT "\n Post Device is Released or closed \n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
	printk(KERN_INFO "Cleaning up post_ring_buf module.\n\n");
}

#ifdef KTHREADS
//EXPORT_SYMBOL(post_task);
EXPORT_SYMBOL(post_data_to_ring_buffer);
//EXPORT_SYMBOL(bufid);
#endif

//EXPORT_SYMBOL(post_data_to_ring_buffer);


module_init(post_main);
module_exit(post_cleanup);


