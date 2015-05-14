//#include <stdio.h>
//#include <string.h>
//#include <assert.h>
//#include <pthread.h>
//#include <sipa.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/ring_buf.h>

MODULE_LICENSE("NEOMINDS");
MODULE_AUTHOR("SHOYAB");
MODULE_DESCRIPTION("A Ring buffer module");

struct task_struct *post_task;
struct task_struct *pull_task;
int bufid; 

struct data_ring_buff rings[RING_BUFFER_SLOTS];

unsigned char init_done = 0;

int ring_buf_init(void)
{
	unsigned int l_bufid;
	int i;

	if (!init_done){
		for (i=0; i< RING_BUFFER_SLOTS; i++)
			rings[i].bufid = SLOT_AVAILABLE;
		init_done = 1;
	}

	for (l_bufid = 0; l_bufid < RING_BUFFER_SLOTS; l_bufid++)
		if (rings[l_bufid].bufid == SLOT_AVAILABLE) break;
	if (l_bufid == RING_BUFFER_SLOTS) return E_NO_RING_FREE;

	rings[l_bufid].bufid = l_bufid;

	//if(pthread_mutex_init(&rings[l_bufid].b_lock, NULL)) 
	spin_lock_init(&rings[l_bufid].b_lock); 
//	spin_lock_init(&p_lock); 
	
	memset(rings[l_bufid].ring_buf,0,BYTES_IN_BUFFER);
	rings[l_bufid].tail = -1;
	rings[l_bufid].head = -1;
	rings[l_bufid].read = -1;
	rings[l_bufid].bytes_free = BYTES_IN_BUFFER;
	rings[l_bufid].bytes_used = 0;
	rings[l_bufid].op_ok = OP_OK;
        printk("ringbuf id %d\n",l_bufid);
	return l_bufid;
}

inline int ring_buf_free_count(unsigned int bufid){
	return rings[bufid].bytes_free;
}

int uninit(unsigned int bufid)  
{
	//if(pthread_mutex_destroy(&rings[bufid].b_lock)) 
	//if(spin_unlock(&rings[bufid].b_lock)) 
	//	return E_THREAD;

	DEFINE_SPINLOCK(x);
	rings[bufid].b_lock = x;
	rings[bufid].bufid = SLOT_AVAILABLE;
	return 0;
}

void state_change(unsigned int bufid)
{
//	printk(KERN_INFO "Space Avaliable is :%d\n",rings[bufid].bytes_free );
//	printk(KERN_INFO "Number of Bytes used in Buffer Now: %d\n",rings[bufid].bytes_used);

	if(rings[bufid].bytes_free == 0)
		rings[bufid].state = BUF_ROOM_FULL;
	else if(rings[bufid].bytes_free == BYTES_IN_BUFFER)
		rings[bufid].state = BUF_ROOM_EMPTY;
	else
		rings[bufid].state = BUF_ROOM_NORMAL;
}

void print_rbuf(unsigned int bufid)
{
	//int i;

	printk(KERN_INFO "bufid = %d]\n",bufid);
	printk(KERN_INFO "head = %d]\n", rings[bufid].head);
	printk(KERN_INFO "tail = %d]\n", rings[bufid].tail);
	printk(KERN_INFO "read = %d]\n", rings[bufid].read);
	printk(KERN_INFO "rings[l_bufid].bytes_free = %d\n",rings[bufid].bytes_free);
	printk(KERN_INFO "rings[l_bufid].bytes_used = %d\n",rings[bufid].bytes_used);
	printk(KERN_INFO "BUF_STATE :\t ");

	switch(rings[bufid].state)
	{	
		case BUF_ROOM_FULL:
			printk(KERN_INFO "FULL\n");
			break;

		case BUF_ROOM_EMPTY:
			printk(KERN_INFO "EMPTY\n");
			break;

		case BUF_ROOM_NORMAL:
			printk(KERN_INFO "NORMAL\n");
			break;
	}

//	for (i=0; i< BYTES_IN_BUFFER; i++) 
//		printk(KERN_INFO "%02x ",rings[bufid].ring_buf[i]);
//	printk(KERN_INFO "\n");
}

void printall_rbufs(void)
{
	unsigned int i;

	for(i=0;i<RING_BUFFER_SLOTS;i++)
	{
		printk(KERN_INFO "--------------------------\n");
		print_rbuf(i);
	}

	printk(KERN_INFO "\n");
}

void lock(unsigned int bufid)
{
	spin_lock(&rings[bufid].b_lock);
}

void unlock(unsigned int bufid)
{
	spin_unlock(&rings[bufid].b_lock);
}

int post_data(unsigned int bufid, unsigned char *buf, unsigned int nbytes)
{
//	printk(KERN_INFO "Post_data request received for [%d bytes] to buffer id[%d]\n",nbytes,bufid);
	//int i;
	spin_lock(&rings[bufid].b_lock);

	assert(rings[bufid].bufid != SLOT_AVAILABLE);

//	printk(KERN_INFO "bytes free %d\t n = %d\t",rings[bufid].bytes_free,nbytes);	
//	printk(KERN_INFO "ring buffer at %p\n",rings[bufid].ring_buf);	

	if(nbytes <= 0)
		return 0;

	if (rings[bufid].bytes_free < nbytes)
	{
//		printk(KERN_INFO "Now Copying Only[ %d ] Bytes \t Remaining Need To Copy is [ %d ] \n",
//						rings[bufid].bytes_free,(nbytes - rings[bufid].bytes_free));
		nbytes = rings[bufid].bytes_free;
	}

//	printk(KERN_INFO "Post_data for [%d bytes] to buffer id[%d]\n",nbytes,bufid);
//print_rbuf(bufid);
	if ( ( rings[bufid].tail + nbytes + 1) > BYTES_IN_BUFFER)
	{
//		unsigned int bytes_chunk1 = (BYTES_IN_BUFFER - (rings[bufid].tail +1));
//		unsigned int write_position = rings[bufid].tail + 1;

		// need to push data in thwo chunks. one from tail to end and another from beginning 
		memcpy((char *)(rings[bufid].ring_buf + rings[bufid].tail + 1),
				buf,
				(BYTES_IN_BUFFER - (rings[bufid].tail +1)));

		memcpy(rings[bufid].ring_buf,
				buf+(BYTES_IN_BUFFER - (rings[bufid].tail + 1)),
				nbytes-(BYTES_IN_BUFFER - (rings[bufid].tail + 1)));

		//new
		//write chunk1
/*		memcpy((unsigned char *)(rings[bufid].ring_buf + write_position),
				buf,
				bytes_chunk1);
		//write chunk2
		memcpy((unsigned char *)rings[bufid].ring_buf,
				buf + bytes_chunk1,
				nbytes - bytes_chunk1);*/
	}

	else
		memcpy((unsigned char *)(rings[bufid].ring_buf + rings[bufid].tail + 1),
				buf,
				nbytes);

	rings[bufid].tail = (rings[bufid].tail + nbytes) % BYTES_IN_BUFFER;

	rings[bufid].bytes_used += nbytes;

	rings[bufid].bytes_free  -= nbytes;

	state_change(bufid);  //maybe not needed ??

	spin_unlock(&rings[bufid].b_lock);
	//j printk("\n inside ppost_data \n");
	 //for(i=0;i<100;i++) printk("%c ",rings[bufid].ring_buf[i]);
//print_rbuf(bufid);
//	printk(KERN_INFO "Posted data for [%d bytes] to buffer id[%d]\n",nbytes,bufid);
	return nbytes;
}

int get_data(unsigned int bufid, unsigned int numbytes,char *get_buf)
{
	//int i;
	//printk("value of buffid in get_dat is %d\n",bufid);
	spin_lock(&rings[bufid].b_lock);

	assert(rings[bufid].bufid != SLOT_AVAILABLE);

//	printk(KERN_INFO "Get_data request received for[%d bytes] from  buffer id[%d]\n",numbytes,bufid);

	if (rings[bufid].op_ok != OP_OK){ // #define
//		printk(KERN_INFO "With Out Pull operation Doing Get Operation\n");
		spin_unlock(&rings[bufid].b_lock);
		return E_OP_MISMATCH;
	}

	if(rings[bufid].state == BUF_ROOM_EMPTY){
		spin_unlock(&rings[bufid].b_lock);
		return EBUF_EMPTY;
	}

	if(numbytes <= rings[bufid].bytes_used)
	{	
//		printk(KERN_INFO "it is in normal copy\n");

		if(((rings[bufid].tail + 1 - rings[bufid].read) >= numbytes) || (rings[bufid].read + 1 > rings[bufid].tail)) {
			if((BYTES_IN_BUFFER - ((rings[bufid].read +1)%BYTES_IN_BUFFER)) >= numbytes)
				memcpy(get_buf,
						(char *)(rings[bufid].ring_buf + ((rings[bufid].read + 1)%BYTES_IN_BUFFER)),
						numbytes);
		}

		else{
			memcpy(get_buf,
					(char *)(rings[bufid].ring_buf + ((rings[bufid].read + 1)%BYTES_IN_BUFFER)),
					BYTES_IN_BUFFER - rings[bufid].read - 1);

			memcpy((char *)(get_buf + BYTES_IN_BUFFER - rings[bufid].read - 1),
					&rings[bufid].ring_buf,
					(numbytes - (BYTES_IN_BUFFER - rings[bufid].read - 1)));
		}

		rings[bufid].read  = (rings[bufid].read + numbytes)%BYTES_IN_BUFFER ;
		rings[bufid].op_ok = OP_NOT_OK;
		spin_unlock(&rings[bufid].b_lock);
		return numbytes;
	}
	else{
//		printk(KERN_INFO "Requested more data\n");

		if(rings[bufid].read > rings[bufid].tail){
			memcpy(get_buf,
					(char *)(rings[bufid].ring_buf + ((rings[bufid].read + 1)%BYTES_IN_BUFFER)),
					BYTES_IN_BUFFER - rings[bufid].read - 1);

			memcpy((char *)(get_buf + BYTES_IN_BUFFER - rings[bufid].read - 1),
					&rings[bufid].ring_buf,
					rings[bufid].tail + 1);
		}else
			memcpy(get_buf,
					(char *)(rings[bufid].ring_buf + ((rings[bufid].read + 1)%BYTES_IN_BUFFER)),
					rings[bufid].tail - rings[bufid].read);

		rings[bufid].read  = rings[bufid].tail ;
		rings[bufid].op_ok = OP_NOT_OK;
		spin_unlock(&rings[bufid].b_lock);
		return rings[bufid].bytes_used;
	}
	//printk("i am inside ring buf\n");
	 //for(i=0;i<100;i++) printk("%c ",rings[bufid].ring_buf[i]);
}

int pull(unsigned int bufid, signed int numbytes)
{
	int lastget;

//	printk(KERN_INFO "Pull Function called\n");
	spin_lock(&rings[bufid].b_lock);
	assert(rings[bufid].bufid != SLOT_AVAILABLE);

	if(rings[bufid].op_ok != OP_NOT_OK)
	{	
//		printk(KERN_INFO "With Out Get operation Doing Pull Operation\n");
		spin_unlock(&rings[bufid].b_lock);
		return E_OP_MISMATCH;
	}

//	printk(KERN_INFO "head = %d\t read = %d\n",rings[bufid].head, rings[bufid].read);

	if (rings[bufid].head < rings[bufid].read)
		lastget = rings[bufid].read - rings[bufid].head;
	else
		lastget = (BYTES_IN_BUFFER - rings[bufid].head) + rings[bufid].read;

//	printk(KERN_INFO "numbytes = %d\t lastget = %d\n",numbytes,lastget);

	if (numbytes > lastget ){ 
		spin_unlock(&rings[bufid].b_lock);
		return ERR_PULL_MORE_THAN_GET;
	}

	rings[bufid].head =  (rings[bufid].head +  numbytes) %BYTES_IN_BUFFER;
	rings[bufid].read = rings[bufid].head;
	rings[bufid].bytes_used -= numbytes;
	rings[bufid].bytes_free  += numbytes;
	state_change(bufid);
	rings[bufid].op_ok = OP_OK;
	spin_unlock(&rings[bufid].b_lock);
	return 0;
}


/*
 * get_ring_buf_tail:: argument is ring buffer ID
 *
 * This function just returns the current ring buffer tail
 *
 */
int get_ring_buf_tail(int bufid)
{
	return rings[bufid].tail;
}

/*
 * prepend_to_ring_buf:: first argument is ring buffer ID, second one is data buffer to prepend,
 *			 third one is ring buffer tail, fourth one is length of data buffer,
 *			 fifth one is new data length i.e, new_frame_len - tp_hdr
 *
 * This function is to prepend data to ring buffer.
 *
 */
void prepend_to_ring_buf(int bufid, unsigned char *buf, int ring_tail, int len, int new_data_len)
{
	int i;
//	printk("Inside prepend_to_ring_buf: tail is %d, bufid is %d\n",ring_tail, bufid);
	for(i=0;i<len;i++) printk("%02x ",buf[i]);
	printk("\n");
	//memcpy((unsigned char *)(rings[bufid].ring_buf - ring_tail+1), buf, len);
	for(i=len-1;i>=0;i--) rings[bufid].ring_buf[ring_tail - new_data_len - i] = *buf++;
}
int bytes_used(int bufid)
{
//spin_lock(&rings[bufid].b_lock);
return( rings[bufid].bytes_used);
//spin_unlock(&rings[bufid].b_lock);
}

EXPORT_SYMBOL(post_data);
EXPORT_SYMBOL(get_data);
EXPORT_SYMBOL(pull);
EXPORT_SYMBOL(ring_buf_init);
EXPORT_SYMBOL(uninit);
EXPORT_SYMBOL(ring_buf_free_count);
EXPORT_SYMBOL(state_change);
EXPORT_SYMBOL(print_rbuf);
EXPORT_SYMBOL(printall_rbufs);

EXPORT_SYMBOL(lock);
EXPORT_SYMBOL(unlock);
EXPORT_SYMBOL(get_ring_buf_tail);
EXPORT_SYMBOL(prepend_to_ring_buf);
EXPORT_SYMBOL(bufid);
EXPORT_SYMBOL(post_task);
EXPORT_SYMBOL(pull_task);
EXPORT_SYMBOL(bytes_used);


