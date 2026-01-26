#include "FreeRTOS.h"
#include "board.h"
#include "projdefs.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t*)(addr))

#define NRF_SPIM0_BASE 0x40003000UL
#define NRF_P0_BASE 0x50000000UL

#define SPIM0_CONFIG REG32(NRF_SPIM0_BASE + 0x554)
#define SPIM0_ENABLE REG32(NRF_SPIM0_BASE + 0x500)
#define SPIM0_FREQUENCY REG32(NRF_SPIM0_BASE + 0x524)
#define SPIM0_TASKS_START REG32(NRF_SPIM0_BASE + 0x010)
#define SPIM0_EVENTS_END REG32(NRF_SPIM0_BASE + 0x118)
#define SPIM0_TXD_PTR REG32(NRF_SPIM0_BASE + 0x544)
#define SPIM0_TXD_MAXCNT REG32(NRF_SPIM0_BASE + 0x548)
#define SPIM0_RXD_PTR REG32(NRF_SPIM0_BASE + 0x534)
#define SPIM0_RXD_MAXCNT REG32(NRF_SPIM0_BASE + 0x538)

#define PSEL_SCK REG32(NRF_SPIM0_BASE + 0x508)
#define PSEL_MOSI REG32(NRF_SPIM0_BASE + 0x50C)
#define PSEL_MISO REG32(NRF_SPIM0_BASE + 0x510)

// Note: These pins work well with the nRF52840-DK
// I have found that many other P0 pins either:
// - are connected to on-board peripherals (LEDs, buttons, NFC, crystals),
// - are routed through level-shifters or jumpers,
// - or show signal integrity issues on long jumper wires.
//
// P0.02, P0.26, P0.27, and P0.30 are free, directly routed GPIOs on the DK
// and produce clean SPI waveforms on a logic analyzer.
#define CSN_PIN 26
#define SCK_PIN 2
#define MOSI_PIN 27
#define MISO_PIN 30

// P0 pins only
#define P0_CNF(pin) REG32(NRF_P0_BASE + 0x700UL + 4UL * (pin))
#define P0_OUTSET REG32(NRF_P0_BASE + 0x508)
#define P0_OUTCLR REG32(NRF_P0_BASE + 0x50C)

static void csn_low(void) {
	P0_OUTCLR = (1 << CSN_PIN);
}

static void csn_high(void) {
	P0_OUTSET = (1 << CSN_PIN);
}

static void init_spim0(void) {

	/* ---------------- CSN (manual GPIO) ----------------
	 * Output, input buffer disconnected, no pull
	 */
	P0_CNF(CSN_PIN) = (1 << 0) | // DIR = 1 → Output
			  (1 << 1) | // INPUT = 1 → Disconnect input buffer
			  (0 << 2) | // PULL = 00 → Disabled
			  (0 << 8) | // DRIVE = 000 → Standard drive (S0S1)
			  (0 << 16); // SENSE = Disabled

	csn_high(); // CS idle high (active-low)

	/* ---------------- SPIM configuration ----------------
	 * Mode 0: CPOL = 0, CPHA = 0
	 * MSB first
	 */
	SPIM0_CONFIG = (0 << 0) | // CPHA = 0
		       (0 << 1) | // CPOL = 0
		       (0 << 2);  // ORDER = 0 → MSB first

	/* 125 kHz SPI clock */
	SPIM0_FREQUENCY = 0x02000000UL;

	/* ---------------- SCK pin ----------------
	 * Output driven by SPIM
	 */
	PSEL_SCK = (0 << 31) | // CONNECT = 0 → Connected
		   (0 << 5) |  // PORT = 0 → P0
		   (SCK_PIN << 0);

	P0_CNF(SCK_PIN) = (1 << 0) | // DIR = Output
			  (1 << 1) | // INPUT = Disconnect
			  (0 << 2) | // No pull
			  (0 << 8) | // Standard drive
			  (0 << 16); // No sense

	/* ---------------- MOSI pin ----------------
	 * Output driven by SPIM
	 */
	PSEL_MOSI = (0 << 31) | (0 << 5) | (MOSI_PIN << 0);

	P0_CNF(MOSI_PIN) = (1 << 0) | // Output
			   (1 << 1) | // Disconnect input
			   (0 << 2) | (0 << 8) | (0 << 16);

	/* ---------------- MISO pin ----------------
	 * Input to SPIM
	 */
	PSEL_MISO = (0 << 31) | (0 << 5) | (MISO_PIN << 0);

	P0_CNF(MISO_PIN) = (0 << 0) | // DIR = Input
			   (0 << 1) | // INPUT = Connected
			   (0 << 2) | // No pull (depends on slave)
			   (0 << 8) | (0 << 16);

	/* ---------------- Enable SPIM0 ----------------
	 * Value 7 = Enabled
	 */
	SPIM0_ENABLE = 7;
}

static void spi_xfer_blocking(uint8_t* tx_buf, uint8_t* rx_buf, size_t len) {

	// clear end event
	SPIM0_EVENTS_END = 0;

	// program DMA
	SPIM0_TXD_PTR = (uintptr_t)tx_buf;
	SPIM0_TXD_MAXCNT = len;
	SPIM0_RXD_PTR = (uintptr_t)rx_buf;
	SPIM0_RXD_MAXCNT = len;

	// start spi txn
	csn_low();
	SPIM0_TASKS_START = 1;

	// Wait for completion
	while (SPIM0_EVENTS_END == 0) {
		taskYIELD(); // let's other tasks run
	}

	// end transaction
	SPIM0_EVENTS_END = 0;
	csn_high();
}

static void led_task(void* arg) {
	(void)arg;

	for (;;) {
		board_led1_toggle();
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

static uint8_t tx_buf[] = {0x00, 0x39, 0x00, 0x00}; // Read VERSIONR register at offset 0x0039
static uint8_t rx_buf[sizeof(tx_buf)];

static void process_rx(const uint8_t* rx_buf, size_t len) {
	for (size_t i = 0; i < len; i++) {
		// Put a breakpoint on this line to see rx_buf
		volatile uint8_t byte = rx_buf[i];
		(void)byte;
	}
}

static void spi_task(void* arg) {
	(void)arg;

	for (;;) {
		spi_xfer_blocking(tx_buf, rx_buf, sizeof(tx_buf));
		process_rx(rx_buf, sizeof(rx_buf));

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

int main(void) {

	board_led1_init();
	init_spim0();

	/* Create LED task */
	BaseType_t ok;
	ok = xTaskCreate(led_task, /* Task function */
			 "LED",	   /* Name (for debug) */
			 128,	   /* Stack size (words, not bytes) */
			 NULL,	   /* Parameters */
			 1,	   /* Priority */
			 NULL	   /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	ok = xTaskCreate(spi_task, /* Task function */
			 "SPI",	   /* Name (for debug) */
			 256,	   /* Stack size (words, not bytes) */
			 NULL,	   /* Parameters */
			 2,	   /* Priority */
			 NULL	   /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	/* Should never reach here */
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}
