#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

enum Switch finger = MIDDLE;
static const char *TAG = "Button";

volatile button_state_t button_state = BUTTON_RELEASED;
volatile uint32_t button_press_start_time = 0;


static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t current_time = xTaskGetTickCountFromISR();

    if (gpio_get_level(gpio_num) == 0) {
        // Button pressed
        if (button_state == BUTTON_RELEASED) {
            button_state = BUTTON_PRESSED;
            button_press_start_time = current_time;
        }
    } else {
        // Button released
        if (button_state == BUTTON_PRESSED || button_state == BUTTON_HELD) {
            button_state = BUTTON_RELEASED;
        }
    }
}

void button_task(void* arg) {
    while (1) {
        if (button_state == BUTTON_PRESSED) {
            ESP_LOGI(TAG, "Button pressed");
            button_state = BUTTON_HELD;
        } else if (button_state == BUTTON_HELD) {
            uint32_t current_time = xTaskGetTickCount();
            if (current_time - button_press_start_time > pdMS_TO_TICKS(1000)) { // Button held for more than 1 second
                ESP_LOGI(TAG, "Button held");
            }
        } else if (button_state == BUTTON_RELEASED) {
            ESP_LOGI(TAG, "Button released");
            button_press_start_time = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Polling interval
    }
}

void init_button() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void*) BUTTON_PIN);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
