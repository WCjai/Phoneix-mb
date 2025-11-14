/**
 * @file app_status.h
 * @brief App-status frame builder and shared configuration state.
 *
 * Responsibilities:
 * - Owns cfg_conn[] / cfg_count (connectors map uploaded by the App).
 * - Maintains button/status masks (g_status_ext, one-shot/force-01 masks).
 * - Builds GRP_RX_TO_APP SC_STATUS frames into an internal buffer.
 * - Exposes "prepare/peek/send" accessors used by the main loop.
 *
 * Usage:
 * - Call request_status_reply() whenever you want to send a heartbeat.
 * - In main loop, check app_status_peek_len() and send if >0, then
 *   app_status_mark_sent().
 *
 * Dependencies: Reads alive/triggered masks from sched.c.
 */

#ifndef INC_APP_STATUS_H_
#define INC_APP_STATUS_H_

#pragma once
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "proto.h"

// External status bits (buttons)
extern volatile uint8_t  g_status_ext;

// One-shot and force masks
extern volatile uint32_t g_status01_mask;
extern volatile uint32_t g_force01_while_triggered_mask;

// Connector map
extern uint8_t  cfg_conn[MAX_CFG];
extern uint8_t  cfg_count;

// Build status frame into internal buffer and mark ready
void request_status_reply(void);

// Accessors for main loop to send prepared status
size_t       app_status_peek_len(void);
const uint8_t* app_status_peek_buf(void);
void         app_status_mark_sent(void);

// Handlers that modify config/status
void handle_upload_map(const uint8_t *pay, uint8_t pal);

#endif /* INC_APP_STATUS_H_ */
