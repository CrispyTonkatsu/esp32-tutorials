#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "rom/ets_sys.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// The interrupt pin for the GPIO
#define OUT_GPIO CONFIG_OUT_GPIO
#define INTR_GPIO CONFIG_INTR_GPIO

#define TIMEOUT_MS CONFIG_TIMEOUT_MS
#define TASK_DELAY_MS CONFIG_TASK_DELAY_MS

const float sound_speed_cm_ms = 0.0343;
static const char *tag = "MAIN";
TaskHandle_t send_signal_task_handle;

volatile uint64_t echo_start_time = 0;
volatile uint64_t echo_end_time = 0;

void isr_handler(void *arg) {
  if (gpio_get_level(INTR_GPIO) == 1) {
    echo_start_time = esp_timer_get_time();
    return;
  }

  echo_end_time = esp_timer_get_time();

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(send_signal_task_handle, &xHigherPriorityTaskWoken);

  // Requesting the context switch if it is needed
  portYIELD_FROM_ISR_ARG(xHigherPriorityTaskWoken);
}

void print_distance() {
  const uint64_t pulse_duration = echo_end_time - echo_start_time;

  if (pulse_duration == 0) {
    ESP_LOGI(tag, "Distance too short to measure");
    return;
  }

  const float distance = (float)pulse_duration * sound_speed_cm_ms * 0.5f;
  ESP_LOGI(tag, "Distance: %f", distance);
}

void send_signal_task(void *arg) {
  for (;;) {
    gpio_set_level(OUT_GPIO, 1);
    ets_delay_us(10);
    gpio_set_level(OUT_GPIO, 0);

    const uint32_t notified =
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(TIMEOUT_MS));

    if (notified != 0) {
      ESP_LOGI(tag, "Notification received, calculating distance");
      print_distance();

    } else {
      ESP_LOGI(tag, "Timed out, restarting detection cycle");
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

void app_main(void) {
  // Configuring the output GPIO (emitter)
  gpio_config_t out_config = {
      .pin_bit_mask = (1ull << OUT_GPIO),
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&out_config));

  // Configuring the interrupt GPIO (listener)
  gpio_config_t intr_config = {
      .pin_bit_mask = (1ull << INTR_GPIO),
      .mode = GPIO_MODE_INPUT,
      .intr_type = GPIO_INTR_ANYEDGE,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&intr_config));

  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LOWMED));
  ESP_ERROR_CHECK(gpio_isr_handler_add(INTR_GPIO, isr_handler, NULL));

  xTaskCreate(send_signal_task, "send_signal_task", 4096, NULL, 5,
              &send_signal_task_handle);

  ESP_LOGI(tag, "Started task, exiting main");
}
