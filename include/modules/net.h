#include <stdint.h>

// Static Config
#define NET_MAC {0x02, 0x00, 0x00, 0x00, 0x00, 0x50}
#define NET_IP {192, 168, 29, 70}
#define NET_SUBNET {255, 255, 255, 0}
#define NET_GATEWAY {192, 168, 29, 1}
#define NET_DNS {192, 168, 29, 1}

#define HTTP_SOCK_COUNT 8 // 8 sockets
#define HTTP_PORT 8080
#define MAX_PATH_LEN 256

// for ESTABLISHED socket state
#define REQUEST_TIMEOUT_TICKS pdMS_TO_TICKS(1000) // close if no RX data arrives within the timeout
// for CLOSE_WAIT socket state
#define CLEANUP_TIMEOUT_TICKS pdMS_TO_TICKS(250) // close if no RX data arrives within the timeout
// timeout
#define REQUEST_PROCESS_TIMEOUT_TICKS pdMS_TO_TICKS(5000) // 5 seconds

#define NET_RECOVERY_FAIL_THRESHOLD 5

#define HTTP_404                                                                                   \
	"HTTP/1.1 404 Not Found\r\n"                                                               \
	"Content-Type: text/html\r\n"                                                              \
	"Connection: close\r\n"                                                                    \
	"\r\n"                                                                                     \
	"<!DOCTYPE html>"                                                                          \
	"<html><head><title>404 Not Found</title></head>"                                          \
	"<body><h1>404 Not Found</h1>"                                                             \
	"<p>The requested resource could not be found on this server.</p>"                         \
	"</body></html>"

// Initialize the networking module
int net_init(void);

// Initialize the porting layer for the W5500
int8_t w5500_init(void);
