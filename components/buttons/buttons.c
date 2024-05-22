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


static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t current_time = xTaskGetTickCountFromISR();

    if (gpio_get_level(gpio_num) == 0) {
        // Button pressed
        if (gpio_num == BUTTON_PIN_INDEX) {
            if (button_state_index == BUTTON_RELEASED) {
                button_state_index = BUTTON_PRESSED;
                button_press_start_time_index = current_time;
                finger = INDEX;
            }
        } else if (gpio_num == BUTTON_PIN_MIDDLE) {
            if (button_state_middle == BUTTON_RELEASED) {
                button_state_middle = BUTTON_PRESSED;
                button_press_start_time_middle = current_time;
                finger = MIDDLE;
            }
        }
    } else {
        // Button released
        if (gpio_num == BUTTON_PIN_INDEX) {
            if (button_state_index == BUTTON_PRESSED || button_state_index == BUTTON_HELD) {
                button_state_index = BUTTON_RELEASED;
                finger = NONE;
            }
        } else if (gpio_num == BUTTON_PIN_MIDDLE) {
            if (button_state_middle == BUTTON_PRESSED || button_state_middle == BUTTON_HELD) {
                button_state_middle = BUTTON_RELEASED;
                finger = NONE;
            }
        }
    }
}

void button_task(void* arg) {
    while (1) {
        if (button_state_index == BUTTON_PRESSED) {
            ESP_LOGI(TAG, "Button Index pressed");
            button_state_index = BUTTON_HELD;
        } else if (button_state_index == BUTTON_HELD) {
            uint32_t current_time = xTaskGetTickCount();
            if (current_time - button_press_start_time_index > pdMS_TO_TICKS(500)) { // Button held for more than 500 milliseconds
                ESP_LOGI(TAG, "Button Index held");
            }
        } else if (button_state_index == BUTTON_RELEASED) {
            ESP_LOGI(TAG, "Button Index released");
            button_press_start_time_index = 0;
        }

        if (button_state_middle == BUTTON_PRESSED) {
            ESP_LOGI(TAG, "Button Middle pressed");
            button_state_middle = BUTTON_HELD;
        } else if (button_state_middle == BUTTON_HELD) {
            uint32_t current_time = xTaskGetTickCount();
            if (current_time - button_press_start_time_middle > pdMS_TO_TICKS(500)) { // Button held for more than 500 milliseconds
                ESP_LOGI(TAG, "Button Middle held");
            }
        } else if (button_state_middle == BUTTON_RELEASED) {
            ESP_LOGI(TAG, "Button Middle released");
            button_press_start_time_middle = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Polling interval
    }
}

void init_button() {
    gpio_config_t io_conf_37;
    io_conf_37.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_37.mode = GPIO_MODE_INPUT;
    io_conf_37.pin_bit_mask = (1ULL << BUTTON_PIN_INDEX);
    io_conf_37.pull_down_en = 0;
    io_conf_37.pull_up_en = 1;
    gpio_config(&io_conf_37);

    gpio_config_t io_conf_39;
    io_conf_39.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_39.mode = GPIO_MODE_INPUT;
    io_conf_39.pin_bit_mask = (1ULL << BUTTON_PIN_MIDDLE);
    io_conf_39.pull_down_en = 0;
    io_conf_39.pull_up_en = 1;
    gpio_config(&io_conf_39);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN_INDEX, button_isr_handler, (void*) BUTTON_PIN_INDEX);
    gpio_isr_handler_add(BUTTON_PIN_MIDDLE, button_isr_handler, (void*) BUTTON_PIN_MIDDLE);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
