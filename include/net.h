// CSN pin for W5500
#define CSN_PIN 26

// Static Config
#define MAC {0x02, 0x00, 0x00, 0x00, 0x00, 0x50}
#define IP {192, 168, 29, 70}
#define SUBNET {255, 255, 255, 0}
#define GATEWAY {192, 168, 29, 1}
#define DNS {192, 168, 29, 1}

#define HTTP_SOCK_COUNT 4 // 4 sockets (0-3 in w5500) for http
#define HTTP_PORT 8080

// Initialize the porting layer for the W5500
void w5500_init(void);

// Initialize the networking module
void net_init(void);
