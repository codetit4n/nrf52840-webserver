#include "modules/net.h"
#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "ff.h"
#include "memutils.h"
#include "modules/logger.h"
#include "socket.h"
#include "task.h"
#include "w5500.h"

static const uint8_t http_socks[HTTP_SOCK_COUNT] = {0, 1, 2, 3};
static uint8_t rx_buf[SPI_MAX_XFER];

static uint8_t get_mime_type(const char ext[], int n, const char** mime) {
	if ((n == 4 && mem_cmp(ext, "html", 4) == 0) || (n == 4 && mem_cmp(ext, "HTML", 4) == 0) ||
		(n == 3 && mem_cmp(ext, "htm", 3) == 0) ||
		(n == 3 && mem_cmp(ext, "HTM", 3) == 0)) {

		*mime = "text/html";
		return sizeof("text/html") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "css", 3) == 0) || (n == 3 && mem_cmp(ext, "CSS", 3) == 0)) {

		*mime = "text/css";
		return sizeof("text/css") - 1;
	}

	if ((n == 2 && mem_cmp(ext, "js", 2) == 0) || (n == 2 && mem_cmp(ext, "JS", 2) == 0)) {

		*mime = "application/javascript";
		return sizeof("application/javascript") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "png", 3) == 0) || (n == 3 && mem_cmp(ext, "PNG", 3) == 0)) {

		*mime = "image/png";
		return sizeof("image/png") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "jpg", 3) == 0) || (n == 3 && mem_cmp(ext, "JPG", 3) == 0) ||
		(n == 4 && mem_cmp(ext, "jpeg", 4) == 0) ||
		(n == 4 && mem_cmp(ext, "JPEG", 4) == 0)) {

		*mime = "image/jpeg";
		return sizeof("image/jpeg") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "svg", 3) == 0) || (n == 3 && mem_cmp(ext, "SVG", 3) == 0)) {

		*mime = "image/svg+xml";
		return sizeof("image/svg+xml") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "ico", 3) == 0) || (n == 3 && mem_cmp(ext, "ICO", 3) == 0)) {

		*mime = "image/x-icon";
		return sizeof("image/x-icon") - 1;
	}

	if ((n == 3 && mem_cmp(ext, "txt", 3) == 0) || (n == 3 && mem_cmp(ext, "TXT", 3) == 0)) {

		*mime = "text/plain";
		return sizeof("text/plain") - 1;
	}

	*mime = "application/octet-stream";
	return sizeof("application/octet-stream") - 1;
}

static uint8_t
build_http_headers(uint8_t out[], size_t out_size, const char* mime, uint8_t mime_len) {
	static const uint8_t status[] = "HTTP/1.1 200 OK\r\n";
	static const uint8_t content_type[] = "Content-Type: ";
	static const uint8_t connection[] = "\r\nConnection: close\r\n\r\n";

	size_t status_len = sizeof(status) - 1;
	size_t content_type_len = sizeof(content_type) - 1;
	size_t connection_len = sizeof(connection) - 1;

	size_t total_len = status_len + content_type_len + mime_len + connection_len;

	if (total_len > out_size) {
		return 0;
	}

	size_t pos = 0;

	mem_cpy(out + pos, status, status_len);
	pos += status_len;

	mem_cpy(out + pos, content_type, content_type_len);
	pos += content_type_len;

	mem_cpy(out + pos, mime, mime_len);
	pos += mime_len;

	mem_cpy(out + pos, connection, connection_len);
	pos += connection_len;

	return (uint8_t)pos;
}

static int32_t net_send_all(uint8_t sock, const uint8_t* buf, size_t len) {
	size_t sent = 0;

	while (sent < len) {
		int32_t sr = send(sock, (uint8_t*)(buf + sent), (uint16_t)(len - sent));
		if (sr <= 0) {
			return sr;
		}
		sent += (size_t)sr;
	}

	return (int32_t)sent;
}

static int get_next_word(uint8_t inp[], uint8_t out[], int ptr, int lim) {
	int i = ptr;

	while (i < lim && inp[i] != ' ' && inp[i] != '\r' && inp[i] != '\n' && inp[i] != '\0') {

		out[i - ptr] = inp[i];
		i++;
	}

	return i - ptr;
}

static int get_file_extension(uint8_t filename[], uint8_t ext[], int lim) {
	int i = 0;
	int dot = -1;

	while (i < lim && filename[i] != '\0') {
		if (filename[i] == '.') {
			dot = i;
		}

		i++;
	}

	if (dot < 0) {
		return 0;
	}

	int mime_len = i - dot - 1;

	for (int j = 0; j < mime_len; j++) {
		ext[j] = filename[dot + 1 + j];
	}

	ext[mime_len] = '\0';

	return mime_len;
}

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

	size_t sock_label_str_len = sizeof("NET SOCK 0: ") - 1;

	char sock_label_str[] = {'N', 'E', 'T', ' ', 'S', 'O', 'C', 'K', ' ', '0', ':', ' ', '\0'};

	sock_label_str[9] = (uint8_t)('0' + sock);

	logger_log_literal_len(sock_label_str, (uint8_t)sock_label_str_len, state, state_len);

	if (st == SOCK_CLOSED || st == SOCK_INIT || st == SOCK_LISTEN || st == SOCK_SYNSENT ||
		st == SOCK_SYNRECV || st == SOCK_ESTABLISHED || st == SOCK_FIN_WAIT ||
		st == SOCK_CLOSING || st == SOCK_TIME_WAIT || st == SOCK_CLOSE_WAIT ||
		st == SOCK_LAST_ACK) {
		return;
	}

	uint8_t value[2] = {sock, st};

	logger_log_literal_len(sock_label_str,
		(uint8_t)sock_label_str_len,
		"UNKNOWN STATE",
		(uint8_t)(sizeof("UNKNOWN STATE") - 1));

	logger_log_hex_len(sock_label_str, (uint8_t)sock_label_str_len, value, sizeof(value));
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
		/* Waiting for a client */
		break;

	case SOCK_ESTABLISHED: {

		TickType_t start_tick = xTaskGetTickCount();

		size_t req_len = 0;
		uint8_t req_fin = 0;

		for (;;) {

			/* Socket state changed while receiving */
			if (getSn_SR(sock) != SOCK_ESTABLISHED) {
				break;
			}

			uint16_t avail = getSn_RX_RSR(sock);

			if (avail == 0) {

				if ((xTaskGetTickCount() - start_tick) >= REQUEST_TIMEOUT_TICKS) {

					logger_log_literal_len("NET:",
						(uint8_t)(sizeof("NET:") - 1),
						"request timeout",
						(uint8_t)(sizeof("request timeout") - 1));

					disconnect(sock);
					break;
				}

				vTaskDelay(1);
				continue;
			}

			if (req_len >= sizeof(rx_buf)) {
				/* Request headers too large for buffer */
				disconnect(sock);
				break;
			}

			size_t remains = sizeof(rx_buf) - req_len;

			int32_t n = recv(sock, rx_buf + req_len, (uint16_t)remains);

			if (n <= 0) {
				break;
			}

			req_len += (size_t)n;

			if (req_len >= 4) {
				for (size_t i = 0; i <= req_len - 4; i++) {

					if (mem_cmp(&rx_buf[i], (const uint8_t*)"\r\n\r\n", 4) ==
						0) {
						req_fin = 1;
						break;
					}
				}
			}

			if (req_fin) {
				break;
			}
		}

		/* Socket disappeared while receiving */
		if (getSn_SR(sock) != SOCK_ESTABLISHED) {
			break;
		}

		/* Incomplete request */
		if (!req_fin) {
			disconnect(sock);
			break;
		}

		TickType_t process_start = xTaskGetTickCount();

		if (req_len >= 6 && mem_cmp(rx_buf, (const uint8_t*)"GET /", 5) == 0) {

			uint8_t f_name[req_len + 1];
			int f_len = get_next_word(rx_buf, f_name, 4, req_len);
			f_name[f_len] = '\0';

			uint8_t f_name_appended[MAX_PATH_LEN] = {0};
			mem_cpy(f_name_appended, "0:", 2);

			if (f_len == 1 && mem_cmp(f_name, "/", 1) == 0) {
				mem_cpy(f_name_appended + 2, "/INDEX.HTML", 11);
				f_name_appended[13] = '\0';
			} else {
				mem_cpy(f_name_appended + 2, f_name, f_len);
				f_name_appended[f_len + 2] = '\0';
			}

			uint8_t ext[MAX_PATH_LEN];
			int ext_len = get_file_extension(f_name_appended, ext, MAX_PATH_LEN);

			FIL file = {0};
			FRESULT fr = f_open(&file, (const char*)f_name_appended, FA_READ);

			if (fr == FR_NO_FILE || fr == FR_NO_PATH) {
				int32_t sr = net_send_all(sock,
					(const uint8_t*)HTTP_404,
					sizeof(HTTP_404) - 1);

				if (sr <= 0) {

					logger_log_literal_len("NET:",
						(uint8_t)(sizeof("NET:") - 1),
						"404 send() FAIL",
						(uint8_t)(sizeof("404 send() FAIL") - 1));

					close(sock);
					break;
				}

				disconnect(sock);
				break;
			}

			if (fr != FR_OK) {
				/* Other filesystem/storage error */
				disconnect(sock);
				break;
			}

			/* Send HTTP response headers */
			const char* mime = NULL;
			uint8_t mime_len = get_mime_type((const char*)ext, ext_len, &mime);

			uint8_t http_headers[96];
			uint8_t http_headers_len = build_http_headers(http_headers,
				sizeof(http_headers),
				mime,
				mime_len);

			if (http_headers_len == 0) {
				f_close(&file);
				close(sock);
				break;
			}

			int32_t sr = net_send_all(sock, http_headers, http_headers_len);

			if (sr <= 0) {
				logger_log_hex_len("NET:HEADERS SEND SR:",
					(uint8_t)(sizeof("NET:HEADERS SEND SR:") - 1),
					(uint8_t*)&sr,
					sizeof(sr));

				f_close(&file);
				close(sock);
				break;
			}

			/* Stream file contents */
			uint8_t filedata[SPI_MAX_XFER] = {0};

			UINT br = 0;
			uint8_t failed = 0;

			for (;;) {
				if ((xTaskGetTickCount() - process_start) >=
					REQUEST_PROCESS_TIMEOUT_TICKS) {

					logger_log_literal_len("NET:",
						(uint8_t)(sizeof("NET:") - 1),
						"processing timeout",
						(uint8_t)(sizeof("processing timeout") - 1));

					failed = 1;
					break;
				}

				fr = f_read(&file, filedata, sizeof(filedata), &br);

				if (fr != FR_OK) {
					failed = 1;
					break;
				}

				if (br == 0) {
					/* EOF */
					break;
				}

				sr = net_send_all(sock, filedata, br);

				if (sr <= 0) {
					failed = 1;
					break;
				}
			}

			f_close(&file);

			if (failed) {
				close(sock);
				break;
			}

			disconnect(sock);
			break;

		} else {

			/* Unknown path/method */
			int32_t sr =
				net_send_all(sock, (const uint8_t*)HTTP_404, sizeof(HTTP_404) - 1);

			if (sr <= 0) {

				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"404 send() FAIL",
					(uint8_t)(sizeof("404 send() FAIL") - 1));

				close(sock);
				break;
			}

			disconnect(sock);
			break;
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

	BaseType_t ok = xTaskCreate(net_task, "net_task", 2048, NULL, 2, NULL);

	if (ok != pdPASS) {

		logger_log_literal_len("NET INIT:",
			(uint8_t)(sizeof("NET INIT:") - 1),
			"TASK CREATE FAIL",
			(uint8_t)(sizeof("TASK CREATE FAIL") - 1));

		return -1;
	}

	return 0;
}
