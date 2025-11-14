/**
 * @file queues.h
 * @brief Lock-free single-producer/single-consumer TX rings for UART1/2.
 *
 * - U1Frame (small fixed-size) and U2Frame (larger, up to 192 bytes).
 * - ISR-safe push:  u1q_push_isr(), u2q_push_isr()  (no malloc, non-blocking).
 * - Main-loop pop:  u1q_pop_main(),  u2q_pop_main()  (drained and sent).
 * - Drop counters:  u1_drops, u2_drops for diagnostics.
 *
 * Design: ISRs **only push**, main loop **only pops**.
 */

#ifndef INC_QUEUES_H_
#define INC_QUEUES_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// UART1 TX ring (frames to slaves)
typedef struct { uint8_t data[9];   uint8_t len; } U1Frame;
bool u1q_push_isr(const uint8_t *d, uint8_t n);
bool u1q_pop_main(U1Frame *out);
extern volatile uint32_t u1_drops;

// UART2 TX ring (frames to BIN)
typedef struct { uint8_t data[192]; uint8_t len; } U2Frame;
bool u2q_push_isr(const uint8_t *d, uint8_t n);
bool u2q_pop_main(U2Frame *out);
extern volatile uint32_t u2_drops;


#endif /* INC_QUEUES_H_ */
