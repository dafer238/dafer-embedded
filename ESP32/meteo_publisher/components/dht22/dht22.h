/**
 * @file dht22.h
 * @brief Pure C driver for DHT22 temperature and humidity sensor
 *
 * Provides low-level hardware interface for DHT22 using 1-wire protocol.
 * Driver is stateless and reusable - all state is stored in dht22_handle_t.
 * No heap allocation, no global variables, no FreeRTOS task creation.
 */

#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Configuration for DHT22 sensor
     */
    typedef struct
    {
        gpio_num_t gpio_pin; ///< GPIO pin for data line
    } dht22_config_t;

    /**
     * DHT22 driver handle - contains all state
     * Application must allocate this structure (stack or static)
     */
    typedef struct
    {
        dht22_config_t config;
        bool initialized;
    } dht22_handle_t;

    /**
     * Initialize DHT22 sensor
     *
     * @param handle Pointer to driver handle (must be allocated by caller)
     * @param config Pointer to configuration structure
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t dht22_init(dht22_handle_t *handle, const dht22_config_t *config);

    /**
     * Read temperature and humidity from DHT22
     *
     * @param handle Pointer to initialized driver handle
     * @param temp Pointer to store temperature in Celsius
     * @param humidity Pointer to store relative humidity in percent
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t dht22_read(dht22_handle_t *handle, float *temp, float *humidity);

#ifdef __cplusplus
}
#endif
