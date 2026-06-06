#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "rom/ets_sys.h"

#include "driver/gpio.h"

#include "soc/gpio_num.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>
#include <stdint.h>

static const char *tag = "NUMPAD";
static const char characters[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

static const gpio_num_t row_pins[] = {
    CONFIG_GPIO_ROW_0,
    CONFIG_GPIO_ROW_1,
    CONFIG_GPIO_ROW_2,
    CONFIG_GPIO_ROW_3,
};
static const gpio_num_t column_pins[] = {
    CONFIG_GPIO_COLUMN_0,
    CONFIG_GPIO_COLUMN_1,
    CONFIG_GPIO_COLUMN_2,
    CONFIG_GPIO_COLUMN_3,
};

static TaskHandle_t numpad_task_handle = 0;
static QueueHandle_t numpad_queue_handle = 0;

void numpad_isr(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendToBackFromISR(numpad_queue_handle, &arg, &xHigherPriorityTaskWoken);

  portYIELD_FROM_ISR_ARG(xHigherPriorityTaskWoken);
}

void numpad_task(void *arg) {
  for (;;) {
    intptr_t column;
    const BaseType_t received =
        xQueueReceive(numpad_queue_handle, (void *)&column, portMAX_DELAY);

    if (received == errQUEUE_EMPTY) {
      ESP_LOGI(tag, "Queue empty, retrying receive");
      continue;
    }

    // Just to catch bounces
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_ERROR_CHECK(gpio_intr_disable(column_pins[column]));

    for (size_t i = 0; i < 4; i++) {
      ESP_ERROR_CHECK(gpio_set_level(row_pins[i], 1));
    }

    intptr_t row = -1;
    for (size_t i = 0; i < 4; i++) {
      ESP_ERROR_CHECK(gpio_set_level(row_pins[i], 0));
      ets_delay_us(5);

      if (gpio_get_level(column_pins[column]) == 0) {
        row = i;
        break;
      }

      ESP_ERROR_CHECK(gpio_set_level(row_pins[i], 1));
    }

    for (size_t i = 0; i < 4; i++) {
      ESP_ERROR_CHECK(gpio_set_level(row_pins[i], 0));
    }

    ESP_ERROR_CHECK(gpio_intr_enable(column_pins[column]));

    if (row != -1) {
      ESP_LOGI(tag, "Key pressed: %c", characters[row][column]);
    }
  }
}

void app_main(void) {
  numpad_queue_handle = xQueueCreate(10, sizeof(intptr_t));

  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LOWMED));

  // Configuring the row output pins
  gpio_config_t row_config = {
      .pin_bit_mask = 0,
      .mode = GPIO_MODE_OUTPUT,
  };

  for (size_t i = 0; i < 4; i++) {
    row_config.pin_bit_mask |= (1ull << row_pins[i]);
  }
  ESP_ERROR_CHECK(gpio_config(&row_config));

  for (size_t i = 0; i < 4; i++) {
    ESP_ERROR_CHECK(gpio_set_level(row_pins[i], 0));
  }

  // Configuring the column input pins
  gpio_config_t column_config = {
      .pin_bit_mask = 0,
      .mode = GPIO_MODE_INPUT,
      .intr_type = GPIO_INTR_NEGEDGE,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  for (size_t i = 0; i < 4; i++) {
    column_config.pin_bit_mask |= (1ull << column_pins[i]);
  }
  ESP_ERROR_CHECK(gpio_config(&column_config));

  for (intptr_t i = 0; i < 4; i++) {
    ESP_ERROR_CHECK(
        gpio_isr_handler_add(column_pins[i], numpad_isr, (void *)i));
  }

  xTaskCreate(numpad_task, "Numpad Task", 4096, NULL, 5, &numpad_task_handle);
}
