#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"

#define BUTTON_PIN_INDEX GPIO_NUM_37
#define BUTTON_PIN_MIDDLE GPIO_NUM_39

#define DEBOUNCE_TIME_MS 20
#define DOUBLE_TAP_TIME_MS 50
#define HOLD_TIME_MS 500

enum Switch{
    NONE,
    INDEX,
    MIDDLE,
    DOUBLE_TAP_INDEX,
    DOUBLE_TAP_MIDDLE,
    DEBUG,
    DEV0,
    DEV1
};


typedef enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED,
    BUTTON_HOLD,
    BUTTON_DOUBLE_TAP,
    BUTTON_TAP_HOLD
} button_state_t;

extern volatile button_state_t button_state_index;
extern volatile button_state_t button_state_middle;

extern enum Switch finger;

void init_button();

#endif // BUTTONS_H

