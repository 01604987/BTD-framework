#ifndef BUTTONS_H
#define BUTTONS_H

#include "driver/gpio.h"

#define BUTTON_PIN 37

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


void init_button();

#endif // BUTTONS_H



// TODO define functions for initialization (registering buttons, etc)
// TODO implement callback functions that sets finger var to enum values

