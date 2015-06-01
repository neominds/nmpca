#include <speex/speex.h>
#include <stdio.h>
#include <time.h>
/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 160
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include<pthread.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include<signal.h>
#include "app.h"
#include"ring_buf_user.h"
#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)

int run_flag=1;
int bufid;
static void* encode_start(void*);
void handler(int);
int main()
{
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
        pthread_t t;
        struct sigaction sa;
        sa.sa_flags=SA_RESETHAND;
        sa.sa_handler=handler;
        if( sigaction(SIGINT,&sa,NULL)==-1)
        {
          printf("error in sigaction\n");
          return 0;
        }
        bufid=ring_buf_init();
	pthread_create(&t,NULL,encode_start,NULL);
        FILE *fp;

       fp=fopen("sample","w");
	/* Open PCM device for recording (capture). */
	rc = snd_pcm_open(&handle, "default",
			SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr,
				"unable to open pcm device: %s\n",
				snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params,
			SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 1);

	/* 44100 bits/second sampling rate (CD quality) */
	val = 8000;
	snd_pcm_hw_params_set_rate_near(handle, params,
			&val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;

	snd_pcm_hw_params_set_period_size_near(handle,
			params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,
				"unable to set hw parameters: %s\n",
				snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params,
			&frames, &dir);
	size = frames * 2; /* 2 bytes/sample, 2 channels */
	//printf("\nsize %d \n",size);
	buffer = (char *) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,
			&val, &dir);
	loops = 5000000 / val;
        printf("size is %d val is %d\n",size,val);
       while(run_flag)
//	while (loops>0) 
        {

		loops--;
		rc = snd_pcm_readi(handle, buffer, frames);

		//printf("pcm_read : rc = %d frames = %d\n",rc,frames);

		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr,
					"error from read: %s\n",
					snd_strerror(rc));
		} else if (rc != (int)frames) {
			fprintf(stderr, "short read, read %d frames\n", rc);
		}
//              fwrite(buffer,1,size,fp); 
		

		rc=post_data(bufid,buffer,size);
		if (rc != size)
			fprintf(stderr,
					"short write: wrote %d bytes\n", rc);
		
	}
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
        fclose(fp);
printf("record cleanup done\n");
       pthread_join(t,NULL);
printf("encode thread joined\n");
	return 0;
}


static void* encode_start(void* data)
{

	FILE *fp;
	fp=fopen("sample_enc","w");
	short in[FRAME_SIZE];
	float input[FRAME_SIZE];
	char cbits[200];
	unsigned char nbBytes;
	/*Holds the state of the encoder*/
	void *state;
	/*Holds bits so they can be read and written to by the Speex routines*/
	SpeexBits bits;
	int i, tmp,r,fd;
	struct post_st postdata;
	time_t start,stop;
	int interval;
	int previous;
        int total=0;
	/*Create a new encoder state in narrowband mode*/
	state = speex_encoder_init(&speex_nb_mode);

	/*Set the quality to 8 (15 kbps)*/
	tmp=8;
	speex_encoder_ctl(state, SPEEX_SET_QUALITY, &tmp);


	/*Initialization of the structure that holds the bits*/
	speex_bits_init(&bits);
	postdata.type=Tx;  
	fd = open("/dev/my_dev", O_RDWR);
	if(fd<0){ printf("\n post open fails\n"); return 0;}

	time(&start);
        while(run_flag)
//	while (interval<6)
	{
		/*Read a 16 bits/sample audio frame*/

		
		if(bytes_in_ring(bufid)>=(FRAME_SIZE*2))
		{
			
                        r=get_data(bufid,(FRAME_SIZE*2),(unsigned char*)in);
                        if(r==(FRAME_SIZE*2))
                        pull(bufid,FRAME_SIZE*2);
                        else
                        continue;
//			printf("get_data return %d\n",r); 
			
		}
		else
		{
			
			time(&stop);
			interval=difftime(stop,start);
			if(interval!=previous)
			{
				previous=interval;
				printf(" time interval %d\n",interval);
			}
			continue;
		}
		/*Copy the 16 bits values to float so Speex can work on them*/
		for (i=0;i<FRAME_SIZE;i++)
			input[i]=in[i];
		/*Flush all the bits in the struct so we can encode a new frame*/
		speex_bits_reset(&bits);

		/*Encode the frame*/
		speex_encode(state, input, &bits);
		/*Copy the bits to an array of char that can be written*/
		nbBytes = speex_bits_write(&bits, cbits, 200);
		printf("SIZE: %d\n ",nbBytes);
		/*Write the size of the frame first. This is what sampledec expects but
		  it's likely to be different in your own application*/
		postdata.buf=&nbBytes;
		postdata.len=1;
		if(ioctl(fd, WRITE_IOCTL, &postdata)<0) {
			printf("post ioctl fail\n");
			close(fd);
			return 0;
		}
		/*Write the compressed data*/
		postdata.buf=cbits;
		postdata.len=nbBytes;
		if(ioctl(fd, WRITE_IOCTL, &postdata)<0) {
			printf("post ioctl fail\n");
			close(fd);
			return 0;
		}
                total=total+nbBytes+1;
//		fwrite(&nbBytes,1,1,fp);
//		fwrite(cbits,1,nbBytes,fp);
		time(&stop);
		interval=difftime(stop,start);
		if(interval!=previous)
		{
			previous=interval;
			printf(" time interval %d\n",interval);
		}
	}
	printf("encode thread done time interval %d\n",interval);
	/*Destroy the encoder state*/
	speex_encoder_destroy(state);
	/*Destroy the bit-packing struct*/
	speex_bits_destroy(&bits);
	close(fd);
	fclose(fp);
        printf("total data %d\n",total);
	printf("encode cleanup done\n");
	return 0;
}                           

void handler(int sig)
{
printf("entering signal handler\n");
run_flag=0;
}


