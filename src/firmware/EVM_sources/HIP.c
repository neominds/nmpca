﻿#pragma pack(1)
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
#include <linux/uart_driver_x86.h>
#include <linux/jiffies.h>
//#include "HIP.h"
#include <linux/delay.h>
#include <linux/HIP.h>
/* HIP Outline: */

//#include<stdio.h>
//#include<string.h>

int Tx_bufid;
int tx_bufid;
int txbufid;
int rxbufid;
spinlock_t b_lock;
struct task_struct *tp_tx_task;
struct task_struct *tp_rx_task;
struct task_struct *ic_task;

struct task_struct *voc_task;


/* TP Outline: */
spinlock_t v_lock;
struct evt_st ic_evt_st[SIZE_OF_EVT_BUF];
struct evt_st voc_evt_st[SIZE_OF_EVT_BUF];
struct evt_st tp_tx_evt_st[SIZE_OF_EVT_BUF];

char ic_event_buf[SIZE_OF_EVT_BUF];
char voc_event_buf[SIZE_OF_EVT_BUF];
char tp_tx_event_buf[SIZE_OF_EVT_BUF];


/* 
 *new_frame_event::	first argument is an EVENT, 
 *			second one is the ring buffer tail and
 *			third one is frame length i.e, IC header + TP header + data length
 *
 * This function fills the event structure with required information and
 * post event to corresponding event buffer and wakes up require kernel thread
 * 
 * if EVENT is	VOC_NEWFRAME_EVENT then function post event to voc_event_buf and wakes up 'tp tx task'
 *		DATA_NEWFRAME_EVENT then it post event to data_event_buf and wakes up 'tp tx task'
 * 		TP_TX_NEWFRAME_EVENT then it post event to tp_tx_event_buf and wakes up 'ic task'
 *		IC_NEWFRAME_EVENT then it post event to ic_event_buf and wakes up 'tp rx task'
 *
 */
int nv,nd,ntt,ntr,ni;
int tx_flag,ic_flag;
int cnt_voc,cnt_tx,cnt_ic;
void new_frame_event(enum EVENT evt, unsigned int frame_end, unsigned short new_frame_len)
{
	
 //        flag=1;
	///printk(KERN_ALERT "Inside new_frame_event function\n");
       spin_lock(&b_lock);
	switch(evt)
	{
		case VOC_NEWFRAME_EVENT:	
                                                
                                                if(nv==SIZE_OF_EVT_BUF) nv=0;
						voc_evt_st[nv].frame_end = frame_end;
						voc_evt_st[nv].new_frame_len = new_frame_len;
						voc_event_buf[nv] = evt;
						nv++;
                                               // printk("voc index:%d frame_end:%d slot count: %d\n",nv-1,frame_end,cnt_voc);
						spin_unlock(&b_lock);
						wake_up_process(tp_tx_task);
   //                                             flag=0;                             
						break;

		case DATA_NEWFRAME_EVENT:	wake_up_process(tp_tx_task);
						break;

		case TP_TX_NEWFRAME_EVENT://	spin_lock(&b_lock);
                                                
                                                if(ntt==SIZE_OF_EVT_BUF) ntt=0;
						tp_tx_evt_st[ntt].frame_end = frame_end;
						tp_tx_evt_st[ntt].new_frame_len = new_frame_len;
						tp_tx_event_buf[ntt] = evt;
						ntt++;
                                               // printk("tx index:%d frame_end:%d slot count:%d\n",ntt-1,frame_end,cnt_tx);
						spin_unlock(&b_lock);
						wake_up_process(ic_task);
     //                                           flag=0;
						break;

		case IC_NEWFRAME_EVENT:		if(ni==SIZE_OF_EVT_BUF) ni=0;
						ic_evt_st[ni].frame_end = frame_end;
						ic_evt_st[ni].new_frame_len = new_frame_len;
						ic_event_buf[ni] = evt;
						ni++;
                                               // printk("ic index:%d frame_end:%d\n",ni,frame_end);
						spin_unlock(&b_lock);
						wake_up_process(tp_rx_task);
						break;

		/*case TP_RX_NEWFRAME_EVENT:	wake_up_process();
							break; */
	}
}

/*
 * voq_post::	first argument is data buffer to post data to ring buffer and
 *		second one is length of data
 *
 * This function post data to ring buffer and returns the length of data posted
 *
 */
int voq_post(unsigned char *buf, int new_frame_len)
{
	return post_data(txbufid, buf, new_frame_len);
}

/*
//schedule can be delayed using : schedule_delayed_work(&work, delay);
//where delay is mentioned in the timer ticks (jiffies)
struct work_struct work;
DECLARE_WORK(work, read_data_from_vocoder);
INIT_WORK(struct work_struct *work, read_data_from_vocoder);
schedule_work(&work);
*/


/*
 * read_data_from_vocoder ::
 *
 * This function reads data from vocoder and stores in local buffer,
 * and when ever the 'i' value becomes NO_CHUNKS_PER_PKT then it post whole
 * local buffer data to ring buffer and post event to voc_event_buf.
 *
 * Suppose NO_CHUNKS_PER_PKT is 5, then for every 5 reads from vocoder it post data 
 * to ring buffer and post event to voc_event_buf
 *
 */
int k=0;
int tot=0;
int read_data_from_vocoder(void *data)
{
	static int i=0, new_frame_len=5;
	int ret, frame_end,r;
	unsigned char buf[200];
	int j, nlen=15;
        int flag=1,num;
        unsigned long interval;
        
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
	while(!kthread_should_stop())
        {

               set_current_state(TASK_INTERRUPTIBLE);
		if(bytes_used(tx_bufid)>=39)
		{
                        flag=1;
                        ret= get_data(tx_bufid,39,buf+5);	
			if(ret>0)
				{
                                   pull(tx_bufid,ret);
                                       tot+=ret;
                                   r = voq_post(buf, ret+5);
                                }
			if(r == 0) {
				printk(KERN_ALERT "voq_post:: Failed to post data to ring buffer\n");
				continue;

			}
                        else if(r>0)
                         cnt_voc++;

			frame_end = get_ring_buf_tail(txbufid);

			new_frame_event(VOC_NEWFRAME_EVENT, frame_end, r);
                        msleep_interruptible(5);
                 }
               else if (bytes_used(tx_bufid)>0)
                  {
                     if(flag==1)
                     {
                       interval =jiffies;
                       flag=0;
                     }
                     else if(time_after(jiffies,interval+50))
                     {
                         num=bytes_used(tx_bufid); 
                        ret= get_data(tx_bufid,num,buf+5);	
			if(ret>0)
				{
                                   pull(tx_bufid,ret);
                                       tot+=ret;
                                   r = voq_post(buf, ret+5);
                                }
			if(r == 0) {
				printk(KERN_ALERT "voq_post:: Failed to post data to ring buffer\n");
				continue;

			}
                        else if(r>0)
                         cnt_voc++;

			frame_end = get_ring_buf_tail(txbufid);
			new_frame_event(VOC_NEWFRAME_EVENT, frame_end, r);
                       

                     }
                     else
                      msleep_interruptible(5);

                  }
             else 
                {
                   schedule();
                   __set_current_state(TASK_RUNNING);
                }
                      
          }      
	return(0);                    


}  



/*
 * reset_event:: first argument is index of event buffer
 *		 second one is EVENT
 *
 * This function just reset the corresponding event buffer to EVENT
 *
 */
void reset_event(int index, enum EVENT evt)
{
	spin_lock(&b_lock);
	switch(evt)
	{
		case VOC_NEWFRAME_EVENT:	voc_event_buf[index] = '\0';
						break;

		case DATA_NEWFRAME_EVENT:	break;

		case TP_TX_NEWFRAME_EVENT:	tp_tx_event_buf[index] ='\0';
						break;

		case IC_NEWFRAME_EVENT:		ic_event_buf[index] ='\0';
						break;

	/*	case TP_RX_NEWFRAME_EVENT:	break; */
	}
	spin_unlock(&b_lock);
}

unsigned int seqno;

/*
 * prepend_tp_header_to_data_in_ring_buf::
 * 
 * arguments:	first one msgtype i.e., VOICE/DATA, second one source ID
 *		third one destination bitmap, fourth one data len and
 *		fifth one ring buffer tail
 *
 * This function is to create VOICE/DATA header and prepend that header
 * to data which was posted by read_data_from_vocoder function in ring buffer
 *
 */
void prepend_tp_header_to_data_in_ring_buf(char msgtype, char srcid, unsigned short dstid_bm, unsigned short len, int frame_end)
{                  
                 int i;
           	if(msgtype == VOICE) {
		struct tp_voice_hdr vhdr;
		unsigned char hdr[4];

		vhdr.msgtype = msgtype;
		vhdr.srcid = srcid;
		vhdr.seqno = seqno;
		vhdr.len = len;

		unsigned char *dummy = (unsigned char *)&vhdr;
   //             printk("vhdr len %d\n",vhdr.len);

		//printk("prepend_tp_header_to_data_in_ring_buf::%02x %02x %02x %02x\n",vhdr.msgtype, vhdr.srcid, vhdr.seqno, vhdr.len);
		//printk("prepend_tp_header_to_data_in_ring_buf::%02x %02x %02x %02x\n",
				//	*((unsigned char *)&vhdr), *(((unsigned char *)&vhdr)+1),
				//	*(((unsigned char *)&vhdr)+2), *(((unsigned char *)&vhdr)+3));
		//printk("prepend_tp_header_to_data_in_ring_buf::%d %d %d %d\n",
				//	*((unsigned char *)&vhdr), *(((unsigned char *)&vhdr)+1),
				//	*(((unsigned char *)&vhdr)+2), *(((unsigned char *)&vhdr)+3));
		//printk("prepend_tp_header_to_data_in_ring_buf::%d %d %d %d\n",
				//	*dummy, *(dummy+1), *(dummy+2), *(dummy+3));
		memcpy(hdr, (unsigned char *)&vhdr, 4);
//		printk("prepend_tp_header_to_data_in_ring_buf::%02x %02x %02x %02x\n",
//					hdr[0], hdr[1], hdr[2], hdr[3]);
		seqno++;
		if(seqno%4095 == 0) {
			seqno=0;
//			printk("prepend_tp_header_to_data_in_ring_buf:: inside if\n");
		}

		//printk("prepend_tp_header_to_data_in_ring_buf:: frame_end %d\n",frame_end);
		prepend_to_ring_buf(txbufid, hdr, frame_end, 4, len);
   
	} else if(msgtype == DATA) {
	}
}

int tpt, tpr, ict;


/*
 * check_event:: argument is an EVENT to check
 *
 * This function is to check an EVENT in corresponding EVENT buffer
 * and returns the buffer index
 *
 */
int check_event(enum EVENT evt)
{
	int ret;
	spin_lock(&b_lock);

	//printk(KERN_ALERT "Inside check_event function\n");

	switch(evt)
	{
		case VOC_NEWFRAME_EVENT:	while(voc_event_buf[tpt]!=VOC_NEWFRAME_EVENT)
						{
							tpt++;
							if(tpt==SIZE_OF_EVT_BUF) {
							//	printk(KERN_INFO "check_event:: Vocoder Event Buffer is Empty\n");
								break;
							}
						}
						ret = tpt;
						tpt++;
						if(tpt >= SIZE_OF_EVT_BUF) tpt=0;
						break;

		case DATA_NEWFRAME_EVENT:	break;

		case TP_TX_NEWFRAME_EVENT:	while(tp_tx_event_buf[ict]!=TP_TX_NEWFRAME_EVENT)
						{
							ict++;
							if(ict==SIZE_OF_EVT_BUF) {
							//	printk(KERN_INFO "check_event:: TP Tx Event Buffer is Empty\n");
								break;
							}
						}
						ret = ict;
						ict++;
						if(ict >= SIZE_OF_EVT_BUF) ict=0;
						break;

		case IC_NEWFRAME_EVENT:		while(ic_event_buf[tpr]!=IC_NEWFRAME_EVENT)
						{
							tpr++;
							if(tpr==SIZE_OF_EVT_BUF) {
							///	printk(KERN_INFO "check_event:: IC Event Buffer is Empty\n");
								break;
							}
						}
						ret = tpr;
						tpr++;
						if(tpr >= SIZE_OF_EVT_BUF) tpr=0;
						break;

		/*case TP_RX_NEWFRAME_EVENT:	wake_up_process();
						break; */
	}
	spin_unlock(&b_lock);
	return ret;
}


/*
 * tp_tx_evt_processing:: argument is NULL (as this is thread function we need argument as void *data)
 *
 * This function was waken up by read_data_from_vocoder function, after that this function
 * will check for 'VOC_NEWFRAME_EVENT' event in voc_event_buf, prepends voice header to 
 * ring buffer and post 'TP_TX_NEWFRAME_EVENT' event to IC thread and wakes up that IC task
 *
 /p*/
int tp_tx_evt_processing(void *data) //kernel thread
{
	int index;


         set_current_state(TASK_INTERRUPTIBLE);
          schedule();
        


	//	printk(KERN_ALERT "Inside tp_tx_evt_processing function 111\n");

	while(!kthread_should_stop()) 
	{

		//		printk(KERN_ALERT "Inside tp_tx_evt_processing function\n");

                set_current_state(TASK_INTERRUPTIBLE);
		if(cnt_tx<cnt_voc)
		{ 

			index = check_event(VOC_NEWFRAME_EVENT);

			if(index==SIZE_OF_EVT_BUF) 
			{

				printk(KERN_ALERT "tp_tx_evt_processing:: Empty buffer\n");

				continue;
			}

			reset_event(index, VOC_NEWFRAME_EVENT);

			prepend_tp_header_to_data_in_ring_buf(VOICE, 1, 9, voc_evt_st[index].new_frame_len-5, voc_evt_st[index].frame_end);
			cnt_tx++;
			new_frame_event(TP_TX_NEWFRAME_EVENT, voc_evt_st[index].frame_end, voc_evt_st[index].new_frame_len);
		}
                else
		{
                   schedule();
                   __set_current_state(TASK_RUNNING);
                }
	}
	return 0;
}


/*
 * voq_get::	first argument is buffer to get data from ring buffer and
 *		second one is length of data to get
 *
 * This function get/read data from ring buffer and returns the length of data read
 *
 */
int voq_get(unsigned char *buf, int new_frame_len)
{
	return get_data(rxbufid, new_frame_len, buf);
}


/*
 * voq_pull:: argument is length to pull from ring buffer
 *
 * This function calls pull function of ring buffer and returns the length pulled
 *
 */
int voq_pull(int new_frame_len)
{
	return pull(rxbufid, new_frame_len);
}


/*
 * get_len_from_data:: argument is the data buffer which read from ring buffer
 *
 * This function parse length field of TP header from data and returns that length
 *
 */
unsigned short get_len_from_data(unsigned char *data)
{
	unsigned short len;
	char msgtype;
	msgtype = (*(data+1)>>4);
	if(msgtype == VOICE) {
               
	struct tp_voice_hdr vhdr;
        memcpy(&vhdr,data+1,4);
        return vhdr.len;
	}
	else if(msgtype == DATA) {
		len = ((*(data+2)&0x0f) << 8);
		len |= *(data+3);
		return len;
	}
}


/*
 * tp_rx_evt_processing:: argument is NULL (as this is thread function we need argument as void *data)
 *
 * This function was waken up by ic_evt_processing thread, after that this function
 * will check for 'IC_NEWFRAME_EVENT' event in ic_event_buf and pulls data from ring buffer
 * then feeds data to vocoder in required format
 *
 */
int tp_rx_evt_processing(void *data) //kernel thread
{
	int ret, index;

	/* kernel sleep */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
        ic_flag=1;

	while(!kthread_should_stop()) {
		printk(KERN_ALERT "Inside tp_rx_evt_processing function\n");

		//spin_lock(&b_lock);

		index = check_event(IC_NEWFRAME_EVENT);
		if(index==SIZE_OF_EVT_BUF) {
			//	spin_unlock(&b_lock);
                        ic_flag=0;
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
                        ic_flag=1;
			continue;
		}
		unsigned char buf[ic_evt_st[index].new_frame_len];

		reset_event(index, IC_NEWFRAME_EVENT);
		ret = voq_get(buf, ic_evt_st[index].new_frame_len);
		if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY) {
			printk(KERN_INFO "tp_rx_evt_processing: Failed to get data from ring buffer\n");
			//	spin_unlock(&b_lock);
                        ic_flag=0;
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
                        ic_flag=1;
			continue;
			//return -1;
		}

		//signed char validate_tp_header(IN char *buf);

		unsigned short length;
		length = get_len_from_data(buf);
		//	while(length)
		//	{
		//chunk_len = int get_one_chunk_from_data(IN unsigned char *buf, INOUT unsigned char *chunk);
		//send_data_to_vocoder(....);
		//	}
		ret = voq_pull(ret);
		if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) {
			printk(KERN_INFO "tp_rx_evt_processing: Pull failed\n");
			//	spin_unlock(&b_lock);
                        ic_flag=0;
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
                        ic_flag=1;
			continue;
			//return -1;
		}
		//void voc_rx_newframe_callback(enum EVENT evt, int len);

		//spin_unlock(&b_lock);
                ic_flag=0;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
                ic_flag=1;
	}
	return 0;
}



/* IC Outline: */

/*
 * icq_get::	first argument is buffer to get data from ring buffer and
 *		second one is length of data to get
 *
 * This function get/read data from ring buffer and returns the length of data read
 *
 */
int icq_get(unsigned char *buf, int new_frame_len)
{
	return get_data(txbufid, new_frame_len, buf);
}


/*
 * icq_pull:: argument is length to pull from ring buffer
 *
 * This function calls pull function of ring buffer and returns the length pulled
 *
 */
int icq_pull(int new_frame_len)
{
	return pull(txbufid, new_frame_len);
}


/*
 * add_ic_hdr_to_data:: argument is data buffer to send out from IC module
 *
 * This function will add IC header to data buffer
 *
 */
void add_ic_hdr_to_data(unsigned char *buf)
{
#define SRCID 00
#define DSTID 01
	*buf = ((SRCID << 6) | (DSTID << 4));
}


/*
 * ic_send::	first argument is data buffer to send through PCI
 *		second one is length of that data buffer
 *
 * This function will send data through PCI Express port
 *
 */
int ic_send(unsigned char *buf, int new_frame_len)
{
	// using PCI driver send function instead of serial_send is serial driver send function
	return serial_send(buf, new_frame_len);
}


/*
 * ic_evt_processing:: argument is NULL (as this is thread function we need argument as void *data)
 *
 * This function was waken up by tp_tx_evt_processing thread, after that this function
 * will check for 'TP_TX_NEWFRAME_EVENT' event in tp_tx_event_buf, pulls data from ring buffer
 * then add IC header to that data and sends out through PCI Express
 *
 */
int ic_evt_processing(void *data) //kernel thread
{
	int ret, sret, index,ret1,i;
        unsigned char buf[200];
        //struct frames fr; // temp commentd by jacob **Confusion with subin's file and bhargav's shoud check**
        //fr.start=START_FRAME;
        //fr.stop=END_FRAME;
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
         
       while(!kthread_should_stop()) 
	{
		//		printk(KERN_ALERT "Inside ic_evt_processing function\n");


	        set_current_state(TASK_INTERRUPTIBLE);
                if(cnt_ic<cnt_tx)
		{
			index = check_event(TP_TX_NEWFRAME_EVENT);

			if(index==SIZE_OF_EVT_BUF) 
			{

                                printk(KERN_ALERT "ic_evt_processing:: Empty buffer\n");
				continue;
			}

			reset_event(index, TP_TX_NEWFRAME_EVENT);

		        ret = icq_get(buf+2, tp_tx_evt_st[index].new_frame_len);
			if(ret == E_OP_MISMATCH || ret == EBUF_EMPTY||ret < 0) 
			{
				printk(KERN_INFO "ic_evt_processing: Failed to get data from ring buffer %d\n",ret);
				continue;

			}

                        add_ic_hdr_to_data(buf+2);
               //	printk("size: %hu packet: %d\n",get_len_from_data(buf),cnt_ic);
		//	for(i=0;i<ret;i++)
		//		printk("%d\n",buf[i]);
                        
                        //memcpy(buf,&fr.start,sizeof(short int)); by jacob
                        //memcpy(buf+ret+2,&fr.stop,sizeof(short int)); by jacob
                        
			ret1=post_data(Tx_bufid,buf,ret+4);
                        cnt_ic++;
                        ret = icq_pull(ret);
			if(ret == E_OP_MISMATCH || ret == ERR_PULL_MORE_THAN_GET) 
			{
				printk(KERN_INFO "ic_evt_processing: Pull failed\n");
				continue;

			}

		}
                else
		{
                    schedule();
	            __set_current_state(TASK_RUNNING);
                    
                }
	}
	return 0;
}

/*NOTE:	Flush all the works using: void flush_scheduled_work(void);
  work queue can be canceled using: int cancel_delayed_work(struct work_struct *work); */

/*
   /vostruct work_struct icwork;
   DECLARE_WORK(icwork, ic_recv);
   INIT_WORK(struct work_struct *icwork, ic_recv);
   schedule_work(&icwork);
 */


/*
 * ic_recv:: 
 *
 * This function will read data from PCI Express, check IC header, reorder packets
 * according to sequence number then post data to ring buffer and post 'IC_NEWFRAME_EVENT'
 * event to ic_event_buf and wakes up 'tp_rx_evt_processing' thread
 *
 */
/*
int ic_recv(void *data)
{
	while(1)
	{
		int serial_recv(char  *buf, int new_frame_len);
		int check_ic_hdr(char *buf);
		int reorder_packets(char *buf, int new_frame_len);
		int icq_post(int type, const char *buf, int new_frame_len);
		void new_frame_event(IC_NEWFRAME_EVENT, ring_buf_tail, new_frame_len);
	}
}
*/

static int __init HIP_main(void)
{
	int i;
        tx_bufid=ring_buf_init();
	Tx_bufid = ring_buf_init();
	
	txbufid = ring_buf_init();

	printk(KERN_ALERT "\ntx_bufid txbufid  Tx_bufid is %d %d %d\n",tx_bufid,txbufid,Tx_bufid);


	spin_lock_init(&b_lock);
        spin_lock_init(&v_lock);
	tp_tx_task = kthread_run(&tp_tx_evt_processing, NULL, "tp_tx_evt_processing");
	tp_rx_task = kthread_run(&tp_rx_evt_processing, NULL, "tp_rx_evt_processing");
	ic_task = kthread_run(&ic_evt_processing, NULL, "ic_evt_processing");
	voc_task = kthread_run(&read_data_from_vocoder, NULL, "vocoder thread");
/*        init_timer(&my_timer);
        my_timer.expires=jiffies+1;
        my_timer.function=read_data_from_vocoder;
        my_timer.data=0;
        add_timer(&my_timer);*/

	//printk(KERN_INFO "Kernel Thread : %s post_task:%p &post_task:%p\n",post_task->comm, post_task, &post_task);
	printk(KERN_INFO "Started up HIP module.\n\n");
        return 0;
}

static void __exit HIP_cleanup(void)
{
	kthread_stop(tp_tx_task);
	kthread_stop(tp_rx_task);
	kthread_stop(ic_task);
	kthread_stop(voc_task);
       // del_timer(&my_timer);
	printk(KERN_INFO "Cleaning up HIP module.\n\n");
        
	printk(KERN_ALERT "\ntx_bufid txbufid  Tx_bufid is %d %d %d\n",tx_bufid,txbufid,Tx_bufid);
//	printk(KERN_ALERT "\nrxbufid Rx_bufid is %d \n",rxbufid,Rx_bufid);
}

EXPORT_SYMBOL(tx_bufid);
EXPORT_SYMBOL(Tx_bufid);
EXPORT_SYMBOL(txbufid);
EXPORT_SYMBOL(tp_tx_evt_processing);
EXPORT_SYMBOL(tp_rx_evt_processing);
EXPORT_SYMBOL(ic_evt_processing);
EXPORT_SYMBOL(read_data_from_vocoder);
EXPORT_SYMBOL(voc_task);
EXPORT_SYMBOL(tp_tx_task);

module_init(HIP_main);
module_exit(HIP_cleanup);

