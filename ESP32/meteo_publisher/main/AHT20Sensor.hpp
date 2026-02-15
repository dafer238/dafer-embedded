/**
 * @file AHT20Sensor.hpp
 * @brief C++ wrapper for AHT20 sensor
 *
 * Wraps the C driver and implements sensor interfaces.
 * Owns C handle, manages initialization, provides high-level API.
 */

#pragma once

#include "SensorInterface.hpp"

extern "C"
{
#include "aht20.h"
}

/**
 * C++ wrapper for AHT20 temperature and humidity sensor
 * Implements TemperatureSensor and HumiditySensor interfaces
 */
class AHT20Sensor : public TempHumiditySensor
{
public:
    /**
     * Constructor - initializes the sensor
     * @param i2c_port I²C port number
     * @param sda_pin SDA GPIO pin
     * @param scl_pin SCL GPIO pin
     * @param i2c_freq_hz I²C clock frequency (typically 100000)
     */
    AHT20Sensor(i2c_port_t i2c_port,
                gpio_num_t sda_pin,
                gpio_num_t scl_pin,
                uint32_t i2c_freq_hz);

    /**
     * Constructor with calibration offsets
     */
    AHT20Sensor(i2c_port_t i2c_port,
                gpio_num_t sda_pin,
                gpio_num_t scl_pin,
                uint32_t i2c_freq_hz,
                float temp_offset,
                float temp_factor,
                float humidity_offset,
                float humidity_factor);

    ~AHT20Sensor() override = default;

    // TemperatureSensor interface
    bool read_celsius(float *temp) override;

    // HumiditySensor interface
    bool read_humidity(float *humidity) override;

    // TempHumiditySensor interface
    bool read_temp_humidity(float *temp, float *humidity) override;

    /**
     * Perform soft reset of sensor
     */
    bool soft_reset();

    /**
     * Check if sensor is initialized
     */
    bool is_initialized() const { return m_initialized; }

    /**
     * Check if sensor is calibrated
     */
    bool is_calibrated() const { return m_handle.calibrated; }

private:
    aht20_handle_t m_handle; ///< C driver handle (owned)
    bool m_initialized;

    // Calibration factors
    float m_temp_offset;
    float m_temp_factor;
    float m_humidity_offset;
    float m_humidity_factor;
};
