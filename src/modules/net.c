#include "modules/net.h"
#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "ff.h"
#include "memutils.h"
#include "modules/logger.h"
#include "socket.h"
#include "task.h"

static const uint8_t http_socks[HTTP_SOCK_COUNT] = {0, 1, 2, 3};

static uint8_t rx_buf[SPI_MAX_XFER];

// hardcoded for now
// static uint8_t http_headers[] = "HTTP/1.1 200 OK\r\n"
//				"Content-Type: text/html\r\n"
//				"\r\n";

static uint8_t http_headers[] = "HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Connection: close\r\n"
				"\r\n";

static uint8_t http_404[] = "HTTP/1.1 404 Not Found\r\n"
			    "Content-Type: text/html\r\n"
			    "Connection: close\r\n"
			    "\r\n"
			    "<!DOCTYPE html>"
			    "<html><head><title>404 Not Found</title></head>"
			    "<body><h1>404 Not Found</h1>"
			    "<p>The requested resource could not be found on this server.</p>"
			    "</body></html>";

static const uint8_t lorem[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sodales varius sem quis "
	"porta. "
	"Sed tristique mollis molestie. Integer efficitur feugiat massa ac bibendum. Morbi ut urna "
	"et nisl "
	"dignissim ornare. Donec id pharetra lectus, sit amet mattis eros. Etiam nec sem nec orci "
	"sollicitudin viverra. Nam vitae varius ex, tristique cursus massa.\n"
	"Morbi et urna id magna bibendum sagittis. Donec non est eu massa lacinia convallis. Cras "
	"elementum "
	"venenatis libero, non lacinia enim luctus nec. Proin eget egestas velit. Vivamus in "
	"gravida ligula, "
	"sit amet euismod nulla. Ut placerat, lectus eu semper varius, mauris orci venenatis est, "
	"sed "
	"efficitur nisi sem ut libero. Aenean eleifend orci lacus, vitae accumsan nisi ullamcorper "
	"sit amet.\n"
	"Vivamus nec facilisis ex. Nullam in odio vitae nisi consectetur elementum. Maecenas eu "
	"dui hendrerit, "
	"vehicula elit ut, volutpat ipsum. Sed semper, enim at rhoncus semper, felis ligula "
	"aliquam nisl, et "
	"lobortis dolor erat non sapien. Proin eu leo elit. In et leo nunc. Vestibulum id turpis "
	"vel metus "
	"iaculis rutrum a eu lorem. Ut non laoreet purus. Donec semper turpis mauris, quis "
	"volutpat turpis "
	"elementum a.\n"
	"Ut ante dolor, porttitor et dignissim et, pulvinar in metus. Nunc porta dignissim justo, "
	"in tempor mi "
	"vestibulum rhoncus. Cras pulvinar neque ac dolor efficitur, nec laoreet risus vulputate. "
	"Sed posuere "
	"quis est quis pharetra. Sed vel semper dui. Mauris et varius libero. Etiam a bibendum "
	"magna. Aliquam "
	"luctus vel lorem sit amet ornare. Nullam luctus justo interdum nisi rhoncus, quis feugiat "
	"augue mollis. "
	"Donec ipsum ex, condimentum nec volutpat eu, bibendum vitae libero. Ut efficitur ante vel "
	"urna commodo, "
	"quis ultricies neque molestie. Vestibulum pellentesque mattis hendrerit. Nulla quis nisi "
	"neque. Nullam "
	"lacinia gravida semper. Donec ultricies risus nibh, ut aliquet turpis pulvinar vel. "
	"Quisque sit amet "
	"varius massa, ac ultrices est.\n"
	"Nullam posuere lobortis diam ac auctor. Vivamus molestie dolor diam, eget auctor turpis "
	"scelerisque ac. "
	"Nunc elementum dui hendrerit felis mattis, et ultrices neque ultricies. Nam sit amet "
	"turpis interdum, "
	"porttitor magna sed, venenatis est. Curabitur orci dolor, tempus id porta at, aliquet "
	"eget mi. Aenean "
	"fermentum purus cursus odio sollicitudin rhoncus. Suspendisse nec mi et turpis semper "
	"facilisis. Aenean "
	"lobortis tempor tortor sed sodales. Orci varius natoque penatibus et magnis dis "
	"parturient montes, "
	"nascetur ridiculus mus. Etiam nec iaculis felis. Donec erat libero, auctor vel dolor sit "
	"amet, ultrices "
	"sodales ligula. Sed malesuada tincidunt sem sed aliquam. Vivamus sed ligula ut dolor "
	"pulvinar eleifend. "
	"Proin vehicula quis quam at aliquet. Vestibulum eleifend neque quis dolor tempus, "
	"ultrices hendrerit "
	"arcu rhoncus.\n"
	"Ut eu odio commodo, bibendum orci vel, hendrerit arcu. Etiam et erat elit. Aliquam "
	"efficitur varius "
	"lacus sit amet dictum. Cras tincidunt orci lacus, at sodales dolor dapibus id. "
	"Pellentesque congue "
	"fermentum dolor. Pellentesque tincidunt egestas pulvinar. Sed a luctus tortor, vel "
	"sodales nunc.\n"
	"Etiam eleifend et ipsum in aliquam. Sed pretium posuere lacus et pellentesque. Vivamus "
	"mattis consequat "
	"vulputate. Proin ultrices diam id risus maximus feugiat. Integer hendrerit arcu leo, eget "
	"finibus neque "
	"sodales luctus. Suspendisse dapibus sed lacus a viverra. Aliquam elementum turpis "
	"condimentum risus "
	"efficitur convallis. Etiam consectetur venenatis rutrum. Orci varius natoque penatibus et "
	"magnis dis "
	"parturient montes, nascetur ridiculus mus. Sed placerat eleifend libero, et tempus velit "
	"mattis non. "
	"Etiam consequat ligula eros, aliquam iaculis erat aliquet vel. Morbi ac augue id leo "
	"feugiat maximus "
	"quis et lacus. Donec at mauris in dui faucibus viverra. Morbi blandit sed mi sit amet "
	"ornare. Praesent "
	"dui tellus, sollicitudin ac vestibulum id, malesuada ac neque.\n"
	"Nulla consectetur orci tellus, ac maximus est feugiat sed. Nullam interdum in massa eu "
	"tempor. Proin "
	"posuere massa id dolor feugiat, eget lacinia magna accumsan. Maecenas sit amet ornare "
	"ipsum. Ut vitae "
	"neque libero. Aenean at eros sed enim interdum imperdiet eget sit amet erat. Praesent "
	"placerat.\n"
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sodales varius sem quis "
	"porta. "
	"Sed tristique mollis molestie. Integer efficitur feugiat massa ac bibendum. Morbi ut urna "
	"et nisl "
	"dignissim ornare. Donec id pharetra lectus, sit amet mattis eros. Etiam nec sem nec orci "
	"sollicitudin viverra. Nam vitae varius ex, tristique cursus massa.\n"
	"Morbi et urna id magna bibendum sagittis. Donec non est eu massa lacinia convallis. Cras "
	"elementum "
	"venenatis libero, non lacinia enim luctus nec. Proin eget egestas velit. Vivamus in "
	"gravida ligula, "
	"sit amet euismod nulla. Ut placerat, lectus eu semper varius, mauris orci venenatis est, "
	"sed "
	"efficitur nisi sem ut libero. Aenean eleifend orci lacus, vitae accumsan nisi ullamcorper "
	"sit amet.\n"
	"Vivamus nec facilisis ex. Nullam in odio vitae nisi consectetur elementum. Maecenas eu "
	"dui hendrerit, "
	"vehicula elit ut, volutpat ipsum. Sed semper, enim at rhoncus semper, felis ligula "
	"aliquam nisl, et "
	"lobortis dolor erat non sapien. Proin eu leo elit. In et leo nunc. Vestibulum id turpis "
	"vel metus "
	"iaculis rutrum a eu lorem. Ut non laoreet purus. Donec semper turpis mauris, quis "
	"volutpat turpis "
	"elementum a.\n"
	"Ut ante dolor, porttitor et dignissim et, pulvinar in metus. Nunc porta dignissim justo, "
	"in tempor mi "
	"vestibulum rhoncus. Cras pulvinar neque ac dolor efficitur, nec laoreet risus vulputate. "
	"Sed posuere "
	"quis est quis pharetra. Sed vel semper dui. Mauris et varius libero. Etiam a bibendum "
	"magna. Aliquam "
	"luctus vel lorem sit amet ornare. Nullam luctus justo interdum nisi rhoncus, quis feugiat "
	"augue mollis. "
	"Donec ipsum ex, condimentum nec volutpat eu, bibendum vitae libero. Ut efficitur ante vel "
	"urna commodo, "
	"quis ultricies neque molestie. Vestibulum pellentesque mattis hendrerit. Nulla quis nisi "
	"neque. Nullam "
	"lacinia gravida semper. Donec ultricies risus nibh, ut aliquet turpis pulvinar vel. "
	"Quisque sit amet "
	"varius massa, ac ultrices est.\n"
	"Nullam posuere lobortis diam ac auctor. Vivamus molestie dolor diam, eget auctor turpis "
	"scelerisque ac. "
	"Nunc elementum dui hendrerit felis mattis, et ultrices neque ultricies. Nam sit amet "
	"turpis interdum, "
	"porttitor magna sed, venenatis est. Curabitur orci dolor, tempus id porta at, aliquet "
	"eget mi. Aenean "
	"fermentum purus cursus odio sollicitudin rhoncus. Suspendisse nec mi et turpis semper "
	"facilisis. Aenean "
	"lobortis tempor tortor sed sodales. Orci varius natoque penatibus et magnis dis "
	"parturient montes, "
	"nascetur ridiculus mus. Etiam nec iaculis felis. Donec erat libero, auctor vel dolor sit "
	"amet, ultrices "
	"sodales ligula. Sed malesuada tincidunt sem sed aliquam. Vivamus sed ligula ut dolor "
	"pulvinar eleifend. "
	"Proin vehicula quis quam at aliquet. Vestibulum eleifend neque quis dolor tempus, "
	"ultrices hendrerit "
	"arcu rhoncus.\n"
	"Ut eu odio commodo, bibendum orci vel, hendrerit arcu. Etiam et erat elit. Aliquam "
	"efficitur varius "
	"lacus sit amet dictum. Cras tincidunt orci lacus, at sodales dolor dapibus id. "
	"Pellentesque congue "
	"fermentum dolor. Pellentesque tincidunt egestas pulvinar. Sed a luctus tortor, vel "
	"sodales nunc.\n"
	"Etiam eleifend et ipsum in aliquam. Sed pretium posuere lacus et pellentesque. Vivamus "
	"mattis consequat "
	"vulputate. Proin ultrices diam id risus maximus feugiat. Integer hendrerit arcu leo, eget "
	"finibus neque "
	"sodales luctus. Suspendisse dapibus sed lacus a viverra. Aliquam elementum turpis "
	"condimentum risus "
	"efficitur convallis. Etiam consectetur venenatis rutrum. Orci varius natoque penatibus et "
	"magnis dis "
	"parturient montes, nascetur ridiculus mus. Sed placerat eleifend libero, et tempus velit "
	"mattis non. "
	"Etiam consequat ligula eros, aliquam iaculis erat aliquet vel. Morbi ac augue id leo "
	"feugiat maximus "
	"quis et lacus. Donec at mauris in dui faucibus viverra. Morbi blandit sed mi sit amet "
	"ornare. Praesent "
	"dui tellus, sollicitudin ac vestibulum id, malesuada ac neque.\n"
	"Nulla consectetur orci tellus, ac maximus est feugiat sed. Nullam interdum in massa eu "
	"tempor. Proin "
	"posuere massa id dolor feugiat, eget lacinia magna accumsan. Maecenas sit amet ornare "
	"ipsum. Ut vitae "
	"neque libero. Aenean at eros sed enim interdum imperdiet eget sit amet erat. Praesent "
	"placerat.";

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

		size_t req_len = 0;
		uint8_t req_fin = 0;

		for (;;) {
			if (getSn_SR(sock) != SOCK_ESTABLISHED) // state changed - exit
				break;

			uint16_t avail = getSn_RX_RSR(sock); // check for data received from client

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
				/* Req headers too large for the buffer*/
				disconnect(sock);
				break;
			}

			size_t remains = sizeof(rx_buf) - req_len;

			int32_t n = recv(sock,
				rx_buf + req_len, // append to the buffer
				(uint16_t)remains);

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

		if (req_len >= 6 && mem_cmp(rx_buf, (const uint8_t*)"GET / ", 6) == 0) {

			FIL file = {0};

			FRESULT fr = f_open(&file, "0:/INDEX.HTML", FA_READ);
			if (fr == FR_NO_FILE || fr == FR_NO_PATH) {
				// 404
				int32_t sr = send(sock, http_404, (uint16_t)(sizeof(http_404) - 1));
				if (sr < 0) {
					logger_log_literal_len("NET:",
						(uint8_t)(sizeof("NET:") - 1),
						"404 send() FAIL",
						(uint8_t)(sizeof("404 send() FAIL") - 1));
					close(sock); // hard recovery
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

			// send http headers
			int32_t sr = send(sock, http_headers, (uint16_t)(sizeof(http_headers) - 1));

			if (sr < 0) {
				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"headers send() FAIL",
					(uint8_t)(sizeof("headers send() FAIL") - 1));
				f_close(&file);
				close(sock); // hard recovery
				break;
			}

			// tmp
			size_t total = sizeof(lorem) - 1;
			size_t sent = 0;
			uint8_t failed = 0;
			uint32_t chunk_no = 0;

			while (sent < total) {
				size_t remaining = total - sent;
				uint16_t chunk =
					(uint16_t)((remaining > SPI_MAX_XFER) ? SPI_MAX_XFER
									      : remaining);

				chunk_no++;

				logger_log_uint_len("NET: chunk #",
					(uint8_t)(sizeof("NET: chunk #") - 1),
					&chunk_no,
					(uint8_t)sizeof(chunk_no));

				sr = send(sock, lorem + sent, chunk);

				if (sr < 0) {
					logger_log_literal_len("NET:",
						(uint8_t)(sizeof("NET:") - 1),
						"streaming send() FAIL",
						(uint8_t)(sizeof("streaming send() FAIL") - 1));

					logger_log_hex_len("NET: sock",
						(uint8_t)(sizeof("NET: sock") - 1),
						(uint8_t*)&sock,
						(uint8_t)sizeof(sock));

					logger_log_hex_len("NET: sr",
						(uint8_t)(sizeof("NET: sr") - 1),
						(uint8_t*)&sr,
						(uint8_t)sizeof(sr));

					failed = 1;
					break;
				}

				sent += (size_t)sr;
			}

			// file streaming
			//			uint8_t filedata[SPI_MAX_XFER];
			//			UINT br = 0;
			//			uint8_t failed = 0;

			//			for (;;) {
			//				// fr = f_read(&file, filedata,
			// sizeof(filedata), &br);
			//				// if (fr != FR_OK) {
			//				//	failed = 1;
			//				//	break;
			//				// }
			//
			//				// if (br == 0) {
			//				//	break; // EOF
			//				// }
			//				int32_t sockmode = getSn_MR(sock);
			//				logger_log_hex_len("NET: sockmode",
			//					(uint8_t)(sizeof("NET: sockmode") -
			// 1), 					(uint8_t*)&sockmode,
			// (uint8_t)sizeof(sockmode));
			//
			//				sr = send(sock, filedata, (uint16_t)br);
			//				if (sr < 0) {
			//					logger_log_literal_len("NET:",
			//						(uint8_t)(sizeof("NET:") -
			// 1), 						"streaming send() FAIL",
			// (uint8_t)(sizeof("streaming send() FAIL") - 1));
			//
			//					logger_log_hex_len("NET: sock, sr",
			//						(uint8_t)(sizeof("NET: sock,
			// sr") - 1), 						(uint8_t*)&sock,
			// (uint8_t)sizeof(sock));
			// logger_log_hex_len("NET: sock, sr",
			//						(uint8_t)(sizeof("NET: sock,
			// sr") - 1), 						(uint8_t*)&sr,
			// (uint8_t)sizeof(sr)); 					failed = 1;
			// break;
			//				}
			//
			//				// TODO: Handle partial send
			//			}
			//			f_close(&file);

			if (failed) {
				close(sock);
				break;
			}

			disconnect(sock);
		} else {
			// 404

			int32_t sr = send(sock, http_404, (uint16_t)(sizeof(http_404) - 1));

			if (sr < 0) {
				logger_log_literal_len("NET:",
					(uint8_t)(sizeof("NET:") - 1),
					"404 send() FAIL",
					(uint8_t)(sizeof("404 send() FAIL") - 1));
				close(sock); // hard recovery
				break;
			}

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
