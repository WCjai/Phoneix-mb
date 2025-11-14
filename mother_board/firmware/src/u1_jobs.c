/*
 * u1_jobs.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "u1_jobs.h"
#include "queues.h"
#include "proto.h"
#include "sched.h"

volatile U1Job g_u1_jobs[MAX_U1_JOBS];
uint8_t u1_jobs_rr = 0;
volatile bool g_led_streaming_active = false;

static inline void slave_enqueue_led_on(uint8_t con, uint8_t led){
    uint8_t f[9] = { SOF, GRP_RX_TO_SLV, 0x05, SC_SLAVE, con, 0x02, 0x01, led, END_BYTE };
    (void)u1q_push_isr(f, sizeof f);
}

void u1_jobs_clear_all(void){
    for (uint8_t i=0;i<MAX_U1_JOBS;++i) g_u1_jobs[i].active = 0;
}
void u1_jobs_remove_by_con_except(uint8_t con, uint8_t keep_led){
    for (uint8_t i=0;i<MAX_U1_JOBS;++i)
        if (g_u1_jobs[i].active && g_u1_jobs[i].con==con && g_u1_jobs[i].led!=keep_led)
            g_u1_jobs[i].active = 0;
}
uint8_t u1_job_find(uint8_t con, uint8_t led){
    for (uint8_t i=0;i<MAX_U1_JOBS;++i)
        if (g_u1_jobs[i].active && g_u1_jobs[i].con==con && g_u1_jobs[i].led==led) return i;
    return 0xFF;
}
uint8_t u1_job_alloc(uint8_t con, uint8_t led){
    for (uint8_t i=0;i<MAX_U1_JOBS;++i)
        if (!g_u1_jobs[i].active){ g_u1_jobs[i]=(U1Job){con,led,(uint16_t)g_tick,1}; return i; }
    return 0xFF;
}

bool u1_scheduler_emit_one(void){
    // Are there any active jobs?
    g_led_streaming_active = false;
    for (uint8_t i=0;i<MAX_U1_JOBS;++i){ if (g_u1_jobs[i].active){ g_led_streaming_active = true; break; } }
    if (!g_led_streaming_active) return false;

    for (uint8_t k=0;k<MAX_U1_JOBS;++k){
        const uint8_t i = (uint8_t)((u1_jobs_rr + k) % MAX_U1_JOBS);
        if (!g_u1_jobs[i].active) continue;
        const bool time_ok = ((int16_t)((uint16_t)g_tick - g_u1_jobs[i].next_allowed_tick) >= 0);
        if (time_ok){
            slave_enqueue_led_on(g_u1_jobs[i].con, g_u1_jobs[i].led);
            g_u1_jobs[i].next_allowed_tick = (uint16_t)(g_tick + 1); // LED_JOB_MIN_PERIOD_TICKS
            u1_jobs_rr = (uint8_t)((i + 1) % MAX_U1_JOBS);
            return true;
        }
    }
    return false;
}

