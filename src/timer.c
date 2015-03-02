
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
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <linux/timer.h>
#include "app.h"

//#include "HIP.h"
#include <linux/HIP.h>

MODULE_LICENSE("GPL");

//#define SET_TIMER_TO	(2/100)
#define SET_TIMER_TO	1

#define MS_TO_NS(x)	(x * 1E6L)

#define FIRST_MINOR	0
#define MINOR_CNT	1

#define MY_MACIG	'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)

extern struct task_struct *tp_tx_task;

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

//static struct hrtimer hr_timer;
//static ktime_t kt_periode;
//ktime_t ktime;

struct timer_list timer;

void timer_interrupt(void)
{
	static int i=0;
	printk("Inside timer interrupt\n");
	read_data_from_vocoder();
	if(i<32) mod_timer(&timer, jiffies + SET_TIMER_TO*HZ);
	i++;
}

static int post_open(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "\n Post Open \n");
	return 0;
}


static int post_release(struct inode *inode, struct file *file)
{
	//	printk(KERN_ALERT "\n Post Release \n");
	return 0;
}
//static int device_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long args)
static long post_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int ret;//i;
	unsigned char r;
	int len;
	struct post_st *buf;
	buf=(struct post_st *)args;
	copy_from_user(&r,(unsigned char *)&buf->type,1);
	printk("\n ioctl type = %d \n",r);
	if(r==4) {
		//hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
		init_timer(&timer);
		timer.function=timer_interrupt;
		timer.expires=jiffies + SET_TIMER_TO*HZ;
		add_timer(&timer);
		printk("timer started\n\n");
	}
	if(r==5) {
		//hrtimer_cancel( &hr_timer );
		del_timer_sync(&timer);
		printk("The timer was still in use...\n");
	}

	return 0;
}

static struct file_operations fops = {
	.open = post_open,
	.release = post_release,
	.unlocked_ioctl = post_ioctl,
};

/*
enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
//	static int i;

	printk( "my_hrtimer_callback called (%ld).\n", jiffies );

//	wake_up_process(tp_tx_task);
	int i;
	unsigned char *p = my_hrtimer_callback;
	//printk("my_hrtimer_callback is at %p\n",&my_hrtimer_callback);
	for(i=0;i<30;i++)
		printk("%p ",*(p+i));
	printk("\n");

	//read_data_from_vocoder();

//	if(i<7)
//		hrtimer_forward_now(timer, kt_periode);
//	i++;

	return HRTIMER_RESTART;
}*/

int init_module( void )
{
//	unsigned long delay_in_ms = 10200L;
	int ret, i;
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

/*	//    kt_periode = ktime_set(0, 204167); //seconds,nanoseconds
	kt_periode = ktime_set(10, 200000000); //seconds,nanoseconds
	printk("HR Timer module installing\n");

	ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );

	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );

	hr_timer.function = &my_hrtimer_callback;
	//hr_timer.function = &read_data_from_vocoder;


	unsigned char *p = my_hrtimer_callback;
	printk("my_hrtimer_callback is at %p\n",&my_hrtimer_callback);
	for(i=0;i<30;i++)
		printk("%p ",*(p+i));
	printk("\n");
	printk( "Starting timer to fire in %ldms (%ld)\n", delay_in_ms, jiffies );*/

	printk( "Starting timer to fire in %ds (%ld)\n", SET_TIMER_TO, jiffies );

	return 0;
}

void cleanup_module( void )
{
	int ret;
	printk(KERN_ALERT "\n Post Device is Released or closed \n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
	printk(KERN_INFO "Cleaning up post_ring_buf module.\n\n");

//	hrtimer_cancel( &hr_timer );
	printk("HR Timer module uninstalling\n");

	return;
}


