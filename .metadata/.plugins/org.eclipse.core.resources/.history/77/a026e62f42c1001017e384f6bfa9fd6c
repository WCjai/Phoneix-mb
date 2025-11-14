#include "buttons.h"
#include "sched.h"
#include "config.h"
#include "app_status.h"   // for request_status_reply()
#include "chip.h"

volatile uint16_t g_btn1_last_tick = 0;
volatile uint16_t g_btn2_last_tick = 0;

// Define these in main as macros to your actual pins/port
#ifndef GPIO_BUTTON_PORT
#define GPIO_BUTTON_PORT 2
#endif
#ifndef GPIO_BUTTON_S1_PIN
#define GPIO_BUTTON_S1_PIN 4  // P2.4
#endif
#ifndef GPIO_BUTTON_S2_PIN
#define GPIO_BUTTON_S2_PIN 3  // P2.3
#endif

extern volatile uint8_t g_status_ext;

void GPIO_IRQ_HANDLER(void){
    // Clear & read latched edges
    uint32_t stat_f = Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, GPIOINT_PORT2);
    if (stat_f) Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, stat_f);
    uint32_t stat_r = Chip_GPIOINT_GetStatusRising (LPC_GPIOINT, GPIOINT_PORT2);
    if (stat_r) Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT2, stat_r);

    uint16_t now = (uint16_t)g_tick;

    // P2.4 => S1 (bit0)
    if (stat_f & (1u << GPIO_BUTTON_S1_PIN)) {
        if (!Chip_GPIO_GetPinState(LPC_GPIO, GPIO_BUTTON_PORT, GPIO_BUTTON_S1_PIN)) {
            if ((int16_t)((uint16_t)now - g_btn1_last_tick) >= (int16_t)BTN_DEBOUNCE_TICKS) {
                g_btn1_last_tick = now;
                g_status_ext |= BTN_P24_BIT;
                request_status_reply();   // send heartbeat to PC reflecting S1 press
            }
        }
    }

    // P2.3 => S2 (bit1)
    if (stat_f & (1u << GPIO_BUTTON_S2_PIN)) {
        if (!Chip_GPIO_GetPinState(LPC_GPIO, GPIO_BUTTON_PORT, GPIO_BUTTON_S2_PIN)) {
            if ((int16_t)((uint16_t)now - g_btn2_last_tick) >= (int16_t)BTN_DEBOUNCE_TICKS) {
                g_btn2_last_tick = now;
                g_status_ext |= BTN_P23_BIT;
                request_status_reply();   // send heartbeat to PC reflecting S2 press
            }
        }
    }
}
