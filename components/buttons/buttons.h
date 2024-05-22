#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"

#define BUTTON_PIN_INDEX GPIO_NUM_37
#define BUTTON_PIN_MIDDLE GPIO_NUM_39

enum Switch{
    NONE,
    INDEX,
    MIDDLE,
    DEBUG,
    DEV0,
    DEV1
};


typedef enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED,
    BUTTON_HELD
} button_state_t;

extern volatile button_state_t button_state_index;
extern volatile button_state_t button_state_middle;

extern enum Switch finger;

void init_button();

#endif // BUTTONS_H

