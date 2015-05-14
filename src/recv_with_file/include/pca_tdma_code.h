
#define RX_START 0
#define RX_STOP 1
#define TX_START 2
#define TX_STOP 3

#define INT_BUF_MAX_SIZE 10000

#define RX 1
#define TX 2

#define SUCCESS -1
#define FAIL -2

#define  PCA_TDMA_MAX_SLOTS 10
#define  GUARD_TIME 10
#define SIZE 1024
#define uint8 unsigned char
#define int8   char 
#define uint16   unsigned short int  
#define int16  short int 
#define uint32  unsigned int   
#define int32  int 


struct internal_buffer
{
	char buf[INT_BUF_MAX_SIZE];
	int  head;
	int  tail;
	int  used;
	int  free;
};


enum hrtimer_restart pca_tdma_timer_event_handler(struct hrtimer *timer);
void tdma_event_callback(char action);
int post_event_queue(int action);
int read_event_queue(void);
void pca_tdma_main_loop(void ); // this is the thread function..
void pca_tdma_create_timer(void);
int pca_tdma_set_timer(unsigned long interval);
void pca_tdma_start_timer(void );
int pca_tdma_recv(unsigned char *buf,int len);
int pca_tdma_post(char bufid, unsigned char *buf,int len);
int pca_tdma_get(char bufid, unsigned char *buf,int len);
int pca_tdma_pull(char bufid,int len);
int pca_tdma_send(unsigned char *buf,int len);
//void pca_tdma_init(void );
int pca_tdma_internal_post(int bufid,unsigned char *buf,int len);
int pca_tdma_internal_get(int bufid,unsigned char *buf,int len);
int pca_tdma_internal_pull(int bufid,int len);
static int __init pca_tdma_init(void);
static void __exit pca_tdma_exit(void);

struct tdma_slot_entry
{
        uint16 time;
        uint16 action;
};


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

