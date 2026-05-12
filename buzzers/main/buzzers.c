#include <stdint.h>

#include "driver/gpio.h"

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
  gpio_reset_pin(CONFIG_BUZZ_GPIO);
  gpio_set_direction(CONFIG_BUZZ_GPIO, GPIO_MODE_OUTPUT);

  uint32_t state = 1;

  for (;;) {
    gpio_set_level(CONFIG_BUZZ_GPIO, state);
    state = !state;

    vTaskDelay(CONFIG_BUZZ_DURATION / portTICK_PERIOD_MS);
  }
}
