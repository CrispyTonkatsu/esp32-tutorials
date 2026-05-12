#include <stdio.h>

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
  gpio_reset_pin(CONFIG_LED_GPIO);
  gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);

  gpio_reset_pin(CONFIG_INPUT_GPIO);
  gpio_set_direction(CONFIG_INPUT_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(CONFIG_INPUT_GPIO, GPIO_PULLUP_ONLY);

  int state = 0;
  gpio_set_level(CONFIG_LED_GPIO, state);

  for (;;) {
    if (gpio_get_level(CONFIG_INPUT_GPIO) == 0) {
      state = !state;

      gpio_set_level(CONFIG_LED_GPIO, state);

      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
