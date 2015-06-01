#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include"app.h"
#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, char *)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, char *)
#define READ_IOCTL_SYN _IOR(MY_MACIG, 2, char *)
#define NO_OF_BYTES_TO_PULL 1000000

int main(int argc,char **argv )
{
        if(argc!=2)
        {
          printf("wrong format\n");
          return 0;
        }
        char *num;
	int fd, i,ret;
//	unsigned char msg[NO_OF_BYTES_TO_PULL];
 	FILE *fp;
        int l;     
	struct pull_st pull_data;
        num=argv[1];
        l=atoi(num);
        printf("%d\n",l);
	pull_data.type=Rx;
	pull_data.len=l;
	pull_data.buf=(unsigned char *)malloc(pull_data.len);

		fd = open("/dev/my_cdev", O_RDWR);
		if(fd<0){ printf("pull open fails\n"); return 1;}
		if((ret=ioctl(fd, READ_IOCTL_SYN,&pull_data))<0) {
			printf("pull ioctl fail\n");
			close(fd);
			return 0;
		}

/*		for(i=0;i<NO_OF_BYTES_TO_PULL;i++) {
			if(msg[i] != 0xff) {
				printf("Data is Corrupted at %d\n",i);
				close(fd);
				return -1;
			}
		}

		for(i=0;i<NO_OF_BYTES_TO_PULL;i++) {
			if(i%32 == 0) printf("\n");
			printf("%02x ",msg[i]);
		  }
              
*/               printf("return %d\n",ret);
//                  for(i=0;i<ret;i++)
//                  printf("%d %d\n",i,pull_data.buf[i]);

		fp = fopen("pull.bin","wb");
		if(fp == NULL) return -1;
		fwrite(pull_data.buf,ret,1,fp);
		fclose(fp);
		printf("pull ioctl executed\n");
		close(fd);

printf("----------------------------------------------\n");
//system("cksum post.bin");
system("cksum pull.bin");
//	}
	return 0;
}
