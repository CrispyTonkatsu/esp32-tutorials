#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/soc_caps.h"

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
  return pxHigherPriorityTaskWoken;
}

void print_values_task(void *arg) {
  uint8_t buffer[128];

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    uint32_t read_length;
    ESP_ERROR_CHECK(adc_continuous_read(adc_handle, buffer, sizeof(buffer),
                                        &read_length, 0));

    adc_continuous_data_t parsed_data[read_length / SOC_ADC_DIGI_RESULT_BYTES];
    uint32_t parsed_samples_count;
    ESP_ERROR_CHECK(adc_continuous_parse_data(
        adc_handle, buffer, read_length, parsed_data, &parsed_samples_count));

    for (uint32_t i = 0; i < parsed_samples_count; i++) {
      if (parsed_data[i].valid) {
        ESP_LOGI(tag, "ADC%d, Channel: %d, Value: %" PRIu32,
                 parsed_data[i].unit + 1, parsed_data[i].channel,
                 parsed_data[i].raw_data);
      }
    }
  }
}

void app_main(void) {
  adc_handle = configure_adc(adc_conversion_done_callback);
  adc_continuous_start(adc_handle);

  xTaskCreate(print_values_task, "print_values", 4096, NULL, 5,
              &task_print_values_handle);
}
