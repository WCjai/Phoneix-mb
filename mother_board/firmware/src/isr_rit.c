/*
 * isr_rit.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "sched.h"
#include "queues.h"
#include "u1_jobs.h"
#include "u2_jobs.h"
#include "ws_led.h"
#include "proto.h"
#include "config.h"
#include "chip.h"

volatile uint32_t g_tick = 0;
volatile uint16_t g_app_last_activity_tick = 0;
volatile uint8_t  g_idle_ws_cleared = 0;

volatile uint32_t g_alive_mask=0, g_triggered_mask=0;
volatile uint32_t round_alive_mask=0, round_triggered_mask=0;

volatile uint8_t  g_off_broadcast_pending=0;
volatile uint8_t  g_off_broadcast2_pending=0;

static uint8_t poll_rr_idx=0;
static bool led_active_prev=false;

static inline void slave_enqueue_poll(uint8_t con){
    uint8_t f[9] = { SOF, GRP_RX_TO_SLV, 0x05, SC_SLAVE, con, 0x00, 0x00, 0x00, END_BYTE };
    (void)u1q_push_isr(f, sizeof f);
}
static inline void slave_enqueue_led_off_broadcast(void){
    uint8_t f[8] = { SOF, GRP_RX_TO_SLV, 0x04, SC_SLAVE, 0xFF, 0x03, 0x00, END_BYTE };
    (void)u1q_push_isr(f, sizeof f);
}

void sched_commit_and_clear_poll_round(void){
    g_alive_mask     = round_alive_mask;
    g_triggered_mask = round_triggered_mask & g_alive_mask;
    round_alive_mask=0; round_triggered_mask=0;
}

extern uint8_t cfg_conn[MAX_CFG];
extern uint8_t cfg_count;

void RIT_IRQHandler(void){
    Chip_RIT_ClearInt(LPC_RITIMER);
    g_tick++;

    if (g_off_broadcast_pending){  slave_enqueue_led_off_broadcast();      g_off_broadcast_pending=0; }
    if (g_off_broadcast2_pending){ bin_enqueue_led_off_broadcast_uart2();  g_off_broadcast2_pending=0; }

    const bool app_idle = ((int16_t)((uint16_t)g_tick - g_app_last_activity_tick) >= (int16_t)APP_IDLE_TICKS);
    if (app_idle){
        slave_enqueue_led_off_broadcast();
        bin_enqueue_led_off_broadcast_uart2();
        if (!g_idle_ws_cleared){ ws_clear_all(); g_idle_ws_cleared = 1; }
        ws_request_flush();
        return;
    }
    g_idle_ws_cleared = 0;

    if (u1_scheduler_emit_one()){
        led_active_prev = true;
        ws_request_flush();  // WS may have changed in LED CTRL
    } else {
        if (led_active_prev){ led_active_prev=false; poll_rr_idx=0; }
        if (cfg_count){
            if (poll_rr_idx == 0) sched_commit_and_clear_poll_round();
            const uint8_t con = cfg_conn[poll_rr_idx];
            slave_enqueue_poll(con);
            if (++poll_rr_idx >= cfg_count) poll_rr_idx=0;
        }
        ws_request_flush();
    }

    (void)u2_scheduler_emit_one(); // one BIN job per tick
}

