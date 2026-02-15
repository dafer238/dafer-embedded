/**
 * @file DHT22Sensor.hpp
 * @brief C++ wrapper for DHT22 sensor
 *
 * Wraps the C driver and implements sensor interfaces.
 * Owns C handle, manages initialization, provides high-level API.
 */

#pragma once

#include "SensorInterface.hpp"

extern "C"
{
#include "dht22.h"
}

/**
 * C++ wrapper for DHT22 temperature and humidity sensor
 * Implements TemperatureSensor and HumiditySensor interfaces
 */
class DHT22Sensor : public TempHumiditySensor
{
public:
    /**
     * Constructor - initializes the sensor
     * @param gpio_pin GPIO pin for data line
     */
    explicit DHT22Sensor(gpio_num_t gpio_pin);

    /**
     * Constructor with calibration offsets
     */
    DHT22Sensor(gpio_num_t gpio_pin,
                float temp_offset,
                float temp_factor,
                float humidity_offset,
                float humidity_factor);

    ~DHT22Sensor() override = default;

    // TemperatureSensor interface
    bool read_celsius(float *temp) override;

    // HumiditySensor interface
    bool read_humidity(float *humidity) override;

    // TempHumiditySensor interface
    bool read_temp_humidity(float *temp, float *humidity) override;

    /**
     * Check if sensor is initialized
     */
    bool is_initialized() const { return m_initialized; }

private:
    dht22_handle_t m_handle; ///< C driver handle (owned)
    bool m_initialized;

    // Calibration factors
    float m_temp_offset;
    float m_temp_factor;
    float m_humidity_offset;
    float m_humidity_factor;
};
