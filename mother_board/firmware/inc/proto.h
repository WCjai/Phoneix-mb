/**
 * @file proto.h
 * @brief Protocol constants shared across modules.
 *
 * - Framing bytes: SOF (0x27), END_BYTE (0x16).
 * - Group IDs and Service Codes (SC_*).
 * - RX_ID and helpers like CONN_BIT().
 *
 * Pure constants/macros; no state. Safe for ISRs.
 */

#ifndef INC_PROTO_H_
#define INC_PROTO_H_

#pragma once
#include <stdint.h>

// Framing
enum { SOF = 0x27, END_BYTE = 0x16 };

// Groups and Service Codes
enum { GRP_APP_TO_RX=0x85, GRP_RX_TO_APP=0x00, GRP_RX_TO_SLV=0x97, GRP_SLV_TO_RX=0x27 };

enum {
  SC_POLL=0x00,
  SC_LED_CTRL=0x02,
  SC_NEW_STATUS01=0x03,   // one-shot Si=01 + LED reset
  SC_UPLOAD_MAP=0x04,
  SC_LED_RESET=0x3A,
  SC_RELAY_SET=0x06,
  SC_STATUS=0x0A,
  SC_BTNFLAG_RESET=0x09,
  SC_SLAVE=0x85,
  SC_BIN_MASK=0x0B
};

#define RX_ID 0x01
#define CONN_BIT(c) (1u << ((c) - 1))


#endif /* INC_PROTO_H_ */
