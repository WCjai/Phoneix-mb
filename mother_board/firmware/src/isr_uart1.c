/*
 * isr_uart1.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "proto.h"
#include "sched.h"
#include "config.h"
#include "chip.h"

typedef enum { U1_WAIT_SOF=0, U1_GOT_SOF, U1_WAIT_LEN, U1_COLLECT, U1_WAIT_END } u1_fsm_t;
static volatile u1_fsm_t u1_state = U1_WAIT_SOF;
static volatile uint8_t  u1_len   = 0, u1_idx = 0;
static uint8_t           u1_pay[8];

void UART1_IRQHandler(void){
    while (Chip_UART_ReadLineStatus(LPC_UART1) & UART_LSR_RDR){
        const uint8_t b = Chip_UART_ReadByte(LPC_UART1);
        switch (u1_state){
        case U1_WAIT_SOF: if (b == SOF) u1_state = U1_GOT_SOF; break;
        case U1_GOT_SOF:
            if (b == SOF) { u1_state = U1_WAIT_LEN; }
            else {
                u1_len = b; u1_idx = 0;
                if (!u1_len || u1_len > sizeof(u1_pay)) { u1_state = U1_WAIT_SOF; break; }
                u1_state = U1_COLLECT;
            }
            break;
        case U1_WAIT_LEN:
            u1_len = b; u1_idx = 0;
            if (!u1_len || u1_len > sizeof(u1_pay)) { u1_state = U1_WAIT_SOF; break; }
            u1_state = U1_COLLECT; break;
        case U1_COLLECT:
            u1_pay[u1_idx++] = b;
            if (u1_idx == u1_len) u1_state = U1_WAIT_END;
            break;
        case U1_WAIT_END:
            if (b == END_BYTE && u1_len == 3 && u1_pay[0] == SC_STATUS){
                const uint8_t addr = u1_pay[1], st = u1_pay[2];
                if (addr >= 1 && addr <= 31 && st){
                    const uint32_t bit = CONN_BIT(addr);
                    round_alive_mask |= bit;
                    if (st == 0x03) round_triggered_mask |= bit;
                    else            round_triggered_mask &= ~bit;
                    // Streaming snapshot (optional)
                    extern volatile bool g_led_streaming_active;
                    if (g_led_streaming_active){
                        g_alive_mask |= bit;
                        if (st == 0x03) g_triggered_mask |= bit;
                        else            g_triggered_mask &= ~bit;
                    }
                }
            }
            u1_state = U1_WAIT_SOF; break;
        default: u1_state = U1_WAIT_SOF; break;
        }
    }
}

