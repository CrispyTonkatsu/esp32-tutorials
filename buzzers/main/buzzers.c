#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "hal/ledc_types.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define FREQ_COUNT 8
#define FREQ_STARTING 523

void active_main() {
  gpio_reset_pin(CONFIG_BUZZ_GPIO);
  gpio_set_direction(CONFIG_BUZZ_GPIO, GPIO_MODE_OUTPUT);

  uint32_t state = 1;

  for (;;) {
    gpio_set_level(CONFIG_BUZZ_GPIO, state);
    state = !state;

    vTaskDelay(CONFIG_BUZZ_DURATION / portTICK_PERIOD_MS);
  }
}

void initialize_pwm(int gpio_pin, int timer, int channel) {
  const ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = timer,
      .freq_hz = FREQ_STARTING,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

  ledc_channel_config_t channel_config = {
      .speed_mode = LEDC_MODE,
      .channel = channel,
      .timer_sel = timer,
      .gpio_num = gpio_pin,
      .duty = 128,
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void passive_main() {
  initialize_pwm(CONFIG_BUZZ_GPIO, LEDC_TIMER_0, LEDC_CHANNEL_0);

  const int frequencies[FREQ_COUNT] = {
      523, 587, 659, 698, 784, 880, 988, 1047,
  };

  for (size_t i = 0; i < FREQ_COUNT; i++) {
    ledc_set_freq(LEDC_MODE, LEDC_TIMER_0, frequencies[i]);

    vTaskDelay(CONFIG_BUZZ_DURATION / portTICK_PERIOD_MS);
  }
}

void app_main() {
#ifdef CONFIG_ACTIVE_BUZZER
  active_main();
#else
  passive_main();
#endif
}
