#pragma once

#include "nrf52840.h"

#define BOARD_NAME "nRF52840-DK"

#define REG32(addr) (*(volatile uint32_t*)(addr))

// working with just one gpio port for now
#define GPIO_PORT NRF_P0
#define GPIO_CNF(pin) REG32(GPIO_PORT->PIN_CNF[pin])
#define GPIO_OUTSET REG32(GPIO_PORT + GPIO_PORT->OUTSET)
#define GPIO_OUTCLR REG32(GPIO_PORT + GPIO_PORT->OUTCLR)

// working with just one spim for now
#define SPIM_BASE NRF_SPIM0
#define SPIM_CONFIG REG32(SPIM_BASE + SPIM_BASE->CONFIG)
#define SPIM_FREQUENCY REG32(SPIM_BASE + SPIM_BASE->FREQUENCY)
#define SPIM_ENABLE REG32(SPIM_BASE + SPIM_BASE->ENABLE)
#define SPIM_EVENTS_END REG32(SPIM_BASE + SPIM_BASE->EVENTS_END)
#define SPIM_TXD_PTR REG32(SPIM_BASE + SPIM_BASE->TXD.PTR)
#define SPIM_TXD_MAXCNT REG32(SPIM_BASE + SPIM_BASE->TXD.MAXCNT)
#define SPIM_RXD_PTR REG32(SPIM_BASE + SPIM_BASE->RXD.PTR)
#define SPIM_RXD_MAXCNT REG32(SPIM_BASE + SPIM_BASE->RXD.MAXCNT)
#define SPIM_TASKS_START REG32(SPIM_BASE + SPIM_BASE->TASKS_START)

#define SPIM_PSEL_SCK REG32(SPIM_BASE + SPIM_BASE->PSEL.SCK)
#define SPIM_PSEL_MOSI REG32(SPIM_BASE + SPIM_BASE->PSEL.MOSI)
#define SPIM_PSEL_MISO REG32(SPIM_BASE + SPIM_BASE->PSEL.MISO)

// Note: These pins work well with the nRF52840-DK
// I have found that many other P0 pins either:
// - are connected to on-board peripherals (LEDs, buttons, NFC, crystals),
// - are routed through level-shifters or jumpers,
// - or show signal integrity issues on long jumper wires.
//
// P0.02, P0.26, P0.27, and P0.30 are free, directly routed GPIOs on the DK
// and produce clean SPI waveforms on a logic analyzer.

#define SCK_PIN 2
#define MOSI_PIN 27
#define MISO_PIN 30

