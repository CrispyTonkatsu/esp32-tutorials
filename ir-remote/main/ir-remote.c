#include "driver/rmt_types.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/rmt_rx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/rmt_types.h"
#include "portmacro.h"

static const char *tag = "IR-Receiver";
static rmt_channel_handle_t rx_channel_handle = NULL;
static TaskHandle_t print_inputs_handle = NULL;
static QueueHandle_t rx_queue_handle = NULL;

bool rx_recv_done_callback(rmt_channel_handle_t rx_chan,
                           const rmt_rx_done_event_data_t *edata,
                           void *user_ctx) {
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(rx_queue_handle, edata, &pxHigherPriorityTaskWoken);

  return (pxHigherPriorityTaskWoken == pdTRUE);
}

void print_inputs(void *args) {
  rmt_receive_config_t rx_reception_config = {
      .signal_range_min_ns = 1250,
      .signal_range_max_ns = 1000000,
  };

  rmt_symbol_word_t raw_symbols[64];
  ESP_ERROR_CHECK(rmt_receive(rx_channel_handle, raw_symbols,
                              sizeof(raw_symbols), &rx_reception_config));

  while (true) {
    rmt_rx_done_event_data_t *edata;
    xQueueReceive(rx_queue_handle, &edata, portMAX_DELAY);

    // TODO: Left off here, decoding the NEC signal into bits

    ESP_LOGI(tag, "Input received: %i", edata->received_symbols);

    ESP_ERROR_CHECK(rmt_receive(rx_channel_handle, raw_symbols,
                                sizeof(raw_symbols), &rx_reception_config));
  }
}

void app_main(void) {
  rx_queue_handle = xQueueCreate(8, sizeof(void *));

  rmt_rx_channel_config_t rx_channel_config = {
      .gpio_num = GPIO_NUM_19,
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 1000000,
      .mem_block_symbols = 64,
      .intr_priority = 3,
  };

  ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_config, &rx_channel_handle));

  rmt_rx_event_callbacks_t callbacks = {
      .on_recv_done = rx_recv_done_callback,
  };

  ESP_ERROR_CHECK(rmt_enable(rx_channel_handle));
  rmt_rx_register_event_callbacks(rx_channel_handle, &callbacks, NULL);

  xTaskCreate(print_inputs, "print_inputs", 4096, NULL, 2,
              &print_inputs_handle);
}
