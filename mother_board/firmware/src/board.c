#include "board.h"

#define RELAY1_GPIO_PORT_NUM  4
#define RELAY1_GPIO_BIT_NUM   28
#define RELAY2_GPIO_PORT_NUM  0
#define RELAY2_GPIO_BIT_NUM   4
#define RELAY3_GPIO_PORT_NUM  0
#define RELAY3_GPIO_BIT_NUM   5
#define RELAY4_GPIO_PORT_NUM  0
#define RELAY4_GPIO_BIT_NUM   6
#define RELAY5_GPIO_PORT_NUM  0
#define RELAY5_GPIO_BIT_NUM   7
#define RELAY6_GPIO_PORT_NUM  0
#define RELAY6_GPIO_BIT_NUM   8

#define GPIO_BUTTON_PORT 2
#define GPIO_BUTTON_S2_PIN  3 // P2.3
#define GPIO_BUTTON_S1_PIN  4 // P2.4

#define GPIO_WS2812B_PORT 3
#define GPIO_WS2812B_1_PIN  25 // P3.25
#define GPIO_WS2812B_2_PIN  26 // P3.26

typedef struct { uint8_t port, pin; } relay_pin_t;

static const relay_pin_t kRelayPins[6] = {
   { RELAY1_GPIO_PORT_NUM, RELAY1_GPIO_BIT_NUM },
   { RELAY2_GPIO_PORT_NUM, RELAY2_GPIO_BIT_NUM },
   { RELAY3_GPIO_PORT_NUM, RELAY3_GPIO_BIT_NUM },
   { RELAY4_GPIO_PORT_NUM, RELAY4_GPIO_BIT_NUM },
   { RELAY5_GPIO_PORT_NUM, RELAY5_GPIO_BIT_NUM },
   { RELAY6_GPIO_PORT_NUM, RELAY6_GPIO_BIT_NUM },
};

//const uint32_t OscRateIn    = 12000000;
//const uint32_t RTCOscRateIn = 32768;

const uint32_t OscRateIn   = 12000000;   // main oscillator input
const uint32_t RTCOscRateIn = 0;         // or 32768 if you have an RTC crystal

void Board_Relays_Init(void) {
    for (unsigned i = 0; i < 6; ++i) {
        const relay_pin_t *rp = &kRelayPins[i];
        Chip_GPIO_WriteDirBit(LPC_GPIO, rp->port, rp->pin, true);
        Chip_GPIO_WritePortBit(LPC_GPIO, rp->port, rp->pin, false);
    }
}

void Board_ws2812_Init(void) {
    Chip_IOCON_Init(LPC_IOCON);
    Chip_GPIO_Init(LPC_GPIO);
    Chip_GPIO_Init(LPC_GPIO3);
    LPC_SYSCTL->PCONP |= (1 << 15);
}

void Board_buttons_sw_Init(void) {
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIO_BUTTON_PORT, GPIO_BUTTON_S1_PIN);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIO_BUTTON_PORT, GPIO_BUTTON_S2_PIN);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_WS2812B_PORT, GPIO_WS2812B_1_PIN);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIO_WS2812B_PORT, GPIO_WS2812B_2_PIN);

    Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIO_BUTTON_PORT,
        (1u << GPIO_BUTTON_S1_PIN) | (1u << GPIO_BUTTON_S2_PIN));
    Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIO_BUTTON_PORT,
        (1u << GPIO_BUTTON_S1_PIN) | (1u << GPIO_BUTTON_S2_PIN));
    Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIO_BUTTON_PORT,
        (1u << GPIO_BUTTON_S1_PIN) | (1u << GPIO_BUTTON_S2_PIN));
}

void Board_Relay_Set(uint8_t relay, bool on) {
    if (relay >= 1 && relay <= 6) {
        const relay_pin_t *rp = &kRelayPins[relay - 1];
        Chip_GPIO_WritePortBit(LPC_GPIO, rp->port, rp->pin, on);
    }
}


void Board_Init(void) {
    Chip_GPIO_Init(LPC_GPIO);
    Chip_IOCON_Init(LPC_IOCON);
    Board_Relays_Init();
    Board_buttons_sw_Init();

}
