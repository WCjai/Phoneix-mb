# RX Controller — Comprehensive Technical Documentation

> Version: 1.0  
> Target MCU family: LPC (LPCOpen / `chip.h`, `board.h`)  
> Core buses: **UART0 (App)**, **UART1 (Slaves)**, **UART2 (BIN)**, **WS2812**  
> Scheduler: **RIT** (70 ms)

---

## 1) Repository & File Structure

```
firmware/
├─ inc/
│  ├─ config.h          # Global constants: timing, sizes, debounce, macros
│  ├─ proto.h           # Frame format, Group IDs, Service Codes (SC_*)
│  ├─ queues.h          # ISR-safe TX ring buffers for UART1 / UART2
│  ├─ ws_led.h          # WS2812 framebuffer API + deferred flush
│  ├─ app_status.h      # Status-frame builder + connector map/state
│  ├─ u1_jobs.h         # UART1 LED job table + RR scheduler hook
│  ├─ u2_jobs.h         # UART2 (BIN) streaming jobs + batch mask helpers
│  ├─ buttons.h         # Debounce bookkeeping + helpers
│  ├─ sched.h           # Global timing/state shared with RIT + helpers
│  ├─ isr_uart0.h       # UART0 ISR declaration (App→RX)
│  ├─ isr_uart1.h       # UART1 ISR declaration (Slave→RX)
│  ├─ isr_gpio.h        # GPIO ISR declaration (EINT3)
│  └─ isr_rit.h         # RIT ISR declaration (central scheduler)
└─ src/
   ├─ main.c            # HW init, NVIC, main loop drains queues & flushes WS
   ├─ queues.c          # Ring buffer implementations
   ├─ ws_led.c          # WS framebuffer + flush implementation
   ├─ app_status.c      # Build RX→App status frames; store cfg map & flags
   ├─ u1_jobs.c         # UART1 LED jobs & scheduler emission
   ├─ u2_jobs.c         # UART2 jobs & mask frames
   ├─ isr_uart0.c       # Frame parser & SC dispatch (App commands)
   ├─ isr_uart1.c       # Parse slaves’ SC_STATUS replies into masks
   ├─ isr_gpio.c        # Debounced button press → request status reply
   └─ isr_rit.c         # RIT: idle watchdog, stream-or-poll, BIN tick, WS
```

---

## 2) Program Architecture

### 2.1 High-level Components

- **UART0 (App ⇄ RX)**  
  Parses framed commands from the PC/host. After a valid command is handled, the RX **prepares** a status heartbeat for the App (sent by main loop).

- **UART1 (RX ⇄ Slaves, connectors 1..31)**  
  Two modes:
  - **Streaming**: emit LED-ON frames for active jobs (preempts polling)
  - **Polling**: round-robin poll of configured connectors when no jobs

- **UART2 (“BIN”)**  
  Continuous LED streaming channel plus **batch mask** frames for compact updates.

- **WS2812 Strip**  
  Mirrors BIN=1 visually. All WS writes are **deferred** to main loop.

- **Buttons (S1/S2)**  
  Debounced in GPIO ISR; bits latched into `g_status_ext`. On press, request a status reply to the App.

- **RIT (Scheduler, 70 ms)**  
  Decides per-tick actions: **stream-or-poll** on UART1, **one job** per tick on UART2, manage **idle watchdog**, and request WS flush.

### 2.2 Concurrency & Responsibilities

- **ISRs**  
  - Parse bytes (UART0, UART1)  
  - Update compact state (masks, jobs)  
  - **Only enqueue** frames into ISR-safe queues (no blocking I/O)  
  - Request WS flush (never write WS in ISR)

- **Main loop**  
  - Drain **UART1/2** queues → `Chip_UART_SendBlocking`  
  - Send **prepared status** to App (UART0)  
  - Perform **WS flush** (safe I/O timing)  
  - Sleep (`__WFI`)

### 2.3 Key Invariants

- **No WS writes in ISRs**  
- **ISRs push; main loop pops** (queues)  
- UART1 **polling only when no LED jobs** active  
- **Reset-on-new + de-dup** for LED jobs (target gets a single current job)  
- **Idle watchdog** (~2 s) forces OFF on both buses and clears WS

---

## 3) System Workflow (End-to-end)

### 3.1 Sequence – Handling a Command from App (UART0)

```text
App → UART0 ISR:
  SOF LEN [GRP_APP_TO_RX, RX_ID, SC, payload…] END
              │
              ├─ isr_uart0.c dispatches SC handler:
              │    - may update connector map / jobs / masks / WS buffer
              │    - calls request_status_reply()
              │
              └─ (returns)

Main loop:
  if status prepared → send to App (UART0)
  drain UART1 & UART2 TX queues → send frames
  ws_flush_if_pending()
  WFI
```

### 3.2 Sequence – RIT Tick (70 ms)

```text
RIT:
  if off_broadcast_pending → enqueue UART1 OFF
  if off_broadcast2_pending → enqueue UART2 OFF

  if (app_idle ~2s):
     enqueue both OFF; WS clear; request WS flush; return

  if (UART1 jobs exist):
     u1_scheduler_emit_one()   // enqueue one LED-ON
     request WS flush (if needed)
  else:
     if (start of poll round) commit & clear last round into g_* masks
     enqueue poll for next connector
     request WS flush (if needed)

  u2_scheduler_emit_one()      // enqueue one BIN LED-ON (if due)
```

### 3.3 UART1 Round-Robin vs. Streaming

- **Streaming active** ⇢ **pause** polling; emit at most **one job** per tick  
- **No jobs** ⇢ **resume** round-robin **polling** (strict 1..N)

---

## 4) Protocol Specification

### 4.1 Frame Format

```
SOF (0x27) | LEN | BODY… | END (0x16)

- LEN counts bytes from the first byte of BODY up to the last payload byte.
- BODY begins with: GROUP, ID (if applicable), SC (service code), ...
```

### 4.2 Group IDs

| Direction       | GROUP |
|-----------------|-------|
| App → RX        | 0x85  |
| RX → App        | 0x00  |
| RX → Slave      | 0x97  |
| Slave → RX      | 0x27  |

### 4.3 Service Codes (App → RX)

| SC                | Hex  | Payload (summary)                                                                                         |
|-------------------|------|------------------------------------------------------------------------------------------------------------|
| `SC_UPLOAD_MAP`   | 0x04 | `N, (c1,s1), (c2,s2)…` where `s=0x01` means present/active. Builds `cfg_conn[]`.                          |
| `SC_LED_CTRL`     | 0x02 | **Modeed** control (next byte is `mode`). See §4.4.                                                        |
| `SC_LED_RESET`    | 0x3A | Stop all LED jobs, OFF broadcast on both buses, WS clear.                                                  |
| `SC_BTNFLAG_RESET`| 0x09 | Clear button bits; force `Si=0x01` while currently triggered; OFF both; stop jobs.                         |
| `SC_NEW_STATUS01` | 0x03 | One-shot `Si=0x01` (all or connector) **and** perform LED reset semantics.                                 |
| `SC_STATUS`       | 0x0A | Special helper: turn **LED#1 ON** for a list of connectors (UART1 only).                                   |
| `SC_BIN_MASK`     | 0x0B | BIN LED packed mask: `max_led, l[1..max_led]`. Mirrors WS exactly and sends one compact UART2 frame.       |

#### 4.4 `SC_LED_CTRL` modes

`SC_LED_CTRL (0x02)` payload starts with `mode`:

- **mode = 0x00** — legacy-as-current BIN/WS  
  Payload: `[00, 00, 00, 02, bin, led]`  
  Action: start/refresh BIN per-LED job (de-dup by `bin`), and **mirror WS** if `bin == 1`.

- **mode = 0x01** — UART1 connector LED  
  Payload: `[01, con, led]`  
  Action: start/refresh UART1 job (de-dup per `con`), **preempt RR** to send next tick.

- **mode = 0x02** — Combined BIN + UART1  
  Payload: `[02, bin, bin_led, flags, con, con_led]`  
  Action: update BIN (with WS mirror if `bin==1`) and add UART1 job atomically.

> **Reset-on-new + de-dup**: for a new `(target, led)`:
> - Remove previous jobs for that target except the new LED
> - If job already exists for the same `(target, led)`, only refresh its `next_allowed_tick`
> - Move RR cursor to this job so it fires **immediately on next tick**

### 4.5 Slave → RX status (UART1)

- Body: `[SC_STATUS (0x0A), addr (1..31), st]` then `END`
- Meanings:
  - `st == 0x01` → alive (not triggered)
  - `st == 0x03` → alive + triggered
- RX ISR updates:
  - `round_alive_mask |= CONN_BIT(addr)`
  - `round_triggered_mask` set/cleared based on `st`
  - If streaming active: mirror to `g_alive_mask/g_triggered_mask` (snapshot)

### 4.6 RX → App status frame

**Built after each handled App command** (and immediately on button press):

```
SOF | LEN | 0x00 (GRP_RX_TO_APP) | RX_ID (0x01) | SC_STATUS (0x0A)
    | g_status_ext | N | [Si[0]..Si[N-1]] | 0x00 | 0x00 | END
```

Where `Si` per connector encodes:

| Value | Meaning                             |
|-------|-------------------------------------|
| 0x00  | not alive                           |
| 0x05  | alive                               |
| 0x07  | alive + triggered                   |
| 0x01  | forced-01 (one-shot or while-triggered override) |

> **One-shot mask** clears **after** building the status.  
> **Force-while-triggered** auto-clears when trigger disappears.

---

## 5) Timing, Masks, & State

### 5.1 RIT & Watchdog

- **`g_tick`** increments every 70 ms (RIT period).
- **Idle watchdog**: `app_idle` if `(g_tick - g_app_last_activity_tick) ≥ APP_IDLE_TICKS` (~2 s).
- On idle: enqueue OFF on UART1/2, clear WS, request flush, **return early** (pauses work).

### 5.2 Alive/Triggered Masks

- **round_alive_mask / round_triggered_mask**: accumulated during one full poll cycle.
- **g_alive_mask / g_triggered_mask**: “committed view” used for status frames.
- At the **start** of each poll cycle, `sched_commit_and_clear_poll_round()` moves `round_* → g_*` and clears `round_*`.

### 5.3 Job Scheduling

- **UART1**: `u1_scheduler_emit_one()` emits at most **one** LED-ON per RIT tick (if any job due).  
  RR order is preserved; **new jobs** are positioned to fire **ASAP**.

- **UART2 (BIN)**: `u2_scheduler_emit_one()` emits at most **one** per tick.

---

## 6) Error Handling & Robustness

- **Queue full**: ISR push returns false and increments `u1_drops` / `u2_drops`.  
- **Malformed frames**: UART0/1 FSMs reset to `WAIT_SOF`.  
- **Oversized U2 frame**: rejected (no truncation) to avoid protocol ambiguity.  
- **Out-of-range IDs**: ignored silently (`con ∉ [1..31]`, `led` out of bounds, etc.).

---

## 7) Extensibility Guidelines

- Add new SCs in `isr_uart0.c`, and **always** call `request_status_reply()` at the end.
- Keep **WS writes** confined to `ws_led.c` and performed in **main** via `ws_flush_if_pending()`.
- When adding new per-connector state:
  - Extend `build_status_frame()` (`app_status.c`)
  - Document encoding in this markdown
  - Consider mask lifecycles (round vs. committed masks)

---

## 8) Examples

### 8.1 Turn ON connector 5 LED 2 (UART1)

```
27 | 03 | 85 01 02 01 05 02 | 16
         ^   ^  ^  ^  ^  ^
         |   |  |  |  |  └ LED=2
         |   |  |  |  └ CON=5
         |   |  |  └ mode=0x01 (UART1)
         |   |  └ SC_LED_CTRL (0x02)
         |   └ RX_ID (0x01)
         └ GRP_APP_TO_RX (0x85)
```

### 8.2 Turn ON BIN#1 LED 9 + mirror WS

```
27 | 06 | 85 01 02 00 00 00 02 01 09 | 16
```

### 8.3 Combined BIN+UART1 (BIN#1 LED 7, CON=3 LED 2)

```
27 | 06 | 85 01 02 02 01 07 00 03 02 | 16
                     ^   ^  ^  ^  ^
                     |   |  |  |  └ CON_LED=2
                     |   |  |  └ CON=3
                     |   |  └ flags (reserved)
                     |   └ BIN_LED=7
                     └ BIN=1
```

### 8.4 BIN packed mask for first 8 LEDs (WS mirrors exactly)

```
27 | 0B | 85 01 0B 08 01 02 03 04 05 06 07 08 | 16
```

### 8.5 LED reset (global OFF, stop jobs, WS clear)

```
27 | 01 | 85 01 3A | 16
```

---

## 9) Build & Porting Notes

- Ensure the **GPIO ISR** symbol matches the vector table (e.g., LPC17xx uses `EINT3_IRQHandler`).  
  If you keep a generic name, add `#define GPIO_IRQ_HANDLER EINT3_IRQHandler` before compilation.
- UART speeds: UART0=19200 8N1; UART1/2=9600 8N1.  
- RIT period: `RIT_TICK_MS` (default 70 ms).

---

## 10) Quick Troubleshooting

- **Button press not seen by App**  
  - Check ISR vector name.  
  - We request a status reply on press; main loop must be running to send it.

- **RR “stuck” on (con=1, led=1)**  
  - Ensure **reset-on-new** helpers are used (`*_remove_by_*_except`, then `*_find/alloc`) and that RR index is moved to the new job to preempt.

- **Idle watchdog overrides activity**  
  - `APP_IDLE_MS` too low → increase.  
  - If you want button press to “wake” the watchdog immediately, set `g_app_last_activity_tick = g_tick` in the GPIO ISR.

---

## 11) Glossary

- **RR**: Round-robin.  
- **BIN**: Second LED bus on UART2.  
- **Si**: Status byte per connector in RX→App status (0x00, 0x05, 0x07, 0x01).  
- **One-shot 01**: Forces `Si=0x01` **once** (clears after building status).  
- **Force-while-triggered**: Keeps `Si=0x01` until trigger disappears.

---

## 12) Change Log (abridged)

- Added **modeed** `SC_LED_CTRL` (0x00 legacy BIN/WS, 0x01 UART1, 0x02 dual).  
- Introduced **reset-on-new + de-dup** for both UART1 and UART2 jobs.  
- Added **BIN packed mask** (`SC_BIN_MASK` 0x0B) and WS mirror.  
- Button press now **requests status** immediately.  
- **Idle watchdog** centralization in RIT.

---

### Appendix A — ASCII Data Path Diagram

```
        +-----------+           +-----------------+
App --->|  UART0 RX |--SC-----> |  SC Handlers    |----+--> u1_jobs (UART1)
        +-----------+           +-----------------+    |--> u2_jobs (UART2)
                |                      |               |--> ws_led (buffer)
                |                      +--> request_status_reply()
                v
        +-----------------+
        | status builder  |
        | (app_status.c)  |
        +-----------------+
                |
                v
          [main loop]
    send status to App
    drain U1/U2 queues
    WS flush if pending

[RIT 70ms]
  - idle watchdog
  - if u1 jobs → enqueue one
    else → poll next connector (UART1)
  - enqueue one BIN job
  - request WS flush (if needed)
```
