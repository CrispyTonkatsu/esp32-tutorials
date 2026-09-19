#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "screen.h"

void app_main() {
  const i2c_master_dev_handle_t *screen_handle = initialize_device();
  if (screen_handle == NULL) {
    return;
  }

  clear_buffer();

  size_t dissipation_step = 1;
  const size_t dissipation_max = 32;

  while (true) {

    for (size_t i = 0; i < OLED_WIDTH; i++) {
      for (size_t j = 0; j < OLED_HEIGHT; j++) {
        if (i % dissipation_step == 0 && j % dissipation_step == 0) {
          paint_pixel(i, j, true);
        }
      }
    }

    display_buffer();

    dissipation_step *= 2;
    dissipation_step =
        dissipation_step > dissipation_max ? 1 : dissipation_step;

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
