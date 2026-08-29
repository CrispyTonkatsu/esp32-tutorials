#include "esp_err.h"
#include "esp_log.h"

#include "driver/rmt_rx.h"
#include "driver/rmt_types.h"
#include "hal/rmt_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

static const char *tag = "IR-Receiver";
static rmt_channel_handle_t rx_channel_handle = NULL;
static TaskHandle_t decode_ir_handle = NULL;
static QueueHandle_t rx_queue_handle = NULL;

static TaskHandle_t print_values_handle = NULL;
static QueueHandle_t decoded_values_queue_handle = NULL;

bool rx_recv_done_callback(rmt_channel_handle_t rx_chan,
                           const rmt_rx_done_event_data_t *edata,
                           void *user_ctx) {

  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(rx_queue_handle, edata, &pxHigherPriorityTaskWoken);

  return (pxHigherPriorityTaskWoken == pdTRUE);
}

bool enqueue_decoded_value(uint32_t value) {
  const BaseType_t enqueue_result =
      xQueueSend(decoded_values_queue_handle, &value, 0);

  if (enqueue_result == errQUEUE_FULL) {
    ESP_LOGW(tag, "The queue for decoded values is full");
    return false;
  }

  return true;
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
    input_code = 0;

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
      enqueue_decoded_value(input_code);

      continue;
    }

    ESP_LOGI(tag, "NEC Frame Start");

    bool failed = false;

    for (size_t i = 1; i < edata.num_symbols; i++) {
      const rmt_symbol_word_t *word = &edata.received_symbols[i];

      ESP_LOGI(tag, "Values are: 0:%i, 1:%i", word->duration0, word->duration1);

      if (word->duration0 < 400 || word->duration0 > 700) {
        ESP_LOGW(tag, "The bit header is of invalid duration: %i",
                 word->duration0);

        failed = true;
        break;
      }

      if (word->duration1 == 0) {
        break;
      }

      input_code <<= 1;
      input_code |= (word->duration1 > 1000) ? 1 : 0;
    }

    if (failed) {
      continue;
    }

    enqueue_decoded_value(input_code);
    ESP_LOGI(tag, "NEC Frame End");
  }
}

void print_values(void *args) {
  while (true) {
    uint32_t value;
    xQueueReceive(decoded_values_queue_handle, &value, portMAX_DELAY);

    ESP_LOGI(tag, "Input received: %i", value);
  }
}

void app_main(void) {
  rx_queue_handle = xQueueCreate(8, sizeof(void *));
  decoded_values_queue_handle = xQueueCreate(8, sizeof(uint32_t));

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

  xTaskCreate(decode_ir, "decode_ir", 4096, NULL, 3, &decode_ir_handle);
  xTaskCreate(print_values, "print_values", 4096, NULL, 1,
              &print_values_handle);
}
