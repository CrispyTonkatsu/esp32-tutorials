#include "screen.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2c_master.h"

static uint8_t buffer[OLED_BUFFER_SIZE] = {0x00};

static const char *tag = "Screen Driver";

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

uint8_t *get_buffer() { return buffer; }

i2c_master_dev_handle_t *initialize_device() {
  const esp_err_t master_error =
      i2c_new_master_bus(&master_config, &master_handle);

  if (master_error != 0) {
    ESP_LOGE(tag, "The driver failed to create the bus with error %i",
             master_error);
    return NULL;
  }

  const esp_err_t screen_error =
      i2c_master_bus_add_device(master_handle, &screen_config, &screen_handle);
  if (screen_error != 0) {
    ESP_LOGE(tag, "The driver failed to set the bus with error %i",
             screen_error);
    return NULL;
  }

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
      0x12, // Setting the asscreen 128x64

      0x81, // Setting contrast control
      0x7F,

      0xA4, // Disable entire display on

      0xA6, // Set normal display

      0xD5, // Set oscilator frequency
      0x80,

      0x8D, // Enable charge pump
      0x14,

      0x20, // Setting memory addressing mode
      0x00, // Horizontal addressing mode

      0xAF, // Turn display on
  };

  ESP_ERROR_CHECK(i2c_master_transmit(screen_handle, initialization_commands,
                                      sizeof(initialization_commands), 1000));

  return &screen_handle;
}

void display_buffer() {
  // Resetting the position of the cursor in the case that it was moved or
  // interrupted
  const uint8_t addr_commands[] = {
      0x00,         // Command stream
      0x21, 0, 127, // Column start 0, end 0
      0x22, 0, 7    // Page start 0, end 7 (all 8 pages)
  };
  ESP_ERROR_CHECK(i2c_master_transmit(screen_handle, addr_commands,
                                      sizeof(addr_commands), 1000));

  // Actually drawing the buffer
  const uint8_t display_command = 0x40;
  const i2c_master_transmit_multi_buffer_info_t display_command_info = {
      .write_buffer = &display_command,
      .buffer_size = sizeof(display_command),
  };

  for (size_t i = 0; i < 8; i++) {
    const i2c_master_transmit_multi_buffer_info_t data_buffer_info = {
        .write_buffer = &buffer[i * OLED_WIDTH],
        .buffer_size = OLED_WIDTH,
    };

    i2c_master_transmit_multi_buffer_info_t command_buffer_infos[] = {
        display_command_info,
        data_buffer_info,
    };

    ESP_ERROR_CHECK(i2c_master_multi_buffer_transmit(
        screen_handle, command_buffer_infos, 2, 1000));
  }
}

void clear_buffer() { memset(buffer, 0x00, sizeof(buffer)); }
