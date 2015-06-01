#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/ring_buf.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include "leaky.h"
#include <linux/timer.h>
#include <asm/param.h>
#include <linux/spinlock.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Subin");
MODULE_DESCRIPTION("A leaky bucket module");


int put_into_bucket(unsigned int bucket_id,unsigned char *buff,unsigned int size)
{
	return(post_data(bucket_id,buff,size));
}




int leak_bucket(unsigned int bucket_id,unsigned char *buff)
{
	int len ;
	len =get_data(bucket_id,LEAK_SIZE,buff);
	if(len>0&&len<20)
	{ 
                pull(bucket_id,0);
		return(ERR_NO_ENOU_DAT);
	}
	else
		return(len);
}

int leak_bucket_init(void)
{
return(ring_buf_init());
}

EXPORT_SYMBOL(leak_bucket_init);
EXPORT_SYMBOL(leak_bucket);
EXPORT_SYMBOL(put_into_bucket);


