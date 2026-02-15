/**
 * @file SensorInterface.hpp
 * @brief Abstract C++ interfaces for sensors
 *
 * Provides polymorphic interfaces for interchangeable sensor types.
 * No STL, no exceptions, no RTTI, no dynamic allocation.
 */

#pragma once

/**
 * Interface for temperature sensors
 * Pure virtual base class - implement in concrete sensor wrappers
 */
class TemperatureSensor
{
public:
    virtual ~TemperatureSensor() = default;

    /**
     * Read temperature in Celsius
     * @param temp Pointer to store temperature value
     * @return true on success, false on error
     */
    virtual bool read_celsius(float *temp) = 0;
};

/**
 * Interface for humidity sensors
 * Pure virtual base class - implement in concrete sensor wrappers
 */
class HumiditySensor
{
public:
    virtual ~HumiditySensor() = default;

    /**
     * Read relative humidity in percent
     * @param humidity Pointer to store humidity value
     * @return true on success, false on error
     */
    virtual bool read_humidity(float *humidity) = 0;
};

/**
 * Interface for pressure sensors
 * Pure virtual base class - implement in concrete sensor wrappers
 */
class PressureSensor
{
public:
    virtual ~PressureSensor() = default;

    /**
     * Read atmospheric pressure in Pascals
     * @param pressure Pointer to store pressure value
     * @return true on success, false on error
     */
    virtual bool read_pressure(float *pressure) = 0;
};

/**
 * Interface for combined temperature and humidity sensors
 */
class TempHumiditySensor : public TemperatureSensor, public HumiditySensor
{
public:
    virtual ~TempHumiditySensor() = default;

    /**
     * Read both temperature and humidity in one operation
     * @param temp Pointer to store temperature in Celsius
     * @param humidity Pointer to store humidity in percent
     * @return true on success, false on error
     */
    virtual bool read_temp_humidity(float *temp, float *humidity) = 0;
};

/**
 * Interface for combined temperature and pressure sensors
 */
class TempPressureSensor : public TemperatureSensor, public PressureSensor
{
public:
    virtual ~TempPressureSensor() = default;

    /**
     * Read both temperature and pressure in one operation
     * @param temp Pointer to store temperature in Celsius
     * @param pressure Pointer to store pressure in Pascals
     * @return true on success, false on error
     */
    virtual bool read_temp_pressure(float *temp, float *pressure) = 0;
};
