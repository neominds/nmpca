

void pca_ic_init(void )
{

//initializes pca_ic modules and ring buffer and if any other.

}


void pca_ic_rx_callback(void)
{

// this function will be called after tdma posts the data to the ring buffer.
//when this function called it will post the event to the event queue;

}

void pca_ic_function_wheel()
{
	while(1)
	{
		if(tx)
		{	
			ret = pca_ic_recv(buf,len);		

			pca_ic_post(buf,ret);

		}
		if(rx)
		{
			//it will check for event queue. 	
			if(event_q == EMPTY)  continue;

			else 
				pca_ic_event_process();	
		}
	}
}

pca_ic_event_process()
{
//pick up an event from event queue.
ret = pca_ic_get(buf,len);
pca_ic_send(buf,len);
pca_ic_pull(len);

}

int pca_ic_get(unsigned char *buf,int len)
{

//call ring_buf_get 
// returns no.of bytes if SUCCESS. -1 if fail.

}

int pca_ic_send(unsigned char *buf,int len)
{
//call driver send function
//return no.pf bytes sent if success. -1 if fail.

}

pca_ic_pull(int len)
{
//call ring_buf_pull.
}


int pca_ic_post(unsigned char *buf,int len)
{
//posts data to ring buf
//returns no.of bytes posted.
}
