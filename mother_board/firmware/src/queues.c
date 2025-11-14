/*
 * queues.c
 *
 *  Created on: 14-Nov-2025
 *      Author: mad23
 */

#include "queues.h"
#include <string.h>

// UART1 queue
static volatile uint8_t u1_head=0,u1_tail=0;
volatile uint32_t u1_drops=0;
static U1Frame u1_q[U1_TXQ_CAP];

bool u1q_push_isr(const uint8_t *d, uint8_t n){
    const uint8_t next = (uint8_t)((u1_head + 1) % U1_TXQ_CAP);
    if (next == u1_tail) { u1_drops++; return false; }
    if (n > sizeof(u1_q[0].data)) n = sizeof(u1_q[0].data);
    memcpy(u1_q[u1_head].data, d, n);
    u1_q[u1_head].len = n; u1_head = next; return true;
}
bool u1q_pop_main(U1Frame *out){
    if (u1_tail == u1_head) return false;
    *out = u1_q[u1_tail];
    u1_tail = (uint8_t)((u1_tail + 1) % U1_TXQ_CAP);
    return true;
}

// UART2 queue
static volatile uint8_t u2_head=0,u2_tail=0;
volatile uint32_t u2_drops=0;
static U2Frame u2_q[U2_TXQ_CAP];

bool u2q_push_isr(const uint8_t *d, uint8_t n){
    const uint8_t next = (uint8_t)((u2_head + 1) % U2_TXQ_CAP);
    if (next == u2_tail) { u2_drops++; return false; }
    if (n > sizeof(u2_q[0].data)) { u2_drops++; return false; }
    memcpy(u2_q[u2_head].data, d, n);
    u2_q[u2_head].len = n; u2_head = next; return true;
}
bool u2q_pop_main(U2Frame *out){
    if (u2_tail == u2_head) return false;
    *out = u2_q[u2_tail];
    u2_tail = (uint8_t)((u2_tail + 1) % U2_TXQ_CAP);
    return true;
}

