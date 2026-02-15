# Quick Reference Guide

## Adding a New Sensor - Quick Steps

### Example: AHT20 Temperature & Humidity Sensor

A complete AHT20 implementation is included as a reference. See:
- `components/aht20/` - C driver
- `main/AHT20Sensor.hpp/.cpp` - C++ wrapper
- `docs/AHT20_INTEGRATION.md` - Integration guide

### 1. Create C Driver Component (5 minutes)

```bash
mkdir -p components/mysensor
```

**File: `components/mysensor/mysensor.h`**
```c
#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Configuration parameters
    gpio_num_t pin;
    uint32_t frequency;
} mysensor_config_t;

typedef struct {
    mysensor_config_t config;
    // Calibration data, state, etc.
    bool initialized;
} mysensor_handle_t;

esp_err_t mysensor_init(mysensor_handle_t *handle, const mysensor_config_t *config);
esp_err_t mysensor_read(mysensor_handle_t *handle, float *value);

#ifdef __cplusplus
}
#endif
```

**File: `components/mysensor/mysensor.c`**
```c
#include "mysensor.h"

esp_err_t mysensor_init(mysensor_handle_t *handle, const mysensor_config_t *config) {
    // Initialize hardware
    // Store config in handle
    handle->initialized = true;
    return ESP_OK;
}

esp_err_t mysensor_read(mysensor_handle_t *handle, float *value) {
    // Read sensor
    // Return value
    return ESP_OK;
}
```

**File: `components/mysensor/CMakeLists.txt`**
```cmake
idf_component_register(
    SRCS "mysensor.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)
```

### 2. Create C++ Wrapper (5 minutes)

**File: `main/MySensor.hpp`**
```cpp
#pragma once

#include "SensorInterface.hpp"

extern "C" {
#include "mysensor.h"
}

class MySensor : public TemperatureSensor {  // Or other interface
public:
    explicit MySensor(gpio_num_t pin, uint32_t freq);
    bool read_celsius(float *temp) override;
    
private:
    mysensor_handle_t m_handle;
    bool m_initialized;
};
```

**File: `main/MySensor.cpp`**
```cpp
#include "MySensor.hpp"

MySensor::MySensor(gpio_num_t pin, uint32_t freq)
    : m_initialized(false)
{
    mysensor_config_t config = { .pin = pin, .frequency = freq };
    if (mysensor_init(&m_handle, &config) == ESP_OK) {
        m_initialized = true;
    }
}

bool MySensor::read_celsius(float *temp) {
    if (!m_initialized) return false;
    return mysensor_read(&m_handle, temp) == ESP_OK;
}
```

### 3. Update Build (1 minute)

**File: `main/CMakeLists.txt`**
```cmake
idf_component_register(
    SRCS
        ...
        "MySensor.cpp"
    REQUIRES ... mysensor
)
```

### 4. Use in Application (2 minutes)

**File: `main/app.cpp`**
```cpp
MySensor my_sensor(GPIO_NUM_5, 1000);
TemperatureSensor *sensor = &my_sensor;

float temp;
if (sensor->read_celsius(&temp)) {
    ESP_LOGI(TAG, "Temperature: %.2f°C", temp);
}
```

---

## Common Patterns

### Pattern 1: Sensor with Calibration

```cpp
class MySensor : public TemperatureSensor {
public:
    MySensor(gpio_num_t pin, float offset, float factor);
    
private:
    mysensor_handle_t m_handle;
    float m_offset, m_factor;
};

bool MySensor::read_celsius(float *temp) {
    float raw;
    if (mysensor_read(&m_handle, &raw) != ESP_OK) return false;
    *temp = (raw * m_factor) + m_offset;
    return true;
}
```

### Pattern 2: Multi-Value Sensor

```cpp
class MySensor : public TempPressureSensor {
public:
    bool read_temp_pressure(float *temp, float *pressure) override {
        return mysensor_read_both(&m_handle, temp, pressure) == ESP_OK;
    }
    
    bool read_celsius(float *temp) override {
        float pressure;
        return read_temp_pressure(temp, &pressure);
    }
    
    bool read_pressure(float *pressure) override {
        float temp;
        return read_temp_pressure(&temp, pressure);
    }
};
```

### Pattern 3: Polymorphic Sensor Access

```cpp
// Application depends only on interface
void process_temperature(TemperatureSensor *sensor) {
    float temp;
    if (sensor->read_celsius(&temp)) {
        // Do something with temp
    }
}

// Can use any TemperatureSensor implementation
BMP280Sensor bmp280(...);
DHT22Sensor dht22(...);
MockSensor mock;

process_temperature(&bmp280);   // Works
process_temperature(&dht22);    // Works
process_temperature(&mock);     // Works for testing
```

---

## Available Interfaces

```cpp
// Temperature only
class TemperatureSensor {
    virtual bool read_celsius(float *temp) = 0;
};

// Humidity only
class HumiditySensor {
    virtual bool read_humidity(float *humidity) = 0;
};

// Pressure only
class PressureSensor {
    virtual bool read_pressure(float *pressure) = 0;
};

// Temperature + Humidity
class TempHumiditySensor : public TemperatureSensor, public HumiditySensor {
    virtual bool read_temp_humidity(float *temp, float *humidity) = 0;
};

// Temperature + Pressure
class TempPressureSensor : public TemperatureSensor, public PressureSensor {
    virtual bool read_temp_pressure(float *temp, float *pressure) = 0;
};
```

### Add New Interface

```cpp
// File: main/SensorInterface.hpp

class AccelerometerSensor {
public:
    virtual ~AccelerometerSensor() = default;
    virtual bool read_acceleration(float *x, float *y, float *z) = 0;
};
```

---

## Build Commands

```bash
# Build project
idf.py build

# Flash to device
idf.py flash

# Monitor output
idf.py monitor

# Flash and monitor
idf.py flash monitor

# Clean build
idf.py fullclean
idf.py build

# Set target (if needed)
idf.py set-target esp32
```

---

## Debugging Tips

### Check Component Dependencies

```bash
# View component dependencies
idf.py build --verbose
```

### Common Compile Errors

**Error: "undefined reference to `mysensor_init`"**
- Add `mysensor` to `REQUIRES` in main/CMakeLists.txt

**Error: "mysensor.h: No such file or directory"**
- Check component directory structure
- Verify `INCLUDE_DIRS` in component CMakeLists.txt

**Error: "extern "C" ... conflict"**
- Make sure C headers have `extern "C"` guards
- Include C headers inside `extern "C"` block in C++

### Enable Verbose Logging

```c
// In C driver
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
```

```cpp
// In C++ wrapper
ESP_LOGI(TAG, "Initialized: %s", m_initialized ? "yes" : "no");
```

---

## Testing

### Unit Test C Driver (Stub)

```c
// components/mysensor/test/test_mysensor.c
#include "unity.h"
#include "mysensor.h"

TEST_CASE("mysensor_init returns ESP_OK", "[mysensor]")
{
    mysensor_handle_t handle;
    mysensor_config_t config = { .pin = GPIO_NUM_5, .frequency = 1000 };
    
    TEST_ASSERT_EQUAL(ESP_OK, mysensor_init(&handle, &config));
    TEST_ASSERT_TRUE(handle.initialized);
}
```

### Mock Sensor for Application Testing

```cpp
// main/MockSensor.hpp
class MockTempSensor : public TemperatureSensor {
public:
    MockTempSensor(float value) : m_value(value) {}
    
    bool read_celsius(float *temp) override {
        *temp = m_value;
        return true;
    }
    
private:
    float m_value;
};

// Usage
MockTempSensor mock(25.0f);
TemperatureSensor *sensor = &mock;
```

---

## Performance Notes

- Virtual function calls are typically inlined by compiler
- No heap allocation - everything on stack
- Handle size is small (typically < 100 bytes)
- No runtime overhead compared to C

---

## Best Practices

### DO ✅
- Store all state in handles
- Return error codes (ESP_OK, ESP_FAIL, etc.)
- Add `extern "C"` guards to C headers
- Use `const` for config parameters
- Check initialization status
- Log important events
- Document units (Celsius, Pascals, etc.)

### DON'T ❌
- Use global variables in drivers
- Allocate memory on heap
- Create FreeRTOS tasks in drivers
- Use exceptions in C++ wrappers
- Use STL containers
- Forget to check return values
- Mix hardware access in application code

---

## File Naming Conventions

```
components/
  mysensor/          # Lowercase, no underscores for directories
    mysensor.c       # Lowercase with underscores for C
    mysensor.h
    CMakeLists.txt   # Standard CMake naming

main/
  MySensor.hpp       # PascalCase for C++ classes
  MySensor.cpp
  SensorInterface.hpp
  app.cpp            # Lowercase for applications
```

---

## Getting Help

1. Check `ARCHITECTURE.md` for detailed design documentation
2. Check `REFACTORING.md` for before/after examples
3. Look at existing sensors (BMP280, DHT22) as templates
4. Review ESP-IDF documentation: https://docs.espressif.com/projects/esp-idf/

---

## Summary Checklist

When adding a new sensor:
- [ ] Create component directory in `components/`
- [ ] Write C driver (`.c`, `.h`, `CMakeLists.txt`)
- [ ] Add `extern "C"` guards to header
- [ ] Create C++ wrapper (`.hpp`, `.cpp`)
- [ ] Implement appropriate sensor interface
- [ ] Update `main/CMakeLists.txt` with new files and dependencies
- [ ] Use in `app.cpp` via interface pointer
- [ ] Build and test: `idf.py build flash monitor`
