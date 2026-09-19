#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "screen.h"

void app_main() {
  const i2c_master_dev_handle_t *screen_handle = initialize_device();
  if (screen_handle == NULL) {
    return;
  }

  clear_buffer();

  for (size_t i = 0; i < OLED_WIDTH; i++) {
    for (size_t j = 0; j < OLED_HEIGHT; j++) {
      if (i % 8 == 0 && j % 8 == 0) {
        paint_pixel(i, j, true);
      }
    }
  }

  while (true) {
    display_buffer();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
