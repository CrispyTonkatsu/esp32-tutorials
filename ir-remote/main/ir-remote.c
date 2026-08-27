#include "driver/rmt_types.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/rmt_rx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/rmt_types.h"
#include "portmacro.h"
#include "time.h"
#include <stddef.h>

static const char *tag = "IR-Receiver";
static rmt_channel_handle_t rx_channel_handle = NULL;
static TaskHandle_t decode_ir_handle = NULL;
static QueueHandle_t rx_queue_handle = NULL;

bool rx_recv_done_callback(rmt_channel_handle_t rx_chan,
                           const rmt_rx_done_event_data_t *edata,
                           void *user_ctx) {

  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(rx_queue_handle, edata, &pxHigherPriorityTaskWoken);

  return (pxHigherPriorityTaskWoken == pdTRUE);
}

void decode_ir(void *args) {
  rmt_receive_config_t rx_reception_config = {
      .signal_range_min_ns = 1250,
      .signal_range_max_ns = 10000000,
  };

  rmt_symbol_word_t raw_symbols[64];

  uint32_t input_code = 0;

  while (true) {
    ESP_ERROR_CHECK(rmt_receive(rx_channel_handle, raw_symbols,
                                sizeof(raw_symbols), &rx_reception_config));

    rmt_rx_done_event_data_t edata;
    xQueueReceive(rx_queue_handle, &edata, portMAX_DELAY);

    const rmt_symbol_word_t *leader_code = &edata.received_symbols[0];
    if (leader_code->duration0 < 8000 || leader_code->duration0 > 10000) {
      ESP_LOGW(tag, "Leading 0 is not right: %i", leader_code->duration0);
      continue;
    }

    if (leader_code->duration1 < 4000 || leader_code->duration1 > 5000) {
      if (leader_code->duration1 < 2000 || leader_code->duration1 > 2500) {
        ESP_LOGW(tag, "Leading 1 is not right: %i", leader_code->duration1);
        continue;
      }

      ESP_LOGI(tag, "Repeating previous code");
      continue;
    }

    ESP_LOGI(tag, "NEC Frame Start");

    // for (size_t i = 1; i < edata.num_symbols; i++) {
    //   const rmt_symbol_word_t *word = &edata.received_symbols[i];
    //
    //   // TODO: Left off here, decoding the NEC signal into bits
    //
    // }

    ESP_LOGI(tag, "NEC Frame End");
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

  xTaskCreate(decode_ir, "decode_ir", 4096, NULL, 2, &decode_ir_handle);
}
