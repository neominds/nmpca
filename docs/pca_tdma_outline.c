#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>


#define  PCA_TDMA_MAX_SLOTS 7
#define  GUARD_TIME_INMS 10


struct tdma_slot_entry
{
       uint16 time;
       uint16 action;
};


uint32  twheel_counter=0;
uint16 current_slots_index = 0;
uint16 curr_slot_gcd=100;
uint8 rx,tx,exit_flag;
static struct hrtimer hr_timer;
static ktime_t kt_periode;
 
/*******************************************
actions:-
1)rx_start
2)rx_stop
3)tx_start
4)tx_stop
 
priority from actions will be performed from right to left i.e  from 0th bit to 7th bit..
 
bit map for above actions
  7   6   5   4   3   2   1   0
|   |   |   |   |   |   |   |   |
  0   0   0   0   0   0   0   1   ----->  rx_stop          
  0   0   0   0   0   0   1   0   ----->  tx_stop          
  0   0   0   0   0   1   0   0   ----->  rx_start          
  0   0   0   0   1   0   0   0   ----->  tx_start 
        
  0   0   0   0   0   1   0   1   ----->  stop recieve(rx_stop) and start transmitting(tx_start)          
  0   0   0   0   1   0   1   0   ----->  stop transmit(tx_stop) and start recieving(rx_start)          
 
********************************************/
 
#define TDMA_ACTION_RX_STOP       0x01
#define TDMA_ACTION_TX_STOP       0x02
#define TDMA_ACTION_RX_START      0x04
#define TDMA_ACTION_TX_START      0x08
 
#define TDMA_ACTION_RX_STOP_TXSTART      0x05
#define TDMA_ACTION_TX_STOP_RXSTART      0x0A
 
struct tdma_slot_entry  next_slots[PCA_TDMA_MAX_SLOTS];

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


void pca_tdma_init(void )
{
	// initilizes tdma module and if any other values(for example, ringbuf);
 	pca_tdma_function_wheel();
}

void pca_tdma_create_timer(void)
{
	//this function will create timer 
	//register callback function.

}


int pca_tdma_set_timer(unsigned long interval)
{

	//this function will set timer.
	//return success or fail
}

void pca_tdma_start_timer(void )
{
	//this function will start timer.

	pca_tdma_function_wheel();
}

int  pca_calc_timerwheel_interval()
{

//calculate timer wheel interval (gcd );
//returns the gcd valude of slot array.

}

void pca_tdma_function_wheel(void)
{

	pca_tdma_create_timer();
	
	pca_tdma_get_slot_gcd();

 	current_lcd =  pca_calc_timerwheel_interval();
	pca_tdma_set_timer(200000000); // 200ms , this timer will give nanosec granularity.


	//function will start timer.
	pca_tdma_start_timer();

	while(1)
	{
		if(rx)
		{	
			pca_tdma_recv(buf,len);
			pca_tdma_post(buf,len);
		
		}

		if(tx)
		{
			pca_tdma_get(buf,len);
			pca_tdma_send(buf,len);
			pca_tdma_pull(len);
		}


	}

}



pca_tdma_timer_event_handler()
{
        if (++twheel_counter == current_slots[current_slots_index].time)
        {//to check the arry if timer value matches.
                switch(current_slots[current_slots_index].action)
                {

                        case 0x01:
                                rx=0;
                                break;
                        case 0x02:
                                tx=0;
                                break;
                        case 0x04:
                                rx=1;
                                break;
                        case 0x08:
                                tx=1;
                                break;
                        case 0x05:
                                rx=0;
                                tx=1;
                                break;
                        case 0x0A:
                                tx=0;
                                rx=1;
                                break;
                        default:

                }
                current_slots_index = (current_slots_index +1) % PCA_TDMA_MAX_SLOTS ;

                if (current_slots_index == 0)
                {
                        twheel_counter = 0;
                }
        }
        return;

}


int pca_tdma_recv(unsigned char *buf,int len)
{
// call serial driver recv function

//returns no.of bytes recvd.
}

int pca_tdma_post(unsigned char *buf,int len)
{
//post data to the tx ring buffer...

}

int pca_tdma_pull(int len)
{
//pull data from the ring buffer...

}

int pca_tdma_send(unsigned char *buf,int len)
{
//call serial driver send function

//returns no.of bytes sent.
}
