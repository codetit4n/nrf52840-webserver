#pragma once

#include "nrf52840.h"

/*
 * Board support header (minimal).
 * NOTE:
 * - This file is "board" level, not "chip" level.
 *   The chip-level registers/definitions come from nrf52840.h (MDK).
 */

/* ----------------------------- */
/* Board identity                */
/* ----------------------------- */
#define BOARD_NAME "nRF52840-DK"

/* ----------------------------- */
/* LED mapping                   */
/* ----------------------------- */
/*
 * nRF52840 DK:
 * - LED1 is P0.13 and is active-low (0 = ON, 1 = OFF).
 *
 */
#define BOARD_LED1_PORT        NRF_P0
#define BOARD_LED1_PIN         13
#define BOARD_LED1_ACTIVE_LOW  1

/* Helper macros so code reads clearly */
#define BOARD_PIN_MASK(pin)    (1UL << (pin))

/* LED ON/OFF helpers (write-through, no branching in user code) */
static inline void board_led1_init(void)
{
    /* Configure as output */
    BOARD_LED1_PORT->DIRSET = BOARD_PIN_MASK(BOARD_LED1_PIN);

    /* Default OFF */
#if BOARD_LED1_ACTIVE_LOW
    BOARD_LED1_PORT->OUTSET = BOARD_PIN_MASK(BOARD_LED1_PIN);
#else
    BOARD_LED1_PORT->OUTCLR = BOARD_PIN_MASK(BOARD_LED1_PIN);
#endif
}

static inline void board_led1_on(void)
{
#if BOARD_LED1_ACTIVE_LOW
    BOARD_LED1_PORT->OUTCLR = BOARD_PIN_MASK(BOARD_LED1_PIN);
#else
    BOARD_LED1_PORT->OUTSET = BOARD_PIN_MASK(BOARD_LED1_PIN);
#endif
}

static inline void board_led1_off(void)
{
#if BOARD_LED1_ACTIVE_LOW
    BOARD_LED1_PORT->OUTSET = BOARD_PIN_MASK(BOARD_LED1_PIN);
#else
    BOARD_LED1_PORT->OUTCLR = BOARD_PIN_MASK(BOARD_LED1_PIN);
#endif
}

static inline void board_led1_toggle(void)
{
    BOARD_LED1_PORT->OUT ^= BOARD_PIN_MASK(BOARD_LED1_PIN);
}

