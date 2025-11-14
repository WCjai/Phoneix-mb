/**
 * @file buttons.h
 * @brief Button debounce bookkeeping and utility.
 *
 * - Exposes debounced last-tick variables (g_btn1_last_tick, g_btn2_last_tick).
 * - clear_btn_p23_only(): convenience to clear only S2 bit from a status byte.
 *
 * Implementation detail:
 * - Debounce logic executed in the GPIO ISR uses RIT ticks.
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

#pragma once
#include <stdint.h>
#include "config.h"

extern volatile uint16_t g_btn1_last_tick;
extern volatile uint16_t g_btn2_last_tick;



#endif /* INC_BUTTONS_H_ */
