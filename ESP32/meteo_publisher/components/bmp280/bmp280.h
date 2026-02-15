/**
 * @file bmp280.h
 * @brief Pure C driver for BMP280 temperature and pressure sensor
 *
 * Provides low-level hardware interface for BMP280 over I²C.
 * Driver is stateless and reusable - all state is stored in bmp280_handle_t.
 * No heap allocation, no global variables, no FreeRTOS task creation.
 */

#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Operating modes for BMP280 sensor
     */
    typedef enum
    {
        BMP280_MODE_WEATHER_MONITORING,   ///< Ultra low power: osrs_p=×1, osrs_t=×1
        BMP280_MODE_HIGH_RESOLUTION,      ///< High quality: osrs_p=×16, osrs_t=×2
        BMP280_MODE_METEO_ULTRA_PRECISION ///< Ultra precision: osrs_p=×16, osrs_t=×16, filter=16
    } bmp280_mode_t;

    /**
     * Configuration for BMP280 sensor
     */
    typedef struct
    {
        i2c_port_t i2c_port;  ///< I²C port number
        uint8_t i2c_addr;     ///< I²C device address
        gpio_num_t sda_pin;   ///< SDA GPIO pin
        gpio_num_t scl_pin;   ///< SCL GPIO pin
        uint32_t i2c_freq_hz; ///< I²C clock frequency
        bmp280_mode_t mode;   ///< Operating mode
    } bmp280_config_t;

    /**
     * Calibration data structure - internal use
     */
    typedef struct
    {
        uint16_t dig_T1;
        int16_t dig_T2;
        int16_t dig_T3;
        uint16_t dig_P1;
        int16_t dig_P2;
        int16_t dig_P3;
        int16_t dig_P4;
        int16_t dig_P5;
        int16_t dig_P6;
        int16_t dig_P7;
        int16_t dig_P8;
        int16_t dig_P9;
        int32_t t_fine; ///< Fine temperature value (internal)
    } bmp280_calib_t;

    /**
     * Mode configuration - internal use
     */
    typedef struct
    {
        uint8_t ctrl_meas_value; ///< Control register value for forced mode
        uint8_t meas_time_ms;    ///< Typical measurement time in milliseconds
    } bmp280_mode_config_t;

    /**
     * BMP280 driver handle - contains all state
     * Application must allocate this structure (stack or static)
     */
    typedef struct
    {
        bmp280_config_t config;
        bmp280_calib_t calib;
        bmp280_mode_config_t mode_config;
        bool initialized;
    } bmp280_handle_t;

    /**
     * Initialize BMP280 sensor
     *
     * @param handle Pointer to driver handle (must be allocated by caller)
     * @param config Pointer to configuration structure
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t bmp280_init(bmp280_handle_t *handle, const bmp280_config_t *config);

    /**
     * Read temperature and pressure from BMP280
     *
     * @param handle Pointer to initialized driver handle
     * @param temp Pointer to store temperature in Celsius
     * @param press Pointer to store pressure in Pascals
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t bmp280_read(bmp280_handle_t *handle, float *temp, float *press);

#ifdef __cplusplus
}
#endif
