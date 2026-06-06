#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "rom/ets_sys.h"

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include <stddef.h>

#define SENSOR_PIN GPIO_NUM_27 // CONFIG_SENSOR_PIN
#define POLL_RATE_MS 100       // CONFIG_POLL_RATE_MS
#define TIMEOUT_MS 0.4         // CONFIG_TIMEOUT_MS

const static char *tag = "DHT11";

/**
 * Processing the ack signal
 * @return True if the function succeeded, false if it timed out
 */
bool process_ack_signal() {
  ESP_ERROR_CHECK(gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT));

  const int64_t start_time = esp_timer_get_time();
  while (gpio_get_level(SENSOR_PIN) == 1) {
    const int64_t time = esp_timer_get_time();

    if (time - start_time > 400) {
      return false;
    }
  }

  // TODO: Left off here: Implementing the check for the 80 microsecond low

  return true;
}

void app_main(void) {
  const gpio_config_t pin_config = {
      .pin_bit_mask = (1ull << SENSOR_PIN),
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&pin_config));

  ESP_ERROR_CHECK(gpio_set_level(SENSOR_PIN, 1));

  for (int iter_count = 0;; iter_count++) {
    ESP_ERROR_CHECK(gpio_set_direction(SENSOR_PIN, GPIO_MODE_OUTPUT));

    // Start signal
    ESP_ERROR_CHECK(gpio_set_level(SENSOR_PIN, 0));
    ets_delay_us(18000);

    // Waiting for the ack signal
    if (!process_ack_signal()) {
      ESP_LOGE(tag, "Timeout on ack signal, iterations: %i", iter_count);
      return;
    }

    for (;;) {
    }
  }
}
