
#ifndef RING_BUF_H
#define RING_BUF_H

#include <linux/spinlock.h>

//#define KTHREADS
#define NO_OF_BYTES_TO_POST 10000000
#define NO_OF_BYTES_TO_PULL 10000000
#define RING_BUFFER_SLOTS	6
//#define BYTES_IN_BUFFER	1230670	
#define BYTES_IN_BUFFER		10000
#define BUF_ROOM_CONT_OK	1
#define BUF_ROOM_NEED_WRAP	2
#define NO_ROOM			3
#define SLOT_AVAILABLE		-1
#define EBUF_NOTFREE		-10	
#define EMORE_DATA	        -11  
#define EBUF_EMPTY   	        -12 
#define ERR_PULL_MORE_THAN_GET  -13
#define E_NO_RING_FREE		-14
#define E_OP_MISMATCH		-15
#define E_THREAD		-16
#define BUF_ROOM_FULL		13 
#define BUF_ROOM_EMPTY 		14
#define BUF_ROOM_NORMAL 	15
#define OP_OK			1
#define OP_NOT_OK		0

#define assert( X ) if ( !(X) ) {\
	printk(KERN_INFO "Error: %s, %d\n", __FILE__, __LINE__); \
	return 1;}

struct data_ring_buff
{
unsigned int bufid;
unsigned int bytes_used;
unsigned int bytes_free;
unsigned int op_ok; 
unsigned char ring_buf[BYTES_IN_BUFFER];
unsigned int state; //EMPTY, FULL, NORMAL
signed int tail;
signed int head;
signed int read;
//pthread_mutex_t b_lock;
spinlock_t b_lock;
};

spinlock_t p_lock;

int post_data(unsigned int ,unsigned char *,unsigned int );
int get_data(unsigned int, unsigned int,char *);
int pull(unsigned int , signed int);
int ring_buf_init(void);
int uninit(unsigned int bufid);
inline int ring_buf_free_count(unsigned int);
void state_change(unsigned int);
void print_rbuf(unsigned int);
void printall_rbufs(void);

void lock(unsigned int bufid);
void unlock(unsigned int bufid);

int post_data_to_ring_buffer(void *buff);
int pull_data_from_ring_buffer(void *buff);

int get_ring_buf_tail(int bufid);
void prepend_to_ring_buf(int bufid, unsigned char *buf, int ring_tail, int len, int new_data_len);
int bytes_used(int);  
#endif
