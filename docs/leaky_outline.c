/* global variable defined in leaky.h header are
LEAK_SIZE for determinig size of leak
ERR_NO_ENOU_DAT to return if the no of bytes is less than LEAK_SIZE
*/




int put_into_bucket(unsigned int bucket_id,unsigned char *buff,unsigned int size)
{
 //put data int ring buffer by calling post_data()
}




int leak_bucket(unsigned int bucket_id,unsigned char *buff)
{
	//get fixed data as defined in leaky.h header (LEAK_SIZE) from ring buffer using get_data()
}

int leak_bucket_init(void)
{
  //initialize leaky bucket by calling ring_buf_init()
}



