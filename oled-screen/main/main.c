#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "screen.h"

void app_main() {
  const i2c_master_dev_handle_t *screen_handle = initialize_device();
  if (screen_handle == NULL) {
    return;
  }

  clear_buffer();

  uint8_t *const buffer = get_buffer();

  const size_t step_size = 8;
  size_t current_length = 0;

  while (true) {
    display_buffer();

    for (size_t i = 0; i < current_length; i++) {
      buffer[i] = 0xFF;
    }

    current_length += step_size;

    if (current_length >= OLED_BUFFER_SIZE) {
      current_length = 0;
      clear_buffer();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
