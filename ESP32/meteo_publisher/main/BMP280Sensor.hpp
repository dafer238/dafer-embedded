/**
 * @file BMP280Sensor.hpp
 * @brief C++ wrapper for BMP280 sensor
 *
 * Wraps the C driver and implements sensor interfaces.
 * Owns C handle, manages initialization, provides high-level API.
 */

#pragma once

#include "SensorInterface.hpp"

extern "C"
{
#include "bmp280.h"
}

/**
 * C++ wrapper for BMP280 temperature and pressure sensor
 * Implements TemperatureSensor and PressureSensor interfaces
 */
class BMP280Sensor : public TempPressureSensor
{
public:
    /**
     * Constructor - initializes the sensor
     * @param i2c_port I²C port number
     * @param i2c_addr I²C device address
     * @param sda_pin SDA GPIO pin
     * @param scl_pin SCL GPIO pin
     * @param i2c_freq_hz I²C clock frequency
     * @param mode Operating mode
     */
    BMP280Sensor(i2c_port_t i2c_port,
                 uint8_t i2c_addr,
                 gpio_num_t sda_pin,
                 gpio_num_t scl_pin,
                 uint32_t i2c_freq_hz,
                 bmp280_mode_t mode);

    /**
     * Constructor with calibration offsets
     */
    BMP280Sensor(i2c_port_t i2c_port,
                 uint8_t i2c_addr,
                 gpio_num_t sda_pin,
                 gpio_num_t scl_pin,
                 uint32_t i2c_freq_hz,
                 bmp280_mode_t mode,
                 float temp_offset,
                 float temp_factor,
                 float press_offset,
                 float press_factor);

    ~BMP280Sensor() override = default;

    // TemperatureSensor interface
    bool read_celsius(float *temp) override;

    // PressureSensor interface
    bool read_pressure(float *pressure) override;

    // TempPressureSensor interface
    bool read_temp_pressure(float *temp, float *pressure) override;

    /**
     * Check if sensor is initialized
     */
    bool is_initialized() const { return m_initialized; }

private:
    bmp280_handle_t m_handle; ///< C driver handle (owned)
    bool m_initialized;

    // Calibration factors
    float m_temp_offset;
    float m_temp_factor;
    float m_press_offset;
    float m_press_factor;
};
