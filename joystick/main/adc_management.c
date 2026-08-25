#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "hal/adc_types.h"

adc_continuous_handle_t
configure_adc(adc_continuous_callback_t conversion_done_callback) {
  adc_continuous_handle_t driver_handle = NULL;
  adc_continuous_handle_cfg_t adc_driver_config = {
      .max_store_buf_size = 1024,
      .conv_frame_size = 256,
  };

  ESP_ERROR_CHECK(
      adc_continuous_new_handle(&adc_driver_config, &driver_handle));

  adc_continuous_config_t adc_config = {
      .pattern_num = 2,
      .adc_pattern = NULL,
      .sample_freq_hz = 20 * 1000,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };

  adc_digi_pattern_config_t patterns[2] = {0};
  adc_digi_pattern_config_t config = {
      .atten = ADC_ATTEN_DB_12,
      .channel = ADC_CHANNEL_6,
      .unit = ADC_UNIT_1,
      .bit_width = ADC_BITWIDTH_12,
  };

  for (size_t i = 0; i < 2; i++) {
    patterns[i] = config;
    patterns[i].channel += i;
  }

  adc_config.adc_pattern = patterns;

  ESP_ERROR_CHECK(adc_continuous_config(driver_handle, &adc_config));

  adc_continuous_evt_cbs_t callbacks = {
      .on_pool_ovf = NULL,
      .on_conv_done = conversion_done_callback,
  };

  ESP_ERROR_CHECK(
      adc_continuous_register_event_callbacks(driver_handle, &callbacks, NULL));

  return driver_handle;
}
