/* Minimal driver for the Sensirion SHTC3 temperature/humidity sensor (I2C).

   Shared by the P3 projects (sesion1 and sesion2) through EXTRA_COMPONENT_DIRS.
   The I2C pins are set in menuconfig ("SHTC3 sensor"); the defaults match the
   ESP32-C3-DevKit-RUST-1 board (SDA = GPIO10, SCL = GPIO8).
*/
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature;      // Degrees Celsius
    float humidity;         // Relative humidity, %
} shtc3_reading_t;

/* Creates the I2C bus and checks that the sensor answers with a valid ID. */
esp_err_t shtc3_init(void);

/* Wakes the sensor up, takes one measurement and puts it back to sleep.
   Blocks for ~15 ms. Must not be called from two tasks at the same time. */
esp_err_t shtc3_read(shtc3_reading_t *out);

#ifdef __cplusplus
}
#endif
