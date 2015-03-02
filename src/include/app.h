
#define Tx 0
#define Rx 1

struct post_st
{
unsigned char *buf;
unsigned int len;
unsigned char type;
};

struct pull_st
{
unsigned char *buf;
unsigned int len;
unsigned char type;
};
