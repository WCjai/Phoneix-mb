/**
 * @file isr_gpio.h
 * @brief GPIO interrupt handler declaration (EINT3 on LPC17xx).
 *
 * - Debounced, falling-edge detection for S1 (bit0) and S2 (bit1).
 * - Sets g_status_ext bits; requests a status reply to the App so the
 *   press is visible immediately.
 *
 * Vector name note: Ensure this symbol matches your vector table
 * (e.g., EINT3_IRQHandler on LPC17xx).
 */

#ifndef INC_ISR_GPIO_H_
#define INC_ISR_GPIO_H_

#pragma once
void GPIO_IRQ_HANDLER(void);


#endif /* INC_ISR_GPIO_H_ */
