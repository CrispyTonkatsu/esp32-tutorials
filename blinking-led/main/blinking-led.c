#include <stdint.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// This is something you can change in Blinking Configuration in idf.py menuconfig
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BLINK_DURATION CONFIG_BLINK_DURATION

void app_main(void) {
  gpio_reset_pin(BLINK_GPIO);
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

  uint32_t state = 1;

  for (;;) {
    gpio_set_level(BLINK_GPIO, state);

    state = !state;

    vTaskDelay(BLINK_DURATION / portTICK_PERIOD_MS);
  }
}
