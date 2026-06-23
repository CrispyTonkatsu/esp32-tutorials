#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/projdefs.h"
#include "rom/ets_sys.h"

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SENSOR_PIN GPIO_NUM_27 // CONFIG_SENSOR_PIN
#define POLL_RATE_MS 1000      // CONFIG_POLL_RATE_MS

const static char *tag = "DHT11";

typedef struct {
  int64_t duration;
  bool timed_out;
} WaitData;

inline WaitData WaitData_make(int64_t duration, bool timed_out) {
  const WaitData output = {
      .duration = duration,
      .timed_out = timed_out,
  };
  return output;
}

WaitData wait_for_state(gpio_num_t pin, int state, int64_t timeout) {
  const int64_t start_time = esp_timer_get_time();

  while (gpio_get_level(SENSOR_PIN) != state) {
    const int64_t time = esp_timer_get_time();

    if (time - start_time > timeout) {
      return WaitData_make(time - start_time, true);
    }
  }

  return WaitData_make(esp_timer_get_time() - start_time, false);
}

/**
 * Starting the data stream from the sensor
 * @return True if the function succeeded, false if it timed out
 */
bool start_sensor() {
  ESP_ERROR_CHECK(gpio_set_level(SENSOR_PIN, 0));
  ets_delay_us(18000);

  ESP_ERROR_CHECK(gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT));

  if (wait_for_state(SENSOR_PIN, 0, 40).timed_out) {
    ESP_LOGE(tag, "Timeout on wait for device");
    return false;
  }

  if (wait_for_state(SENSOR_PIN, 1, 80).timed_out) {
    ESP_LOGE(tag, "Timeout on device low");
    return false;
  }

  if (wait_for_state(SENSOR_PIN, 0, 80).timed_out) {
    ESP_LOGE(tag, "Timeout on device high");
    return false;
  }

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

  for (;;) {
    ESP_ERROR_CHECK(gpio_set_direction(SENSOR_PIN, GPIO_MODE_OUTPUT));

    if (!start_sensor()) {
      return;
    }

    // TODO: Reading the data

    vTaskDelay(pdMS_TO_TICKS(POLL_RATE_MS));
  }
}
