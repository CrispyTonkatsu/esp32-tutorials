#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "rom/ets_sys.h"

#include "driver/gpio.h"
#include "soc/gpio_num.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SENSOR_PIN CONFIG_SENSOR_GPIO
#define POLL_RATE_MS CONFIG_POLL_RATE_MS

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

  if (wait_for_state(SENSOR_PIN, 0, 100).timed_out) {
    ESP_LOGE(tag, "Timeout on wait for device");
    return false;
  }

  if (wait_for_state(SENSOR_PIN, 1, 100).timed_out) {
    ESP_LOGE(tag, "Timeout on device low");
    return false;
  }

  if (wait_for_state(SENSOR_PIN, 0, 100).timed_out) {
    ESP_LOGE(tag, "Timeout on device high");
    return false;
  }

  return true;
}

uint8_t read_byte() {
  uint8_t output = 0;

  for (size_t i = 0; i < 8; i++) {
    if (wait_for_state(SENSOR_PIN, 1, 60).timed_out) {
      ESP_LOGE(tag, "Bit header timed out");
      return 0;
    }

    const WaitData signal_data = wait_for_state(SENSOR_PIN, 0, 80);
    if (signal_data.timed_out) {
      ESP_LOGE(tag, "Bit wait for low timed out");
      return 0;
    }

    output <<= 1;

    if (signal_data.duration > 40) {
      output |= 1;
    }
  }

  return output;
}

void app_main() {
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
      vTaskDelay(pdMS_TO_TICKS(POLL_RATE_MS));
      continue;
    }

    const uint8_t humidity_int = read_byte();
    const uint8_t humidity_dec = read_byte();
    const uint8_t temperature_int = read_byte();
    const uint8_t temperature_dec = read_byte();
    const uint8_t checksum = read_byte();

    if (((humidity_int + humidity_dec + temperature_int + temperature_dec) &
         0xFF) == checksum) {

      const float humidity_value =
          (float)humidity_int + (float)humidity_dec / 10.f;

      const float temperature_value =
          (float)temperature_int + (float)temperature_dec / 10.f;

      ESP_LOGI(tag, "Humidity: %.1f, Temperature: %.1f C", humidity_value,
               temperature_value);

    } else {
      ESP_LOGE(tag, "Checksum mismatch, trying again");
    }

    vTaskDelay(pdMS_TO_TICKS(POLL_RATE_MS));
  }
}
