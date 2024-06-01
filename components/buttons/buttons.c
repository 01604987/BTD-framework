#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

enum Switch finger = NONE;
static const char *TAG = "Button";

volatile button_state_t button_state_index = BUTTON_RELEASED;
volatile button_state_t button_state_middle = BUTTON_RELEASED;
volatile uint32_t button_press_start_time_index = 0;
volatile uint32_t button_press_start_time_middle = 0;
volatile uint32_t last_tap_time_index = 0;
volatile uint32_t last_tap_time_middle = 0;


static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t current_time = xTaskGetTickCountFromISR();

    if (gpio_get_level(gpio_num) == 0) {  // Button pressed
        if (gpio_num == BUTTON_PIN_INDEX) {
            if (current_time - last_tap_time_index <= DOUBLE_TAP_TIME_MS && button_state_index == BUTTON_RELEASED) {
                button_state_index = BUTTON_DOUBLE_TAP;
                finger = DOUBLE_TAP_INDEX;
            } else {
                button_state_index = BUTTON_PRESSED;
                finger = INDEX;
            }
            button_press_start_time_index = current_time;
            last_tap_time_index = current_time;
        } else if (gpio_num == BUTTON_PIN_MIDDLE) {
            if (current_time - last_tap_time_middle <= DOUBLE_TAP_TIME_MS && button_state_middle == BUTTON_RELEASED) {
                button_state_middle = BUTTON_DOUBLE_TAP;
                finger = DOUBLE_TAP_MIDDLE;
            } else {
                button_state_middle = BUTTON_PRESSED;
                finger = MIDDLE;
            }
            button_press_start_time_middle = current_time;
            last_tap_time_middle = current_time;
        }
    } else {  // Button released
        if (gpio_num == BUTTON_PIN_INDEX) {
            button_state_index = BUTTON_RELEASED;
            finger = NONE;  
        } else if (gpio_num == BUTTON_PIN_MIDDLE) {
            button_state_middle = BUTTON_RELEASED;
            finger = NONE;
        }
    }
}


void button_task(void* arg) {
    while (1) {
        if (button_state_index == BUTTON_PRESSED || button_state_middle == BUTTON_DOUBLE_TAP) {
            uint32_t current_time = xTaskGetTickCount();
            if (current_time - button_press_start_time_index > pdMS_TO_TICKS(HOLD_TIME_MS)) {
                    button_state_index = BUTTON_HOLD;
            }
        } else if (button_state_index == BUTTON_RELEASED) {
            button_press_start_time_index = 0;
        }

        if (button_state_middle == BUTTON_PRESSED || button_state_middle == BUTTON_DOUBLE_TAP) {
            uint32_t current_time = xTaskGetTickCount();
            if (current_time - button_press_start_time_middle > pdMS_TO_TICKS(HOLD_TIME_MS)) {
                    button_state_middle = BUTTON_HOLD;
            }
        } else if (button_state_middle == BUTTON_RELEASED) {
            button_press_start_time_middle = 0;
        }    

        vTaskDelay(pdMS_TO_TICKS(100)); // Polling interval
    }
}

void init_button() {
    gpio_config_t io_conf_index;
    io_conf_index.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_index.mode = GPIO_MODE_INPUT;
    io_conf_index.pin_bit_mask = (1ULL << BUTTON_PIN_INDEX);
    io_conf_index.pull_down_en = 0;
    io_conf_index.pull_up_en = 1;
    gpio_config(&io_conf_index);

    gpio_config_t io_conf_middle;
    io_conf_middle.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_middle.mode = GPIO_MODE_INPUT;
    io_conf_middle.pin_bit_mask = (1ULL << BUTTON_PIN_MIDDLE);
    io_conf_middle.pull_down_en = 0;
    io_conf_middle.pull_up_en = 1;
    gpio_config(&io_conf_middle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN_INDEX, button_isr_handler, (void*) BUTTON_PIN_INDEX);
    gpio_isr_handler_add(BUTTON_PIN_MIDDLE, button_isr_handler, (void*) BUTTON_PIN_MIDDLE);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
