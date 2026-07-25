#include "esp_err.h"

#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"

void app_main(void) {
  adc_continuous_handle_t driver_handle = NULL;
  adc_continuous_handle_cfg_t adc_driver_config = {
      .max_store_buf_size = 2,
      .conv_frame_size = 4,
  };

  ESP_ERROR_CHECK(
      adc_continuous_new_handle(&adc_driver_config, &driver_handle));

  adc_continuous_config_t adc_config = {
      .pattern_num = 2,
      .adc_pattern = NULL,
      .sample_freq_hz = 20 * 1000,
      .conv_mode = ADC_CONV_BOTH_UNIT,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };

  adc_digi_pattern_config_t pattern[2] = {0};
  adc_digi_pattern_config_t config = {
      // TODO: Left off here figuring out what to do with attenuation and the
      // other parameters
      // NOTE: Find out how to determine the attenuation to use here
      .atten = ADC_ATTEN_DB_12,
  };

  for (size_t i = 0; i < 2; i++) {
    pattern[i] = config;
  }
}
