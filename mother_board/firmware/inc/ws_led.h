/**
 * @file ws_led.h
 * @brief WS2812 (NeoPixel) framebuffer + flush control (no ISR writes).
 *
 * - In-memory RGB buffer for WS strip(s).
 * - High-level ops: clear all, set one LED, set "only bin1" LED,
 *                   set a mask list and clear others.
 * - Flush is *requested* (cheap) from anywhere; actual I/O happens in main.
 *
 * Threading: Functions are non-blocking; hardware write occurs via
 * ws_flush_if_pending() in the main loop to keep ISRs short.
 */

#ifndef INC_WS_LED_H_
#define INC_WS_LED_H_

#pragma once
#include <stdint.h>
#include "config.h"

// Initialize (optional clear)
void ws_init(void);

// Set/clear pixel buffer; flushing is requested then performed in main
void ws_clear_all(void);
void ws_set_red_1indexed(uint16_t led1);
void ws_set_only_bin1(uint16_t led1);
void ws_set_mask_bin1_and_clear_others(uint8_t max_led, const uint8_t *list);

void ws_request_flush(void);
void ws_flush_if_pending(void);

#endif /* INC_WS_LED_H_ */
