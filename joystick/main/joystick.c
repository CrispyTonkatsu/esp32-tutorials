#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "adc_management.h"

static adc_continuous_handle_t adc_handle = NULL;

static const char *tag = "Joystick Values";
static TaskHandle_t task_print_values_handle = 0;

bool adc_conversion_done_callback(adc_continuous_handle_t handle,
                                  const adc_continuous_evt_data_t *edata,
                                  void *user_data) {
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(task_print_values_handle, &pxHigherPriorityTaskWoken);

  return (pxHigherPriorityTaskWoken == pdTRUE);
}

void print_values_task(void *arg) {
  const uint32_t parsed_data_count = 32;
  adc_continuous_data_t parsed_data[parsed_data_count];

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    ESP_LOGI(tag, "Starting loop");

    uint32_t read_sample_count;
    const esp_err_t read_status = adc_continuous_read_parse(
        adc_handle, parsed_data, parsed_data_count, &read_sample_count, 0);

    if (read_status == ESP_OK) {
      for (int i = 0; i < read_sample_count; i++) {
        if (parsed_data[i].valid) {
          ESP_LOGI(tag, "ADC%d, Channel: %d, Value: %" PRIu32,
                   parsed_data[i].unit + 1, parsed_data[i].channel,
                   parsed_data[i].raw_data);
        }
      }
    } else if (read_status == ESP_ERR_TIMEOUT) {
      ESP_LOGI(tag, "Breaking Loop");
      break;
    } else {
      ESP_LOGE(tag, "Data parsing failed: %s", esp_err_to_name(read_status));
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void app_main(void) {
  adc_handle = configure_adc(adc_conversion_done_callback);

  xTaskCreate(print_values_task, "print_values", 4096, NULL, 5,
              &task_print_values_handle);

  ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}
