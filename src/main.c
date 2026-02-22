#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "memutils.h"
#include "socket.h"
#include "task.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include <stdint.h>

// single socket for now, can do more later
#define HTTP_SOCK 0
#define HTTP_PORT 8080

static uint8_t rx_buf[SPI_MAX_XFER];

static const char http_resp[] = "HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 6\r\n"
				"Connection: close\r\n"
				"\r\n"
				"hello\n";

void w5500_init(void);

static void net_task(void* arg) {
	(void)arg;

	logger_log_literal_len("NET:",
		(uint8_t)(sizeof("NET:") - 1),
		"INIT",
		(uint8_t)(sizeof("INIT") - 1));

	w5500_init();

	// doing static ip for now
	struct wiz_NetInfo_t net = {.mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x50},
		.ip = {192, 168, 29, 70},
		.sn = {255, 255, 255, 0},
		.gw = {192, 168, 29, 1},
		.dns = {192, 168, 29, 1},
		.dhcp = NETINFO_STATIC};

	ctlnetwork(CN_SET_NETINFO, &net);

	struct wiz_NetInfo_t get_net = {0};

	ctlnetwork(CN_GET_NETINFO, &get_net);

	// only for static ip
	configASSERT(get_net.dhcp == NETINFO_STATIC);
	configASSERT(mem_cmp(get_net.mac, net.mac, 6) == 0);
	configASSERT(mem_cmp(get_net.ip, net.ip, 4) == 0);
	configASSERT(mem_cmp(get_net.sn, net.sn, 4) == 0);
	configASSERT(mem_cmp(get_net.gw, net.gw, 4) == 0);
	configASSERT(mem_cmp(get_net.dns, net.dns, 4) == 0);

	uint8_t last = 0xFF;

	for (;;) {
		uint8_t st = getSn_SR(HTTP_SOCK);

		if (st != last) {
			last = st;
			logger_log_literal_len("NET:",
				(uint8_t)(sizeof("NET:") - 1),
				"SOCK ST:",
				(uint8_t)(sizeof("SOCK ST:") - 1));
			logger_log_hex_len("NET:", (uint8_t)(sizeof("NET:") - 1), &st, 1);
		}

		switch (st) {
		case SOCK_CLOSED: {
			int8_t r = socket(HTTP_SOCK, Sn_MR_TCP, HTTP_PORT, 0);
			if (r != HTTP_SOCK) {
				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"socket() FAIL",
					(uint8_t)(sizeof("socket() FAIL") - 1));
				break;
			}
			if (listen(HTTP_SOCK) != SOCK_OK) {
				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"listen() FAIL",
					(uint8_t)(sizeof("listen() FAIL") - 1));
				close(HTTP_SOCK);
			}
		} break;

		case SOCK_LISTEN:
			// waiting for a client
			break;

		case SOCK_ESTABLISHED: {
			uint16_t avail = getSn_RX_RSR(HTTP_SOCK);
			if (avail > 0) {
				int32_t n = recv(HTTP_SOCK, rx_buf, sizeof(rx_buf));
				(void)n;
			}

			int32_t s = send(HTTP_SOCK,
				(uint8_t*)http_resp,
				(uint16_t)(sizeof(http_resp) - 1));
			(void)s;

			disconnect(HTTP_SOCK); // client will see Connection: close
		} break;

		case SOCK_CLOSE_WAIT:
			close(HTTP_SOCK);
			break;

		default:
			// later: recover if stuck in weird state
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(5));
	}
}

int main(void) {
	spim_init();
	logger_init();

	BaseType_t ok = xTaskCreate(net_task, "net_task", 512, NULL, 2, NULL);
	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
