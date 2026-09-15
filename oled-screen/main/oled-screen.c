#include "driver/i2c_master.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t master_handle;
static const i2c_master_bus_config_t master_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .flags.enable_internal_pullup = true,
};

static i2c_master_dev_handle_t screen_handle;
static const i2c_device_config_t screen_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x3C,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0,
};

void app_main(void) {
  // 1. Initialize the I2C master bus
  ESP_ERROR_CHECK(i2c_new_master_bus(&master_config, &master_handle));

  // 2. Add the OLED device to the bus
  ESP_ERROR_CHECK(
      i2c_master_bus_add_device(master_handle, &screen_config, &screen_handle));

  // 3. Send initialization commands (must be prefixed with 0x00)
  const uint8_t initialization_commands[] = {
      0x00, // Sending commands

      0xA8, // Setting MUX ratio
      0x3F,

      0xD3, // Setting display offset
      0x00,

      0x40, // Setting display start line

      0xA0, // Setting segment re-map

      0xC0, // Setting COM output scan direction

      0xDA, // Setting COM pins hardware config
      0x02,

      0x81, // Setting contrast control
      0x7F,

      0xA4, // Disable entire display on

      0xA6, // Set normal display

      0xD5, // Set oscilator frequency
      0x80,

      0x8D, // Enable charge pump
      0x14,

      0xAF, // Turn display on
  };

  ESP_ERROR_CHECK(i2c_master_transmit(screen_handle, initialization_commands,
                                      sizeof(initialization_commands), 1000));

  const uint8_t all_on_cmd[] = {0x00, 0xA5};
  ESP_ERROR_CHECK(
      i2c_master_transmit(screen_handle, all_on_cmd, sizeof(all_on_cmd), 1000));

  bool toggle = true;
  while (true) {

    uint8_t cmd[2] = {0x00, 0};
    if (toggle) {
      cmd[1] = 0xAF;
    } else {
      cmd[1] = 0xAE;
    }

    ESP_ERROR_CHECK(i2c_master_transmit(screen_handle, cmd, sizeof(cmd), 1000));

    toggle = !toggle;

    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}
