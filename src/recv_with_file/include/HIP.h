
/* TP */
enum Pkt_Type
{
	DATA,
	VOICE
};

#define NO_CHUNKS_PER_PKT	5

enum EVENT
{
	VOC_NEWFRAME_EVENT = 1,
	DATA_NEWFRAME_EVENT,
	TP_TX_NEWFRAME_EVENT,
	//TP_RX_NEWFRAME_EVENT,
	IC_NEWFRAME_EVENT
};

struct tp_voice_hdr
{
	unsigned int msgtype:4; //msgtype – 4bits
	unsigned int srcid:4;   //srcid - 4bits
	unsigned int seqno:12;  //seqno – 12bits
	unsigned int len:12;    //len - 12bits

};

struct __attribute__((__packed__)) tp_data_hdr
{
	unsigned int msgtype:4; //msgtype – 4bits
	unsigned int srcid:4;   //srcid - 4bits
	unsigned int dstid:12;  //dstid – 12bits bitmap for PCA0 to PCA8 and remaining 3bits are reserve
	unsigned int len:12;    //len -12bits
	unsigned short cksum;
};

struct ic_hdr
{
	unsigned char srcid:2; // 2bit srcid
	unsigned char dstid:2; // 2bit dstid
};

#define SIZE_OF_EVT_BUF	100

struct evt_st
{
	int frame_end;
	int new_frame_len;
};


void new_frame_event(enum EVENT evt, unsigned int frame_end, unsigned short new_frame_len);
int voq_post(unsigned char *buf, int new_frame_len);
// void read_data_from_vocoder(void*);
void reset_event(int index, enum EVENT evt);
int check_event(enum EVENT evt);
void prepend_tp_header_to_data_in_ring_buf(char msgtype, char srcid, unsigned short dstid_bm, unsigned short len, int frame_end);
int tp_tx_evt_processing(void *data);
int voq_get(unsigned char *buf, int new_frame_len);
int voq_pull(int new_frame_len);
unsigned short get_len_from_data(unsigned char *data);
int tp_rx_evt_processing(void *data);


/* IC */

int icq_get(unsigned char *buf, int new_frame_len);
int icq_pull(int new_frame_len);
void add_ic_hdr_to_data(unsigned char *buf);
int ic_send(unsigned char *buf, int new_frame_len);
int ic_evt_processing(void *data);

