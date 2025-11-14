/*
 * ws_led.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 *
 *  Flicker-safe WS2812 driver glue:
 *   - Atomic hardware writes (mask IRQs during tight WS timing)
 *   - Optional coalescing via WS_MIN_FLUSH_TICKS (tick = 70 ms in your system)
 *   - No CMSIS dependency; uses small inline-asm helpers for PRIMASK
 */

#include "ws_led.h"
#include "ws2812b.h"
#include <string.h>

/* ===== Tuning knobs (can be overridden in config.h) ===================== */

#ifndef WS_LED_COUNT
#define WS_LED_COUNT 96
#endif

/* Minimum RIT ticks between actual flushes (0 = no throttling).
   With your RIT=70 ms, 2 ticks ≈ 140 ms. */
#ifndef WS_MIN_FLUSH_TICKS
#define WS_MIN_FLUSH_TICKS 0
#endif

/* If you physically drive only one strip, set to 0 to halve blocked time. */
#ifndef WS_HAS_STRIP2
#define WS_HAS_STRIP2 1
#endif

/* Provided by your scheduler (RIT tick counter). */
extern volatile uint32_t g_tick;

/* ===== Framebuffer & driver handles ===================================== */

static RGB_t   ws_buf[WS_LED_COUNT];
static WS2812B ws_b1 = { .leds = ws_buf, .num_leds = WS_LED_COUNT };
#if WS_HAS_STRIP2
static WS2812B ws_b2 = { .leds = ws_buf, .num_leds = WS_LED_COUNT };
#endif

/* ===== Local state ======================================================= */

static volatile uint8_t  s_ws_flush_pending = 0;   /* request from callers */
static volatile uint8_t  s_ws_dirty         = 0;   /* buffer changed since last flush */
static volatile uint16_t s_ws_last_led_bin1 = 0;   /* for ws_set_only_bin1() */

#if WS_MIN_FLUSH_TICKS > 0
static uint16_t s_ws_next_flush_tick = 0;          /* coalescing window */
#endif

/* ===== Minimal critical-section helpers (no CMSIS needed) =============== */

static inline uint32_t primask_save_and_disable(void){
    uint32_t primask;
    __asm volatile ("MRS %0, PRIMASK" : "=r"(primask) ::);
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}
static inline void primask_restore(uint32_t primask){
    if ((primask & 1u) == 0u){
        __asm volatile ("cpsie i" ::: "memory");
    }
}

/* Prevent any ISR from jittering the WS2812 bitstream.
   ~3 ms per 96 LEDs per strip @800 kHz. */
static inline void ws_write_atomic(void){
    uint32_t ps = primask_save_and_disable();

    WS2812B_write(&ws_b1);
#if WS_HAS_STRIP2
    WS2812B_write(&ws_b2);
#endif

    primask_restore(ps);
}

static inline void ws_mark_dirty_and_request_flush(void){
    s_ws_dirty = 1;
    s_ws_flush_pending = 1;
}

/* ===== Public API (matches ws_led.h) ==================================== */

void ws_init(void){
    memset(ws_buf, 0, sizeof(ws_buf));
    s_ws_last_led_bin1 = 0;
    s_ws_dirty = 1;
    s_ws_flush_pending = 1;
#if WS_MIN_FLUSH_TICKS > 0
    s_ws_next_flush_tick = (uint16_t)g_tick;  /* allow immediate first flush */
#endif
}

void ws_request_flush(void){
    s_ws_flush_pending = 1;
}

void ws_flush_if_pending(void){
    if (!s_ws_flush_pending) return;   /* nothing requested */
    if (!s_ws_dirty){                  /* no visual change */
        s_ws_flush_pending = 0;
        return;
    }

#if WS_MIN_FLUSH_TICKS > 0
    const uint16_t now = (uint16_t)g_tick;
    if ((int16_t)((uint16_t)now - s_ws_next_flush_tick) < 0){
        /* Too soon; keep pending & dirty so caller retries later */
        return;
    }
#endif

    /* Do the actual hardware write atomically */
    ws_write_atomic();

    /* Bookkeeping */
    s_ws_dirty = 0;
    s_ws_flush_pending = 0;

#if WS_MIN_FLUSH_TICKS > 0
    s_ws_next_flush_tick = (uint16_t)((uint16_t)g_tick + WS_MIN_FLUSH_TICKS);
#endif
}

void ws_clear_all(void){
    memset(ws_buf, 0, sizeof(ws_buf));
    s_ws_last_led_bin1 = 0;
    ws_mark_dirty_and_request_flush();
}

void ws_set_red_1indexed(uint16_t led1){
    if (!led1 || led1 > WS_LED_COUNT) return;
    const uint16_t idx = (uint16_t)(led1 - 1);
    const RGB_t old = ws_buf[idx];

    /* Skip if no visible change */
    if (old.r == 255 && old.g == 0 && old.b == 0) return;

    ws_buf[idx].r = 255; ws_buf[idx].g = 0; ws_buf[idx].b = 0;
    ws_mark_dirty_and_request_flush();
}

void ws_set_only_bin1(uint16_t led1){
    /* Turn off previous BIN1 LED if any */
    if (s_ws_last_led_bin1 && s_ws_last_led_bin1 <= WS_LED_COUNT){
        const uint16_t prev = (uint16_t)(s_ws_last_led_bin1 - 1);
        if (ws_buf[prev].r | ws_buf[prev].g | ws_buf[prev].b){
            ws_buf[prev].r = ws_buf[prev].g = ws_buf[prev].b = 0;
        }
    }
    s_ws_last_led_bin1 = led1;
    ws_set_red_1indexed(led1);  /* marks dirty & requests flush */
}

void ws_set_mask_bin1_and_clear_others(uint8_t max_led, const uint8_t *list){
    /* Clear all, then light listed (skip 0 entries) */
    memset(ws_buf, 0, sizeof(ws_buf));
    s_ws_last_led_bin1 = 0;

    for (uint8_t i = 0; i < max_led; ++i){
        const uint8_t l = list[i];
        if (l >= 1 && l <= WS_LED_COUNT){
            const uint16_t idx = (uint16_t)(l - 1);
            ws_buf[idx].r = 255; ws_buf[idx].g = 0; ws_buf[idx].b = 0;
        }
    }
    ws_mark_dirty_and_request_flush();
}
