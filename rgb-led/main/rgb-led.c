#include <math.h>
#include <stddef.h>

#include "esp_err.h"
#include "hal/ledc_types.h"
#include "sdkconfig.h"
#include "soc/clk_tree_defs.h"

#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_FREQ 4000

#define LIGHT_SATURATION (float)(CONFIG_LIGHT_SATURATION / 256.f)
#define LIGHT_VALUE (float)(CONFIG_LIGHT_VALUE / 256.f)

typedef struct {
  // Range 0 to 360
  float hue;
  // Range 0 to 1
  float value;
  float saturation;
} hsv;

typedef struct {
  // range 0 to 255
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb_discrete;

typedef union {
  rgb_discrete rgb;
  uint8_t components[3];
} rgb;

rgb hsv_to_rgb(hsv color) {
  const float c = color.value * color.saturation;
  const float x = c * (1 - fabsf(fmodf(color.hue / 60.f, 2.f) - 1.f));
  const float m = LIGHT_VALUE - c;

  float red = 0.f;
  float green = 0.f;
  float blue = 0.f;

  if (color.hue < 60.f) {
    red = c;
    green = x;

  } else if (color.hue < 120.f) {
    red = x;
    green = c;

  } else if (color.hue < 180.f) {
    green = c;
    blue = x;

  } else if (color.hue < 240.f) {
    green = x;
    blue = c;

  } else if (color.hue < 300.f) {
    red = x;
    blue = c;

  } else if (color.hue < 360.f) {
    red = c;
    blue = x;
  }

  rgb_discrete output = {
      .red = (red + m) * 255,
      .green = (green + m) * 255,
      .blue = (blue + m) * 255,
  };

  rgb converted_out = {.rgb = output};

  return converted_out;
}

void initialize_pwm(int gpio_pin, int timer, int channel) {
  const ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = timer,
      .freq_hz = LEDC_FREQ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

  ledc_channel_config_t channel_config = {
      .speed_mode = LEDC_MODE,
      .channel = channel,
      .timer_sel = timer,
      .gpio_num = gpio_pin,
      .duty = 0,
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void app_main(void) {

  const int pins[3] = {
      CONFIG_RED_GPIO,
      CONFIG_GREEN_GPIO,
      CONFIG_BLUE_GPIO,
  };

  const int timers[3] = {
      CONFIG_RED_LEDC_TIMER,
      CONFIG_GREEN_LEDC_TIMER,
      CONFIG_BLUE_LEDC_TIMER,
  };

  const int channels[3] = {
      CONFIG_RED_LEDC_CHANNEL,
      CONFIG_GREEN_LEDC_CHANNEL,
      CONFIG_BLUE_LEDC_CHANNEL,
  };

  for (size_t i = 0; i < 3; i++) {
    initialize_pwm(pins[i], timers[i], channels[i]);
  }

  hsv color = {
      .hue = 0.f,
      .saturation = LIGHT_SATURATION,
      .value = LIGHT_VALUE,
  };

  for (;;) {
    color.hue += 5.f;
    color.hue = color.hue > 360.f ? color.hue - 360.f : color.hue;

    rgb rgb_color = hsv_to_rgb(color);

    for (size_t i = 0; i < 3; i++) {
      ESP_ERROR_CHECK(
          ledc_set_duty(LEDC_MODE, channels[i], rgb_color.components[i]));

      ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, channels[i]));
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
