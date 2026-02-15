/**
 * @file aht20.h
 * @brief Pure C driver for AHT20 temperature and humidity sensor
 *
 * Provides low-level hardware interface for AHT20 over I²C.
 * Driver is stateless and reusable - all state is stored in aht20_handle_t.
 * No heap allocation, no global variables, no FreeRTOS task creation.
 *
 * AHT20 is a high-precision temperature and humidity sensor with I²C interface.
 * Temperature range: -40°C to +85°C, accuracy: ±0.3°C
 * Humidity range: 0-100% RH, accuracy: ±2% RH
 */

#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// AHT20 I²C address (fixed)
#define AHT20_I2C_ADDR 0x38

    /**
     * Configuration for AHT20 sensor
     */
    typedef struct
    {
        i2c_port_t i2c_port;  ///< I²C port number
        gpio_num_t sda_pin;   ///< SDA GPIO pin
        gpio_num_t scl_pin;   ///< SCL GPIO pin
        uint32_t i2c_freq_hz; ///< I²C clock frequency (typically 100000)
    } aht20_config_t;

    /**
     * AHT20 driver handle - contains all state
     * Application must allocate this structure (stack or static)
     */
    typedef struct
    {
        aht20_config_t config;
        bool initialized;
        bool calibrated;
    } aht20_handle_t;

    /**
     * Initialize AHT20 sensor
     *
     * Configures I²C bus and initializes sensor. Performs calibration check.
     *
     * @param handle Pointer to driver handle (must be allocated by caller)
     * @param config Pointer to configuration structure
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t aht20_init(aht20_handle_t *handle, const aht20_config_t *config);

    /**
     * Read temperature and humidity from AHT20
     *
     * Triggers measurement and reads results.
     * Measurement takes approximately 80ms.
     *
     * @param handle Pointer to initialized driver handle
     * @param temp Pointer to store temperature in Celsius
     * @param humidity Pointer to store relative humidity in percent
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t aht20_read(aht20_handle_t *handle, float *temp, float *humidity);

    /**
     * Perform soft reset of AHT20 sensor
     *
     * @param handle Pointer to initialized driver handle
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t aht20_soft_reset(aht20_handle_t *handle);

#ifdef __cplusplus
}
#endif
