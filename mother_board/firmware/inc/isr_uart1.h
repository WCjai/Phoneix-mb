/**
 * @file isr_uart1.h
 * @brief UART1 (Slave→RX) byte-stream ISR declaration.
 *
 * - Parses SC_STATUS replies from slaves (address, state).
 * - Updates round_alive_mask / round_triggered_mask.
 * - If streaming is active, also mirrors into g_alive_mask / g_triggered_mask.
 *
 * The actual polling frames (and LED-ON frames) are queued by the RIT
 * scheduler via u1_jobs and queues modules.
 */

#ifndef INC_ISR_UART1_H_
#define INC_ISR_UART1_H_

#pragma once
void UART1_IRQHandler(void);

#endif /* INC_ISR_UART1_H_ */
