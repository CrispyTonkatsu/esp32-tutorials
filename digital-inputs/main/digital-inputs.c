#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "hal/gpio_types.h"

#include "driver/uart.h"

#include "hal/uart_types.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUFF_SIZE (1024)

void config_pins() {
  gpio_reset_pin(CONFIG_LED_GPIO);
  gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);

  gpio_reset_pin(CONFIG_INPUT_GPIO);
  gpio_set_direction(CONFIG_INPUT_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(CONFIG_INPUT_GPIO, GPIO_PULLUP_ONLY);
}

void config_uart() {
  const uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(UART_NUM_0, BUFF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
}

void parse_command(uint8_t *command, size_t *length, int *state) {
  for (size_t i = 0; i < *length; i++) {
    if (command[i] == '\b') {
      *length = 0;
      return;
    }
  }

  const char *toggle = "toggle\r";
  if (strcmp((char *)command, toggle) != 0) {
    return;
  }

  *state = !*state;
  gpio_set_level(CONFIG_LED_GPIO, *state);
  *length = 0;
}

void app_main(void) {
  config_uart();

  config_pins();

  int state = 0;
  gpio_set_level(CONFIG_LED_GPIO, state);

  uint8_t *data = (uint8_t *)malloc(BUFF_SIZE);

  uint8_t *command = (uint8_t *)malloc(BUFF_SIZE);
  command[0] = '\0';

  size_t command_len = 0;

  for (;; vTaskDelay(100 / portTICK_PERIOD_MS)) {

    // Button logic
    if (gpio_get_level(CONFIG_INPUT_GPIO) == 0) {
      state = !state;

      gpio_set_level(CONFIG_LED_GPIO, state);

      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // Command logic
    const int len = uart_read_bytes(UART_NUM_0, data, (BUFF_SIZE - 1),
                                    20 / portTICK_PERIOD_MS);

    if (len != 0) {
      // Guard against overflowing the command
      if (command_len + len + 1 >= BUFF_SIZE) {
        command_len = 0;
      }

      for (size_t i = 0; i < len; i++) {
        command[command_len + i] = data[i];
      }

      command_len += len;
      command[command_len] = '\0';

      parse_command(command, &command_len, &state);

      printf("\rCommand: %s", command);
      fflush(stdout);
    }
  }
}
