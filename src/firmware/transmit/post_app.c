#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include "app.h"

#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)

#define NO_OF_BYTES_TO_POST 1500
#define NO_OF_BYTES_TO_PULL 1500

int main()
{
	int fd1,fd2,i,z=0;
	unsigned int ret;
	unsigned char msg[NO_OF_BYTES_TO_POST];
	FILE *fp1,*fp2;
	struct post_st post_data;
	struct pull_st pull_data;
	unsigned int cnt;
	post_data.type=Tx;
	post_data.len=NO_OF_BYTES_TO_POST;
	post_data.buf = (unsigned char *)malloc(post_data.len);
        if(post_data.buf==NULL)
         printf("allocation failed\n");
        
        bzero(post_data.buf, NO_OF_BYTES_TO_POST);
	fp1 = fopen("datafile.bin","rb");
//		fp1 = fopen("file.bin","rb");
	if(fp1==NULL){printf("error in opening file");return -1;}
	do
             {
		ret=fread(post_data.buf,1,1000,fp1);
                  
                    if(ret>0)
                    {
                        z+=ret;
                        post_data.len=ret;
                        fd1 = open("/dev/my_dev", O_RDWR);
                        if(fd1<0){ printf("\n post open fails\n"); return 1;}
                        if(ioctl(fd1, WRITE_IOCTL, &post_data)<0) {
                                printf("post ioctl fail\n");
                                close(fd1);
                                return 0;
                        }

                    }
             }while(ret>0);
                 printf("size %d\n",z);

         printf("post ioctl executed\n");
	close(fd1);
	fclose(fp1);

	//	fd2 = open("/dev/my_cdev", O_RDWR);
	//	fp2 = fopen("output.bin","wb");
	//	for(i=0;i<1000;i++){	
	//	if(ioctl(fd2, READ_IOCTL,&pull_data )<0) {
	//			printf("pull ioctl fail\n");
	//			close(fd2);return 0;}
	//	if(fp2 == NULL) return -1;
	//		fwrite(pull_data.buf,100,1,fp2);}
	//		printf("pull ioctl executed\n");
	//		close(fd2);
	//		fclose(fp2);

	//printf("----------------------------------------------\n");
	//system("cksum input.bin");
	//system("cksum output.bin");



	/*	fd = open("/dev/my_dev", O_RDWR);
		if(fd<0){ printf("post open fails\n"); return 1;}
		if(ioctl(fd, WRITE_IOCTL, msg)<0) {
		printf("post ioctl fail\n");
		close(fd);
		return 0;
		}
		printf("post ioctl executed\n");
		close(fd);*/

	return 0;
}
