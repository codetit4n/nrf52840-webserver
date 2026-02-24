#include "net.h"
#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "memutils.h"
#include "socket.h"
#include "task.h"

static const uint8_t http_socks[HTTP_SOCK_COUNT] = {0, 1, 2, 3};

static uint8_t rx_buf[SPI_MAX_XFER];

// hardcoded for
static const char http_resp[] = "HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 6\r\n"
				"Connection: close\r\n"
				"\r\n"
				"OK\n";

static void log_sock_st(uint8_t sock, uint8_t st) {
	logger_log_literal_len("NET:",
		(uint8_t)(sizeof("NET:") - 1),
		"SOCK STATUS CHANGE",
		(uint8_t)(sizeof("SOCK STATUS CHANGE") - 1));

	// log sock id + status as 2 bytes: [sock, st]
	uint8_t v[2] = {sock, st};
	logger_log_hex_len("NET:", (uint8_t)(sizeof("NET:") - 1), v, 2);
}

static void handle_http_sock(uint8_t sock, uint8_t* last_st) {
	uint8_t st = getSn_SR(sock);

	if (st != *last_st) {
		*last_st = st;
		log_sock_st(sock, st);
	}

	switch (st) {
	case SOCK_CLOSED: {
		int8_t r = socket(sock, Sn_MR_TCP, HTTP_PORT, 0);
		if (r != (int8_t)sock) {
			logger_log_literal_len("NET:",
				(uint8_t)(sizeof("NET:") - 1),
				"socket() FAIL",
				(uint8_t)(sizeof("socket() FAIL") - 1));
			break;
		}
		if (listen(sock) != SOCK_OK) {
			logger_log_literal_len("NET:",
				(uint8_t)(sizeof("NET:") - 1),
				"listen() FAIL",
				(uint8_t)(sizeof("listen() FAIL") - 1));
			close(sock);
		}
	} break;

	case SOCK_LISTEN:
		// waiting for a client
		break;

	case SOCK_ESTABLISHED: {
		uint16_t avail = getSn_RX_RSR(sock);
		if (avail > 0) {
			int32_t n = recv(sock, rx_buf, sizeof(rx_buf));
			(void)n;
		}

		int32_t s = send(sock, (uint8_t*)http_resp, (uint16_t)(sizeof(http_resp) - 1));
		(void)s;

		disconnect(sock); // client will see Connection: close
	} break;

	case SOCK_CLOSE_WAIT:
		close(sock);
		break;

	default:
		// optional: recover if stuck later
		break;
	}
}

static void net_task(void* arg) {
	(void)arg;

	struct wiz_NetInfo_t net = {
		.mac = MAC,
		.ip = IP,
		.sn = SUBNET,
		.gw = GATEWAY,
		.dns = DNS,
		.dhcp = NETINFO_STATIC,
	};

	ctlnetwork(CN_SET_NETINFO, &net);

	struct wiz_NetInfo_t get_net = {0};
	ctlnetwork(CN_GET_NETINFO, &get_net);

	configASSERT(get_net.dhcp == NETINFO_STATIC);
	configASSERT(mem_cmp(get_net.mac, net.mac, 6) == 0);
	configASSERT(mem_cmp(get_net.ip, net.ip, 4) == 0);
	configASSERT(mem_cmp(get_net.sn, net.sn, 4) == 0);
	configASSERT(mem_cmp(get_net.gw, net.gw, 4) == 0);
	configASSERT(mem_cmp(get_net.dns, net.dns, 4) == 0);

	uint8_t last_st[HTTP_SOCK_COUNT] = {0xFF, 0xFF, 0xFF, 0xFF};

	for (;;) {
		for (uint8_t i = 0; i < HTTP_SOCK_COUNT; i++) {
			handle_http_sock(http_socks[i], &last_st[i]);
		}

		vTaskDelay(pdMS_TO_TICKS(5));
	}
}

void net_init(void) {
	w5500_init();

	BaseType_t ok = xTaskCreate(net_task, "net_task", 512, NULL, 2, NULL);
	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}
}
