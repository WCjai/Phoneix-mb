/**
 * @file sched.h
 * @brief Global scheduler timing/state shared by RIT and handlers.
 *
 * Exposed timing/state:
 * - g_tick: RIT tick counter (monotonic).
 * - g_app_last_activity_tick: last time App (UART0) was active.
 * - app idle flags: g_idle_ws_cleared to avoid redundant clears.
 *
 * Alive/trigger bitmasks:
 * - round_* masks: accumulators per polling round (UART1 replies).
 * - g_* masks:     "current view" used for status reporting.
 *
 * Control flags:
 * - g_off_broadcast_pending, g_off_broadcast2_pending: request OFF frames.
 *
 * API:
 * - sched_commit_and_clear_poll_round(): snapshot round_* into g_*.
 */

#ifndef INC_SCHED_H_
#define INC_SCHED_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>

extern volatile uint32_t g_tick;
extern volatile uint16_t g_app_last_activity_tick;
extern volatile uint8_t  g_idle_ws_cleared;

extern volatile uint32_t g_alive_mask, g_triggered_mask;
extern volatile uint32_t round_alive_mask, round_triggered_mask;

extern volatile uint8_t  g_off_broadcast_pending;
extern volatile uint8_t  g_off_broadcast2_pending;

void sched_commit_and_clear_poll_round(void);

#endif /* INC_SCHED_H_ */
