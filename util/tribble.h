#define PORTADDRESS 0x378

#define DATA PORTADDRESS+0
#define STATUS PORTADDRESS+1
#define CONTROL PORTADDRESS+2

#define bsyin 0x40
#define bsyout 0x08
#define stbin 0x80
#define stbout 0x10
#define tribmask 0x07
#define dibmask 0x03

unsigned int havetribble(void);
unsigned char recvtribble(void);
unsigned char recvbyte(void);
void sendtribble(unsigned char b);
void sendbyte(unsigned char b);

extern int tribble_debug;
