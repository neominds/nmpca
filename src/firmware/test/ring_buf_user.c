#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ring_buf_user.h"
#include <pthread.h>

unsigned char init_done = 0;

struct data_ring_buff rings[RING_BUFFER_SLOTS];
int ring_buf_init()
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
	if(pthread_mutex_init(&rings[l_bufid].b_lock, NULL)) 
		return E_THREAD;
	memset(rings[l_bufid].ring_buf,0,BYTES_IN_BUFFER);
	rings[l_bufid].tail = -1;
	rings[l_bufid].head = -1;
	rings[l_bufid].read = -1;
	rings[l_bufid].bytes_free = BYTES_IN_BUFFER;
	rings[l_bufid].bytes_used = 0;
	rings[l_bufid].op_ok = OP_OK;
	return l_bufid;
}

inline int bytes_in_ring(unsigned int bufid){
//        pthread_mutex_lock(&rings[bufid].b_lock);
        return rings[bufid].bytes_used;
//        pthread_mutex_unlock(&rings[bufid].b_lock);
}


inline int ring_buf_free_count(unsigned int bufid){
	return rings[bufid].bytes_free;
}


int uninit(unsigned int bufid)  
{
	if(pthread_mutex_destroy(&rings[bufid].b_lock)) 
		return E_THREAD;
	rings[bufid].bufid = SLOT_AVAILABLE;
}


void state_change(unsigned int bufid)
{
	//printf("Space Avaliable is :%d\n",rings[bufid].bytes_free );
//	printf("Number of Bytes used in Buffer Now: %d\n",rings[bufid].bytes_used);
	if(rings[bufid].bytes_free == 0)
		rings[bufid].state = BUF_ROOM_FULL;
	else if(rings[bufid].bytes_free == BYTES_IN_BUFFER)
		rings[bufid].state = BUF_ROOM_EMPTY;
	else
		rings[bufid].state = BUF_ROOM_NORMAL;
}

void print_rbuf(unsigned int bufid)
{
#if 0
	int i;
	printf("bufid = %d]\n",bufid);
	printf("head = %d]\n", rings[bufid].head);
	printf("tail = %d]\n", rings[bufid].tail);
	printf("read = %d]\n", rings[bufid].read);
	printf("rings[l_bufid].bytes_free = %d\n",rings[bufid].bytes_free);
	printf("rings[l_bufid].bytes_used = %d\n",rings[bufid].bytes_used);
	printf("BUF_STATE :\t ");
	switch(rings[bufid].state)
	{	
		case BUF_ROOM_FULL:
			printf("FULL\n");
			break;
		case BUF_ROOM_EMPTY:
			printf("EMPTY\n");
			break;
		case BUF_ROOM_NORMAL:
			printf("NORMAL\n");
			break;
	}
	for (i=0; i< BYTES_IN_BUFFER; i++) 
		printf("%02x ",rings[bufid].ring_buf[i]);
	printf("\n");
#endif
}


void printall_rbufs()
{
#if 0
	unsigned int i;
	for(i=0;i<RING_BUFFER_SLOTS;i++)
	{
		printf("--------------------------\n");
		print_rbuf(i);
	}
	printf("\n");
#endif
}



int post_data(unsigned int bufid, unsigned char *buf, unsigned int nbytes)
{
//	printf("Post_data request received for [%d bytes] to buffer id[%d]\n",nbytes,bufid);
	pthread_mutex_lock(&rings[bufid].b_lock);
	assert(rings[bufid].bufid != SLOT_AVAILABLE);
	//printf("bytes free %d\t n = %d\t",rings[bufid].bytes_free,nbytes);	
	if(nbytes <= 0)
	return 0;
	if (rings[bufid].bytes_free < nbytes)
	{
//		printf("Now Copying Only[ %d ] Bytes \t Remaining Need To Copy is [ %d ] \n",rings[bufid].bytes_free,(nbytes - rings[bufid].bytes_free));
		nbytes = rings[bufid].bytes_free;
	}
	if ( ( rings[bufid].tail + nbytes + 1) > BYTES_IN_BUFFER)
	{
		memcpy((char *)(rings[bufid].ring_buf + rings[bufid].tail + 1),
				buf,
				(BYTES_IN_BUFFER - (rings[bufid].tail +1)));
		memcpy(rings[bufid].ring_buf,
				buf+(BYTES_IN_BUFFER - (rings[bufid].tail + 1)),
				nbytes-(BYTES_IN_BUFFER - (rings[bufid].tail + 1)));
	}
	else
		memcpy((char *)(rings[bufid].ring_buf + rings[bufid].tail + 1),
				buf,
				nbytes);
	rings[bufid].tail = (rings[bufid].tail + nbytes) % BYTES_IN_BUFFER;
	rings[bufid].bytes_used += nbytes;
	rings[bufid].bytes_free  -= nbytes;
	state_change(bufid);  //maybe not needed ??
	pthread_mutex_unlock(&rings[bufid].b_lock);
	return nbytes;
}


int get_data(unsigned int bufid, unsigned int numbytes,char *get_buf)
{
	pthread_mutex_lock(&rings[bufid].b_lock);
	assert(rings[bufid].bufid != SLOT_AVAILABLE);
//	printf("Get_data request received for[%d bytes] from  buffer id[%d]\n",numbytes,bufid);
	if (rings[bufid].op_ok != OP_OK){ // #define
		//printf("With Out Pull operation Doing Get Operation\n");
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return E_OP_MISMATCH;
	}
	if(rings[bufid].state == BUF_ROOM_EMPTY){
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return EBUF_EMPTY;
	}
	if(numbytes <= rings[bufid].bytes_used)
	{	
//		printf("it is in normal copy\n");
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
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return numbytes;
	}
	else{
//		printf("Requested more data\n");
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
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return rings[bufid].bytes_used;
	}	
}



int pull(unsigned int bufid, signed int numbytes)
{
	int lastget;
//	printf("Pull Function called\n");
	pthread_mutex_lock(&rings[bufid].b_lock);
	assert(rings[bufid].bufid != SLOT_AVAILABLE);
	if(rings[bufid].op_ok != OP_NOT_OK)
	{	
//		printf("With Out Get operation Doing Pull Operation\n");
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return E_OP_MISMATCH;
	}
//	printf("head = %d\t read = %d\n",rings[bufid].head, rings[bufid].read);
	if (rings[bufid].head < rings[bufid].read)
		lastget = rings[bufid].read - rings[bufid].head;
	else
		lastget = (BYTES_IN_BUFFER - rings[bufid].head) + rings[bufid].read;
//	printf("numbytes = %d\t lastget = %d\n",numbytes,lastget);
	if (numbytes > lastget ){ 
		pthread_mutex_unlock(&rings[bufid].b_lock);
		return ERR_PULL_MORE_THAN_GET;
	}
	rings[bufid].head =  (rings[bufid].head +  numbytes) %BYTES_IN_BUFFER;
	rings[bufid].read = rings[bufid].head;
	rings[bufid].bytes_used -= numbytes;
	rings[bufid].bytes_free  += numbytes;
	state_change(bufid);
	rings[bufid].op_ok = OP_OK;
	pthread_mutex_unlock(&rings[bufid].b_lock);
	return 0;
}

