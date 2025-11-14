/**
 * @file u2_jobs.h
 * @brief UART2 (BIN) LED streaming jobs and helpers (including multi-mask).
 *
 * - U2Job: (bin, led, next_allowed_tick, active).
 * - Start/stop/de-dup helpers for per-LED streaming.
 * - u2_scheduler_emit_one(): RIT emits one BIN LED-ON per tick if due.
 * - Frame helpers:
 *     bin_enqueue_led_on_uart2(), bin_enqueue_led_off_broadcast_uart2(),
 *     bin_enqueue_multi_mask_uart2() for compact batch updates.
 *
 * Threading:
 * - Push to the UART2 TX queue is ISR-safe; actual TX occurs in main loop.
 */

#ifndef INC_U2_JOBS_H_
#define INC_U2_JOBS_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef struct { uint8_t bin, led; uint16_t next_allowed_tick; volatile uint8_t active; } U2Job;

extern volatile U2Job g_u2_jobs[MAX_U2_JOBS];
extern uint8_t       u2_jobs_rr;

void    u2_job_start(uint8_t bin, uint8_t led);
void    u2_jobs_stop_by_led(uint8_t led);
void    u2_jobs_stop_all(void);
void    u2_jobs_remove_by_bin_except(uint8_t bin, uint8_t keep_led);
uint8_t u2_job_find(uint8_t bin, uint8_t led);

// Called from RIT: enqueues one BIN frame if time_ok, advances RR
bool    u2_scheduler_emit_one(void);

// Frame helpers used by ISR/RIT
void bin_enqueue_led_on_uart2(uint8_t bin, uint8_t led);
void bin_enqueue_led_off_broadcast_uart2(void);
void bin_enqueue_multi_mask_uart2(uint8_t max_led, const uint8_t *list);

#endif /* INC_U2_JOBS_H_ */
