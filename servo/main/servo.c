#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_types.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/mcpwm_prelude.h"
#include "hal/mcpwm_types.h"

#define SERVO_TIMEBASE_RESOLUTION_HZ (1000000)
#define SERVO_TIMEBASE_PERIOD (20000)

#define SERVO_MIN_TICKS (500)
#define SERVO_MAX_TICKS (2500)

#define SERVO_PULSE_GPIO "todo"

static const char *TAG = "MCPWM Servo";

int angle_to_pulse(int angle) {
  angle = angle < 0 ? 0 : (angle > 180 ? 180 : angle);

  return SERVO_MIN_TICKS +
         ((angle * (SERVO_MAX_TICKS - SERVO_MIN_TICKS)) / 180);
}

void app_main(void) {
  mcpwm_timer_handle_t timer_handle = NULL;
  mcpwm_timer_config_t timer_config = {
      .group_id = 0,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
      .period_ticks = SERVO_TIMEBASE_PERIOD,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
  };

  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_handle));
  ESP_LOGI(TAG, "Initialized Timer");

  mcpwm_oper_handle_t operator_handle = NULL;
  mcpwm_operator_config_t operator_config = {
      .group_id = 0, // It needs to match the timer's group
  };

  ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator_handle));
  ESP_LOGI(TAG, "Created the operator");

  ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_handle, timer_handle));
  ESP_LOGI(TAG, "Connected the timer and operator");

  mcpwm_cmpr_handle_t comparator_handle = NULL;
  mcpwm_comparator_config_t comparator_config = {
      .flags.update_cmp_on_tez = true,
  };

  ESP_ERROR_CHECK(mcpwm_new_comparator(operator_handle, &comparator_config,
                                       &comparator_handle));
  ESP_LOGI(TAG, "Creater comparator");

  mcpwm_gen_handle_t generator_handle = NULL;
  mcpwm_generator_config_t generator_config = {
      .gen_gpio_num = CONFIG_SERVO_GPIO,
  };

  ESP_ERROR_CHECK(mcpwm_new_generator(operator_handle, &generator_config,
                                      &generator_handle));
  ESP_LOGI(TAG, "Created generator");

  ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_handle,
                                                     angle_to_pulse(90)));
  ESP_LOGI(TAG, "Set the comparator's value");

  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
      generator_handle, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                     MCPWM_TIMER_EVENT_EMPTY,
                                                     MCPWM_GEN_ACTION_HIGH)));
  ESP_LOGI(TAG, "Bound the timer event for empty period");

  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
      generator_handle,
      MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     comparator_handle, MCPWM_GEN_ACTION_LOW)));
  ESP_LOGI(TAG, "Bound the comparator event for trigger period");

  ESP_ERROR_CHECK(mcpwm_timer_enable(timer_handle));
  ESP_ERROR_CHECK(
      mcpwm_timer_start_stop(timer_handle, MCPWM_TIMER_START_NO_STOP));
  ESP_LOGI(TAG, "Enabled and started the timer with no stop");

  int angle = 0;
  int delta = 1;
  for (;;) {
    angle += delta;

    if (angle > 180) {
      angle = 180;
      delta = -1;
    }

    if (angle < 0) {
      angle = 0;
      delta = 1;
    }

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_handle,
                                                       angle_to_pulse(angle)));

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
