#ifndef LEAK_H
#define LEAKY_H
#define LEAK_SIZE 20
#define ERR_NO_ENOU_DAT -7
int put_into_bucket(unsigned int bucket_id,unsigned char *buff,unsigned int size);
int leak_bucket(unsigned int bucket_id,unsigned char *buff);
int leak_bucket_init(void);
#endif
