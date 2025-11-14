/**
 * @file isr_rit.h
 * @brief RIT (timer tick) ISR declaration; central scheduler.
 *
 * Duties every tick:
 * - Handle OFF broadcasts requested by other modules.
 * - Check App idle watchdog; on idle ⇒ broadcast OFF, clear WS, pause work.
 * - If LED jobs active ⇒ emit one UART1 LED-ON (u1_scheduler_emit_one()).
 * - Else ⇒ run round-robin poll over configured connectors; at start of a
 *   new poll cycle, commit last round's masks via sched_commit_and_clear_poll_round().
 * - Emit at most one UART2 BIN LED-ON per tick (u2_scheduler_emit_one()).
 * - Request WS flush when needed; actual flush is in main loop.
 *
 * Keeps ISRs short by only queuing frames; main loop performs UART TX.
 */

#ifndef INC_ISR_RIT_H_
#define INC_ISR_RIT_H_

#pragma once
void RIT_IRQHandler(void);

#endif /* INC_ISR_RIT_H_ */
