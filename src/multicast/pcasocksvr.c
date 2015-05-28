#include <stdio.h>     
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>    
#include <time.h>    

//#define DEBUG

#define MAXPENDING 		5    /* Maximum outstanding connection requests */
#define DEFAULT_LISTEN_PORT	15000
#define RCVBUFSIZE 		2100   /* Size of receive buffer */
#define RCVBLOCKSIZE		32
#define WAIT_STX		1
#define WAIT_ETX		2
//#define STX_CHAR		0x02
#define STX_CHAR		'S'
//#define ETX_CHAR		0x03
#define ETX_CHAR		'E'

void error_exit(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void hexdump(const char *buf,unsigned int len);
void HandleTCPClient(int clntSocket);
void pcaPktHandler(int clntSocket,unsigned char *pktBuf, unsigned int len);
void print_timestamp();

#pragma pack(1)
struct pca_data_packet
{
//	unsigned char 	stx;		//0x02
	unsigned char 	saddr[6];
	unsigned char 	daddr[6];
	unsigned char	msgcode[3];
	unsigned char 	msgdata[2048];
//	unsigned char	etx;		//0x03
};

struct pca_ack_frame
{
	unsigned char 	stx;		//0x02
	unsigned char 	saddr[6];
	unsigned char 	daddr[6];
	unsigned char	msgcode[3];
	unsigned char	msgdata[10];
	unsigned char	etx;		//0x03
};


unsigned char gRcvBuf[RCVBUFSIZE];
unsigned int grcv_curr;
unsigned char grcvState = WAIT_STX;
unsigned char ack_code[] = {'9','9','9'};


int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */

    if (argc != 2)     /* Test for correct number of arguments */
    {
	//set to default port
    	echoServPort = DEFAULT_LISTEN_PORT;
        //fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        printf("Setting server to default port %d\n",echoServPort);
    }
    else
    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        error_exit("socket() failed");
      
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        error_exit("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        error_exit("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
            error_exit("accept() failed");

        /* clntSock is connected to a client! */
        printf("Client connected [%s]\n", inet_ntoa(echoClntAddr.sin_addr));

        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}

void error_exit(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}


void HandleTCPClient(int clntSocket)
{
    unsigned char recvBuffer[RCVBLOCKSIZE];
    int recvMsgSize,i;

    do
    {
    	/* Receive message from client */
        if ((recvMsgSize = recv(clntSocket, recvBuffer, RCVBLOCKSIZE, 0)) < 0)
            error_exit("recv() failed");
	if (recvMsgSize == 0 ) break;
#ifdef DEBUG
    	printf("received %d bytes\n", recvMsgSize);
    	hexdump(recvBuffer, recvMsgSize);
#endif
	//scan through the block
	i = 0;
	while (i < RCVBLOCKSIZE)
	{
		switch(recvBuffer[i])
		{
			case STX_CHAR:
				if (grcvState == WAIT_ETX) {
					printf("Error: multiple STX found.\n");
				}
				grcv_curr = 0;
				grcvState = WAIT_ETX;
				break; 
			case ETX_CHAR:
				if (grcvState == WAIT_ETX) {
					pcaPktHandler(clntSocket, gRcvBuf,grcv_curr);
					grcvState = WAIT_STX;
				}
				//ignore the character if state not WAIT_ETX
				break;
			default:
				if (grcvState == WAIT_ETX) {
					//copy the char to global buffer
					gRcvBuf[grcv_curr] = recvBuffer[i];
					grcv_curr++;
				}
				//ignore the character if state not WAIT_ETX
				break;
		}
		i++;
	}

    }
    while (recvMsgSize > 0);      /* zero indicates close by peer*/

exit:
    printf("\nconnection closed by client.\n");
    close(clntSocket);    /* Close client socket */
}

void pcaPktHandler(int clntSocket, unsigned char *pktBuf, unsigned int len)
{
int i=0,sendvMsgRes;
struct pca_data_packet *dataPkt;
struct pca_ack_frame ackPkt;

if (len < 16) {
	printf("<<< Received packet too small.Ignoring..\n");
	hexdump(pktBuf,len);
	return;
}
#ifdef DEBUG
printf("received pkt (Raw dump):\n");
hexdump(pktBuf,len);
#endif

dataPkt = (struct pca_data_packet *)pktBuf;
print_timestamp();
printf("<<< Received data Packet:[%c%c%c%c%c%c]->[%c%c%c%c%c%c] MsgID %c%c%c: \n",
					dataPkt->saddr[0], dataPkt->saddr[1], dataPkt->saddr[2],
					dataPkt->saddr[3], dataPkt->saddr[4], dataPkt->saddr[5],
					dataPkt->daddr[0], dataPkt->daddr[1], dataPkt->daddr[2],
					dataPkt->daddr[3], dataPkt->daddr[4], dataPkt->daddr[5],
					dataPkt->msgcode[0], dataPkt->msgcode[1], dataPkt->msgcode[2]);
hexdump(dataPkt->msgdata,len-15);

//prepare ACK packet
ackPkt.stx = STX_CHAR;
memcpy(ackPkt.saddr,dataPkt->daddr,6);
memcpy(ackPkt.daddr,dataPkt->saddr,6);
memcpy(ackPkt.msgcode, ack_code,3);
strcpy(ackPkt.msgdata,"ACK-MSG");
ackPkt.msgdata[7] = dataPkt->msgcode[0];
ackPkt.msgdata[8] = dataPkt->msgcode[1];
ackPkt.msgdata[9] = dataPkt->msgcode[2];
ackPkt.etx = ETX_CHAR;

//send packet - 27 bytes
if ((sendvMsgRes = send(clntSocket, (void *)&ackPkt, 1+6+6+3+10+1, 0)) < 0)
            error_exit("send() failed");

print_timestamp();
printf(">>> Sent ACK packet:[%c%c%c%c%c%c]->[%c%c%c%c%c%c] MsgID %c%c%c: \n",
					ackPkt.saddr[0], ackPkt.saddr[1], ackPkt.saddr[2],
					ackPkt.saddr[3], ackPkt.saddr[4], ackPkt.saddr[5],
					ackPkt.daddr[0], ackPkt.daddr[1], ackPkt.daddr[2],
					ackPkt.daddr[3], ackPkt.daddr[4], ackPkt.daddr[5],
					ackPkt.msgcode[0], ackPkt.msgcode[1], ackPkt.msgcode[2]);
hexdump((unsigned char *)&(ackPkt.msgdata),10);

return;
}

void hexdump(const char *buf,unsigned int len)
{
#define LINESIZE 16

unsigned int i=0,ii=0,j;
char carray[LINESIZE+1];

memset(carray,0,LINESIZE); ii=0;
while (i < len)
{

	printf("%02X ",buf[i]);carray[ii]=buf[i];
	i++;ii++;
	if (i && (i%LINESIZE == 0) ) {
		printf("\t"); 
		for (j=0; j<LINESIZE;j++) printf("%c",carray[j]); 
		printf("\n"); 
		memset(carray,0,LINESIZE); ii=0;
	}
}
if (i%LINESIZE) {
	for (j=0; j<3*(LINESIZE-i%LINESIZE);j++) printf(" ");
	printf("\t"); 
	for (j=0; j<LINESIZE;j++) printf("%c",carray[j]); 
	printf("\n");
}
}

void print_timestamp()
{
    char            fmt[64], buf[64];
    struct timeval  tv;
    struct tm       *tm;

    gettimeofday(&tv, NULL);
    if((tm = localtime(&tv.tv_sec)) != NULL)
    {
            strftime(fmt, sizeof fmt, "%Y-%m-%d %H:%M:%S", tm);
            snprintf(buf, sizeof buf, fmt, tv.tv_usec);
            printf("\n[%s]:\n", buf); 
    }

}
