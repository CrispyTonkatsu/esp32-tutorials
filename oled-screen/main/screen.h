#pragma once

#include "driver/i2c_types.h"

#define OLED_WIDTH (128)
#define OLED_HEIGHT (64)
#define OLED_BUFFER_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 8)

uint8_t *get_buffer();

i2c_master_dev_handle_t *initialize_device();

void display_buffer();

void clear_buffer();
