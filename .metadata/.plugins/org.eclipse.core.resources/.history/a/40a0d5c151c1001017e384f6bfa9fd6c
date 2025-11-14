/*
 * ws_led.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "ws_led.h"
#include "ws2812b.h"
#include <string.h>

static RGB_t   ws_buf[WS_LED_COUNT];
static WS2812B ws_b1 = { .leds = ws_buf, .num_leds = WS_LED_COUNT };
static WS2812B ws_b2 = { .leds = ws_buf, .num_leds = WS_LED_COUNT };

static volatile uint8_t  s_ws_flush_pending = 0;
static volatile uint16_t s_ws_last_led_bin1 = 0;

void ws_init(void){ memset(ws_buf, 0, sizeof(ws_buf)); }
void ws_request_flush(void){ s_ws_flush_pending = 1; }

void ws_flush_if_pending(void){
    if (!s_ws_flush_pending) return;
    WS2812B_write(&ws_b1);
    WS2812B_write(&ws_b2);
    s_ws_flush_pending = 0;
}
void ws_clear_all(void){
    memset(ws_buf, 0, sizeof(ws_buf));
    ws_request_flush();
}
void ws_set_red_1indexed(uint16_t led1){
    if (!led1 || led1 > WS_LED_COUNT) return;
    ws_buf[led1-1].r=255; ws_buf[led1-1].g=0; ws_buf[led1-1].b=0;
    ws_request_flush();
}
void ws_set_only_bin1(uint16_t led1){
    if (s_ws_last_led_bin1 && s_ws_last_led_bin1 <= WS_LED_COUNT){
        RGB_t *px = &ws_buf[s_ws_last_led_bin1-1]; px->r=px->g=px->b=0;
    }
    s_ws_last_led_bin1 = led1;
    ws_set_red_1indexed(led1);
}
void ws_set_mask_bin1_and_clear_others(uint8_t max_led, const uint8_t *list){
    memset(ws_buf, 0, sizeof(ws_buf));
    for (uint8_t i=0;i<max_led;++i){
        uint8_t l = list[i];
        if (l>=1 && l<=WS_LED_COUNT){
            ws_buf[l-1].r=255; ws_buf[l-1].g=0; ws_buf[l-1].b=0;
        }
    }
    ws_request_flush();
}

