#include<stdio.h>
#include<stdlib.h>
#include"ring_buf.h"


main()
{
FILE *fp1,*fp2;
unsigned char buf[100];
int ring_bufid,i;
//ring_bufid=ring_buf_init();


fp1=fopen("input.bin","rb");
fp2=fopen("output.bin","wb");

for(i=0;i<1000;i++)
{
fread(buf,100,1,fp1);
//post_data(ring_bufid,buf,100);
}

for(i=0;i<1000;i++)
{
//get_data(ring_bufid,100,buf);
//pull(ring_bufid,100);
fwrite(buf,100,1,fp2);
}
fclose(fp1);
fclose(fp2);
}
