#include "modules/net.h"
#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "memutils.h"
#include "modules/logger.h"
#include "socket.h"
#include "task.h"

static const uint8_t http_socks[HTTP_SOCK_COUNT] = {0, 1, 2, 3};

static uint8_t rx_buf[SPI_MAX_XFER];

// hardcoded for now
static uint8_t http_resp[] = "HTTP/1.1 200 OK\r\n"
			     "Content-Type: text/plain\r\n"
			     "Content-Length: 3\r\n"
			     "Connection: close\r\n"
			     "\r\n"
			     "OK\n";

static void log_sock_st(uint8_t sock, uint8_t st) {
	const char* state = "UNKNOWN";
	uint8_t state_len = (uint8_t)(sizeof("UNKNOWN") - 1);

	switch (st) {
	case SOCK_CLOSED:
		state = "CLOSED";
		state_len = (uint8_t)(sizeof("CLOSED") - 1);
		break;

	case SOCK_INIT:
		state = "INIT";
		state_len = (uint8_t)(sizeof("INIT") - 1);
		break;

	case SOCK_LISTEN:
		state = "LISTEN";
		state_len = (uint8_t)(sizeof("LISTEN") - 1);
		break;

	case SOCK_SYNSENT:
		state = "SYN_SENT";
		state_len = (uint8_t)(sizeof("SYN_SENT") - 1);
		break;

	case SOCK_SYNRECV:
		state = "SYN_RECEIVED";
		state_len = (uint8_t)(sizeof("SYN_RECEIVED") - 1);
		break;

	case SOCK_ESTABLISHED:
		state = "ESTABLISHED";
		state_len = (uint8_t)(sizeof("ESTABLISHED") - 1);
		break;

	case SOCK_FIN_WAIT:
		state = "FIN_WAIT";
		state_len = (uint8_t)(sizeof("FIN_WAIT") - 1);
		break;

	case SOCK_CLOSING:
		state = "CLOSING";
		state_len = (uint8_t)(sizeof("CLOSING") - 1);
		break;

	case SOCK_TIME_WAIT:
		state = "TIME_WAIT";
		state_len = (uint8_t)(sizeof("TIME_WAIT") - 1);
		break;

	case SOCK_CLOSE_WAIT:
		state = "CLOSE_WAIT";
		state_len = (uint8_t)(sizeof("CLOSE_WAIT") - 1);
		break;

	case SOCK_LAST_ACK:
		state = "LAST_ACK";
		state_len = (uint8_t)(sizeof("LAST_ACK") - 1);
		break;
	}

	logger_log_literal_len("NET SOCK STATE:",
		(uint8_t)(sizeof("NET SOCK STATE:") - 1),
		state,
		state_len);

	if (st == SOCK_CLOSED || st == SOCK_INIT || st == SOCK_LISTEN || st == SOCK_SYNSENT ||
		st == SOCK_SYNRECV || st == SOCK_ESTABLISHED || st == SOCK_FIN_WAIT ||
		st == SOCK_CLOSING || st == SOCK_TIME_WAIT || st == SOCK_CLOSE_WAIT ||
		st == SOCK_LAST_ACK) {
		return;
	}

	uint8_t value[2] = {sock, st};

	logger_log_hex_len("NET SOCK UNKNOWN:",
		(uint8_t)(sizeof("NET SOCK UNKNOWN:") - 1),
		value,
		(uint8_t)sizeof(value));
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

		TickType_t start_tick = xTaskGetTickCount();

		uint8_t found_rx = 0;

		for (;;) {
			if (getSn_SR(sock) != SOCK_ESTABLISHED) // state changed
				break;				// exit

			uint16_t avail = getSn_RX_RSR(sock); // some data received from client

			if (avail > 0) {
				// received something from the client
				found_rx = 1;

				// drain what's available - recv() advances W5500 RX read pointer.
				int32_t n = recv(sock, rx_buf, sizeof(rx_buf));
				if (n <= 0) {
					// recv error - stop trying to read
					break;
				}

				continue;
			}

			// no data available at this moment
			TickType_t now_tick = xTaskGetTickCount();

			// If never saw any RX and timeout expired - disconnect - client is idle
			if (!found_rx && (now_tick - start_tick) >= REQUEST_TIMEOUT_TICKS) {
				disconnect(sock);
				break;
			}

			// If we already saw some RX, and now it's empty - request drained
			if (found_rx) {
				break;
			}

			// Still waiting for first byte: yield - do not hammer getSn_RX_RSR()
			vTaskDelay(1);
		}

		// If disconnected while waiting for data, skip sending response
		if (getSn_SR(sock) == SOCK_ESTABLISHED) {
			int32_t rc = send(sock, http_resp, (uint16_t)(sizeof(http_resp) - 1));

			if (rc < 0) {
				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"send() FAIL",
					(uint8_t)(sizeof("send() FAIL") - 1));
				close(sock); // hard recovery
				break;
			}
			// todo later: handle partial send()

			disconnect(sock);
		}
	} break;

	case SOCK_CLOSE_WAIT:

		close(sock);
		break;

	default:
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

int net_init(void) {

	int8_t st = w5500_init();

	if (st != 0) {
		logger_log_literal_len("NET INIT:",
			(uint8_t)(sizeof("NET INIT:") - 1),
			"W5500 INIT FAIL",
			(uint8_t)(sizeof("W5500 INIT FAIL") - 1));

		return -1;
	}

	BaseType_t ok = xTaskCreate(net_task, /* Task function */
		"net_task",		      /* Name (for debug) */
		2048,			      /* Stack size (words, not bytes) */
		NULL,			      /* Parameters */
		2,			      /* Priority */
		NULL			      /* Task handle */
	);

	if (ok != pdPASS) {
		logger_log_literal_len("NET INIT:",
			(uint8_t)(sizeof("NET INIT:") - 1),
			"TASK CREATE FAIL",
			(uint8_t)(sizeof("TASK CREATE FAIL") - 1));

		return -1;
	}

	return 0;
}
