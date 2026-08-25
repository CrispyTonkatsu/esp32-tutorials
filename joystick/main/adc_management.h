#pragma once

#include "esp_adc/adc_continuous.h"

adc_continuous_handle_t
configure_adc(adc_continuous_callback_t conversion_done_callback);
