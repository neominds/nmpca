#include <speex/speex.h>
#include <stdio.h>
#include<linux/ioctl.h>
#include<string.h>
#include<linux/kdev_t.h>
#include<pthread.h>
#include<time.h>
#include"app.h"
#include<unistd.h>
#include"ring_buf_user.h"
#include<signal.h>
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, char *)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, char *)
#define READ_IOCTL_SYN _IOR(MY_MACIG,2, char *)

/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 160

int bufid;
int bufid1;
int run_flag=1;
static void* play_start(void*);
static void* encode_start(void *);
pthread_t t1;
void handler(int);
int main()
{
	char temp[15000];
	int interval;
	pthread_t t;
	time_t start,stop;
	bufid=ring_buf_init();
	bufid1=ring_buf_init();
	pthread_create(&t,NULL,encode_start,NULL);
	struct pull_st pull_data;
	int fd,ret;
        int q;
        int total=0;
        struct sigaction sa;
        sa.sa_flags=SA_RESETHAND;
        sa.sa_handler=handler;
        if(sigaction(SIGINT,&sa,NULL)==-1)
        {
           printf("error in sigaction\n");
           return 0;
        }
 
	fd = open("/dev/my_cdev", O_RDWR);
	if(fd<0){ printf("pull open fails\n"); return 1;}
	time(&start); 

	while (run_flag)
//        while(interval<10)
	{
		/*Read the size encoded by sampleenc, this part will likely be 
		  different in your application*/

		pull_data.len=1000;
		pull_data.buf=temp;
		ret=ioctl(fd, READ_IOCTL_SYN,&pull_data);
		if(ret>0)
		{
		     q=	post_data(bufid,temp,ret);
                    
          //              printf("main thread post data return %d\n",q);
			time(&stop);
			interval=difftime(stop,start);
                   total+=q;  
                   
		}
		else
		{      
                      // printf("timeout\n");
                       time(&stop);
			interval=difftime(stop,start);
		}


	}       
	close(fd);        
	pthread_join(t,NULL);
	printf("encode thread joines\n");
        printf("total data %d\n",total);
	pthread_join(t1,NULL);
	printf("play thread joines\n");
	return 0;
}

static void* encode_start(void *data)
{

	FILE *fp;
	FILE *fp1;
	fp =fopen("sample_enc","w");  
	fp1=fopen("sample","w");
	/*Holds the audio that will be written to file (16 bits per sample)*/
	short out[FRAME_SIZE];
	/*Speex handle samples as float, so we need an array of floats*/
	float output[FRAME_SIZE];
	char cbits[200];
	unsigned char nbBytes;
	/*Holds the state of the decoder*/
	void *state;
	/*Holds bits so they can be read and written to by the Speex routines*/
	SpeexBits bits;
	int i, tmp;
	time_t start,stop;
	int interval=0;
	int previous=0;
	int flag=0;
        int ret,r;
        int pflag=0;
        



	/*Create a new decoder state in narrowband mode*/
	state = speex_decoder_init(&speex_nb_mode);

	/*Set the perceptual enhancement on*/
	tmp=1;
	speex_decoder_ctl(state, SPEEX_SET_ENH, &tmp);



	/*Initialization of the structure that holds the bits*/
	speex_bits_init(&bits);

	time(&start);
	while (run_flag)
//        while(interval<11)
	{
		/*Read the size encoded by sampleenc, this part will likely be 
		  different in your application*/
               
		if(flag!=1 && (bytes_in_ring(bufid)>=1))
		{
			ret=get_data(bufid,1,&nbBytes);		
//                        printf("encode thread get data return %d\n",ret);
                        if(ret==1)
                       {
			r=pull(bufid,ret);
                        
//                        printf("encode thread pull ret %d\n",r);
                        if(nbBytes!=38)
                        {
                      printf("nbByes mismatch\n");
			 time(&stop);
			 interval=difftime(stop,start);
                         continue;
                        }
                       else
			 {
                          flag=1;
                          continue;
                         }
                       }
                       else
                       continue;
                }
		else if((bytes_in_ring(bufid)>=nbBytes) && flag==1)
	        {
                       
                        ret=get_data(bufid,nbBytes,cbits);
                        if(ret==nbBytes)
                       {
			pull(bufid,ret);
			flag=0;
                       }
                       else
                      continue;   
                  //  printf("encode thread get data return %d\n",ret);
		}
		else
		{
			time(&stop);
			interval=difftime(stop,start);
			continue;
		}
	//	fwrite(&nbBytes,1,1,fp);
	//	fwrite(cbits,1,nbBytes,fp);

		/*Copy the data into the bit-stream struct*/

		speex_bits_read_from(&bits, cbits, nbBytes);

		/*Decode the data*/
		speex_decode(state, &bits, output);

		/*Copy from float to short (16 bits) for output*/
		for (i=0;i<FRAME_SIZE;i++)
			out[i]=output[i];
	//	fwrite(out,1,FRAME_SIZE*2,fp1);
		/*Write the decoded audio to file*/
		ret=post_data(bufid1,(unsigned char*)out,FRAME_SIZE*2);
//                printf("encode thread post data return %d\n",ret);
                if(pflag==0 && bytes_in_ring(bufid1)>= 128*500)
                {
	        pthread_create(&t1,NULL,play_start,NULL);
                pflag=1;
                }
		time(&stop);
		interval=difftime(stop,start);
	}

	/*Destroy the decoder state*/
	speex_decoder_destroy(state);
	/*Destroy the bit-stream truct*/
	speex_bits_destroy(&bits);
	fclose(fp);
	fclose(fp1);
	printf("decoder cleanup done\n");
	return 0;
}



static void* play_start(void* data)
{
	int rc;
        int r;
	int size,i;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	time_t start,stop;
	int interval=0;
	int previous=0;
        int pr;
        char *buff;
        int nret=5;
        int num;
        int pck;
	/* Open PCM device for playback. */

	rc = snd_pcm_open(&handle, "default",
			SND_PCM_STREAM_PLAYBACK, 0);
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
	snd_pcm_hw_params_get_period_size(params, &frames,
			&dir);
	size = frames * 2; /* 2 bytes/sample, 2 channels */
	buffer = (char *) malloc(size);
        buff= (char *)malloc(size * 700);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,
			&val, &dir);
	time(&start);
	while (run_flag)
//        while(interval<8)
	{


		if(bytes_in_ring(bufid1)>0)
               {
		        num=bytes_in_ring(bufid1);	
                        pck=num/size;
                        if(pck>0)
                        {
                        printf("play thread get data return %d  packet nos: %d \n",rc,pck);
                        num=pck*size;
                        rc=get_data(bufid1,num,buff);
                        if(rc==num)
			r=pull(bufid1,num);
		//       printf("play thread pull return %d\n",r);
                        }
                        else
                        continue;
                     
                
               }
                
		else
		{

			time(&stop);
			interval=difftime(stop,start);
			if(interval!=previous)
			{
				previous=interval;
				printf("inteval is %d\n",interval);
			}
                   
			continue;
                   
		}
               
               for(i=0;i<pck;i++)
              {
                 
                memcpy(buffer,buff+(size*i),size);
                rc=size;
		if (rc == 0) {
			fprintf(stderr, "end of file on input\n");
			break;
		} else if (rc != size) {
			fprintf(stderr,
					"short read: read %d bytes\n", rc);
		}

		rc = snd_pcm_writei(handle, buffer, frames);
		if (rc == -EPIPE) {
			/* EPIPE means underrun */
			
		//	printf( "underrun occurred %d\n ",i);
		      do{
                	 pr=snd_pcm_recover(handle,-EPIPE,0);
                        // printf("prepare return %d\n",pr);
                        }while(pr);
		} 
 
                 
             else if (rc < 0) {
			fprintf(stderr,
					"error from writei: %s\n",
					snd_strerror(rc));
		}  else if (rc != (int)frames) {
			fprintf(stderr,
					"short write, write %d frames\n", rc);
		}
             }
            
		time(&stop);
		interval=difftime(stop,start);
		if(interval!=previous)
		{
			previous=interval;
			printf("inteval is %d\n",interval);
		}
	}
	printf("play thread is done time is %d\n",interval);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
        free(buff);
	printf("play thread cleanup done\n");
	return 0;
}

void handler(int sig)
{
printf("entering signal handler\n");
run_flag=0;
}



