/*
 * app_status.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "app_status.h"
#include "proto.h"

volatile uint8_t  g_status_ext = 0x00;
volatile uint32_t g_status01_mask = 0;
volatile uint32_t g_force01_while_triggered_mask = 0;

uint8_t  cfg_conn[MAX_CFG];
uint8_t  cfg_count = 0;

// Prepared status buffer (sent by main)
static volatile uint8_t g_tx_len = 0;
static uint8_t          g_tx_buf[TX_FRAME_MAX];

// From scheduler (alive/trigger state)
extern volatile uint32_t g_alive_mask, g_triggered_mask;
extern volatile uint32_t round_alive_mask, round_triggered_mask;

static uint8_t build_status_frame(uint8_t *dst, uint8_t cap){
    const uint8_t N   = cfg_count;
    const uint8_t LEN = (uint8_t)(N + 7);
    const uint8_t TOT = (uint8_t)(LEN + 3);
    if (TOT > cap) return 0;

    const uint32_t view_alive = (g_alive_mask | round_alive_mask);
    const uint32_t view_trig  = ((g_triggered_mask | round_triggered_mask) & view_alive);

    uint8_t *p = dst;
    *p++=SOF; *p++=LEN; *p++=GRP_RX_TO_APP; *p++=RX_ID; *p++=SC_STATUS;
    *p++=g_status_ext; *p++=N;

    for (uint8_t i=0;i<N;++i){
        const uint8_t  c = cfg_conn[i];
        const uint32_t b = (c>=1 && c<=31)?(1u<<(c-1)):0;

        uint8_t Si=0x00;
        const uint8_t alive = (view_alive & b)!=0;
        const uint8_t trig  = (view_trig  & b)!=0;

        if ((g_status01_mask == 0xFFFFFFFFu) || (b && (g_status01_mask & b))) {
            Si = 0x01;
        } else if (g_force01_while_triggered_mask & b) {
            if (trig) {
                Si = 0x01;
            } else {
                g_force01_while_triggered_mask &= ~b;
                Si = alive ? 0x05 : 0x00;
            }
        } else {
            Si = alive ? (trig ? 0x07 : 0x05) : 0x00;
        }
        *p++=Si;
    }
    *p++=0x00; *p++=0x00; *p++=END_BYTE;
    return TOT;
}

void request_status_reply(void){
    g_tx_len = build_status_frame(g_tx_buf, sizeof g_tx_buf);
    if (g_tx_len) g_status01_mask = 0; // one-shot cleared after preparing
}

size_t app_status_peek_len(void){ return g_tx_len; }
const uint8_t* app_status_peek_buf(void){ return g_tx_buf; }
void app_status_mark_sent(void){ g_tx_len = 0; }

void handle_upload_map(const uint8_t *pay, uint8_t pal){
    if (pal < 1) return;
    const uint8_t N = pay[0];
    if (pal < (uint8_t)(1 + 2U * N)) return;
    cfg_count = 0;
    for(uint8_t i=0;i<N && cfg_count<MAX_CFG;++i){
        const uint8_t c = pay[1 + 2*i + 0];
        const uint8_t s = pay[1 + 2*i + 1];
        if (s == 0x01) cfg_conn[cfg_count++] = c;
    }
    g_status_ext = 0x00;
}

