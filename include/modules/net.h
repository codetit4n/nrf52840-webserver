// Static Config
#define MAC {0x02, 0x00, 0x00, 0x00, 0x00, 0x50}
#define IP {192, 168, 29, 70}
#define SUBNET {255, 255, 255, 0}
#define GATEWAY {192, 168, 29, 1}
#define DNS {192, 168, 29, 1}

#define HTTP_SOCK_COUNT 4 // 4 sockets (0-7) for w5500
#define HTTP_PORT 8080

// for ESTABLISHED socket state
#define REQUEST_TIMEOUT_TICKS pdMS_TO_TICKS(1000) // close if no RX data arrives within the timeout
// for CLOSE_WAIT socket state
#define CLEANUP_TIMEOUT_TICKS pdMS_TO_TICKS(250) // close if no RX data arrives within the timeout

// Initialize the networking module
void net_init(void);

// Initialize the porting layer for the W5500
void w5500_init(void);
