/**
 * @file isr_uart0.h
 * @brief UART0 (App→RX) byte-stream ISR declaration.
 *
 * - Parses framed commands (SOF/LEN/BODY/END).
 * - Dispatches per-SC handlers:
 *     upload_map, led_ctrl (modes 0/1/2), led_reset, status01_once,
 *     relay_set, btnflag_reset, led1_multi_con, bin_led_mask.
 * - Calls request_status_reply() after handling each valid App frame.
 *
 * TX to App is performed in the main loop by reading the prepared buffer
 * from app_status.h.
 */

#ifndef INC_ISR_UART0_H_
#define INC_ISR_UART0_H_

#pragma once
void UART0_IRQHandler(void);

#endif /* INC_ISR_UART0_H_ */
