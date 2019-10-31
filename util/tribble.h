#define PORTADDRESS	0x378

/* relative to PORTADDRESS (tribble_port) */
#define DATA		0
#define STATUS		1
#define CONTROL		2

#define bsyin		0x40
#define bsyout		0x08
#define stbin		0x80
#define stbout		0x10
#define tribmask	0x07
#define dibmask		0x03

unsigned int havetribble(void);
int recvtribble(void);
int recvbyte(void);
int sendtribble(unsigned char b);
int sendbyte(unsigned char b);

extern int tribble_debug;
unsigned int tribble_port;
