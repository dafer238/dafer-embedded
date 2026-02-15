# Refactored Architecture

This project implements a clean C/C++ boundary for sensor drivers, enabling:
- **Reusability**: Drivers can be ported to other MCUs with minimal changes
- **Interchangeability**: Sensors can be swapped via C++ interfaces
- **Maintainability**: Clear separation of hardware and application logic

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│          Application Logic (C++)                    │
│  app.cpp - FreeRTOS tasks, business logic          │
│  Depends only on interfaces, not implementations    │
└─────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│       C++ Sensor Wrappers (main/)                   │
│  BMP280Sensor.cpp, DHT22Sensor.cpp                 │
│  - Own C driver handles as private members          │
│  - Call C init() in constructor                     │
│  - Implement sensor interfaces                      │
│  - Provide high-level API (read_celsius(), etc.)    │
└─────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│       Sensor Interfaces (main/)                     │
│  SensorInterface.hpp                                │
│  - Abstract base classes (TemperatureSensor, etc.)  │
│  - Pure virtual methods                             │
│  - No STL, exceptions, RTTI, or dynamic allocation  │
└─────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│         C Drivers (components/)                     │
│  bmp280/, dht22/                                    │
│  - Pure C: GPIO, I²C, timing, register access       │
│  - typedef struct handle (all state)                │
│  - init() and read() functions                      │
│  - No globals, no heap, no FreeRTOS task creation   │
│  - Portable to other MCUs                           │
└─────────────────────────────────────────────────────┘
```

## Directory Structure

```
ESP32/meteo_publisher/
├── components/              # Reusable C drivers
│   ├── bmp280/
│   │   ├── bmp280.c        # Pure C implementation
│   │   ├── bmp280.h        # C API with handle
│   │   └── CMakeLists.txt
│   └── dht22/
│       ├── dht22.c         # Pure C implementation
│       ├── dht22.h         # C API with handle
│       └── CMakeLists.txt
├── main/                    # Application code
│   ├── app.cpp             # Main application (C++)
│   ├── SensorInterface.hpp # Abstract interfaces
│   ├── BMP280Sensor.hpp    # C++ wrapper header
│   ├── BMP280Sensor.cpp    # C++ wrapper implementation
│   ├── DHT22Sensor.hpp     # C++ wrapper header
│   ├── DHT22Sensor.cpp     # C++ wrapper implementation
│   ├── wifi.c              # WiFi module (C)
│   ├── mqtt_pub.c          # MQTT module (C)
│   ├── led.c               # LED module (C)
│   └── CMakeLists.txt
└── CMakeLists.txt           # Root build file
```

## Key Design Principles

### 1. C Drivers (components/)

**Stateless and Reusable:**
- All state stored in `_handle_t` structure allocated by caller (stack or static)
- No global variables
- No heap allocation
- No FreeRTOS task creation

**Example - BMP280:**
```c
typedef struct {
    bmp280_config_t config;
    bmp280_calib_t calib;
    bmp280_mode_config_t mode_config;
    bool initialized;
} bmp280_handle_t;

esp_err_t bmp280_init(bmp280_handle_t *handle, const bmp280_config_t *config);
esp_err_t bmp280_read(bmp280_handle_t *handle, float *temp, float *press);
```

**Portability:**
- Hardware abstraction limited to driver
- Easy to port to different MCU by changing GPIO/I²C calls
- No ESP-IDF specific logic except hardware access

### 2. C++ Wrappers (main/)

**Ownership:**
- Each wrapper owns one C driver handle as private member
- Constructor calls C `init()` function
- Handle lifetime tied to wrapper object lifetime

**Example - BMP280Sensor:**
```cpp
class BMP280Sensor : public TempPressureSensor {
public:
    BMP280Sensor(i2c_port_t port, uint8_t addr, ...);
    bool read_celsius(float *temp) override;
    bool read_pressure(float *pressure) override;
    bool read_temp_pressure(float *temp, float *pressure) override;
    
private:
    bmp280_handle_t m_handle;  // Owned C driver handle
    bool m_initialized;
    float m_temp_offset, m_temp_factor;
    float m_press_offset, m_press_factor;
};
```

**No Direct Hardware Access:**
- Wrappers do NOT manipulate GPIO/I²C directly
- All hardware access delegated to C driver
- Wrappers add calibration, error handling, high-level API

### 3. Sensor Interfaces (main/)

**Polymorphism Without STL:**
- Abstract base classes with pure virtual methods
- Allow interchangeable sensor implementations
- No exceptions, RTTI, or dynamic allocation

**Example:**
```cpp
class TemperatureSensor {
public:
    virtual ~TemperatureSensor() = default;
    virtual bool read_celsius(float *temp) = 0;
};

class TempPressureSensor : public TemperatureSensor, public PressureSensor {
public:
    virtual bool read_temp_pressure(float *temp, float *pressure) = 0;
};
```

### 4. Application Logic (main/app.cpp)

**Depends on Interfaces, Not Implementations:**
```cpp
TempHumiditySensor *temp_humidity_sensor = &dht22;
temp_humidity_sensor->read_temp_humidity(&temp, &humidity);

TempPressureSensor *temp_pressure_sensor = &bmp280;
temp_pressure_sensor->read_temp_pressure(&temp, &pressure);
```

**Benefits:**
- Can swap sensor implementations without changing logic
- Test with mock sensors
- Supports multiple sensors of same type

## Building

Standard ESP-IDF build process:

```bash
idf.py build
idf.py flash monitor
```

## Adding a New Sensor

### 1. Create C Driver Component

```bash
mkdir -p components/newsensor
```

Create `newsensor.h`:
```c
#pragma once

typedef struct {
    /* sensor configuration */
} newsensor_config_t;

typedef struct {
    newsensor_config_t config;
    /* calibration, state, etc. */
    bool initialized;
} newsensor_handle_t;

esp_err_t newsensor_init(newsensor_handle_t *handle, const newsensor_config_t *config);
esp_err_t newsensor_read(newsensor_handle_t *handle, float *value);
```

Create `newsensor.c` with implementation and `CMakeLists.txt`.

### 2. Create C++ Wrapper

Create `main/NewSensor.hpp`:
```cpp
#pragma once

#include "SensorInterface.hpp"

extern "C" {
#include "newsensor.h"
}

class NewSensor : public TemperatureSensor {
public:
    NewSensor(/* config params */);
    bool read_celsius(float *temp) override;
    
private:
    newsensor_handle_t m_handle;
    bool m_initialized;
};
```

Create `main/NewSensor.cpp` with implementation.

### 3. Update Build System

Add to `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS
        ...
        "NewSensor.cpp"
    REQUIRES ... newsensor
)
```

### 4. Use in Application

```cpp
NewSensor sensor(/* config */);
TemperatureSensor *temp_sensor = &sensor;
temp_sensor->read_celsius(&temp);
```

## Migration from Original Code

The refactoring preserves all functionality:

### Before (main.c):
```c
static struct { ... } calib;  // Global state

esp_err_t bmp280_init(bmp280_mode_t mode) {
    // Uses global calib
}

void bmp280_read(float *temp, float *press, ...) {
    // Modifies global calib.t_fine
}
```

### After (components/bmp280/):
```c
typedef struct {
    bmp280_calib_t calib;  // State in handle
    ...
} bmp280_handle_t;

esp_err_t bmp280_init(bmp280_handle_t *handle, const bmp280_config_t *config);
esp_err_t bmp280_read(bmp280_handle_t *handle, float *temp, float *press);
```

### Application (main/app.cpp):
```cpp
BMP280Sensor bmp280(I2C_NUM_0, addr, sda, scl, freq, mode, ...);
bmp280.read_temp_pressure(&temp, &pressure);
```

## Testing

The architecture supports testing at multiple levels:

1. **C Driver Unit Tests**: Test pure C drivers in isolation
2. **C++ Wrapper Tests**: Test wrapper logic with mock C drivers
3. **Integration Tests**: Test full sensor stack
4. **Mock Sensors**: Implement interfaces for application testing

## Constraints Observed

✅ **ESP-IDF Compatible**: Uses `idf_component_register`, FreeRTOS, ESP APIs
✅ **No Arduino APIs**: Pure ESP-IDF
✅ **No Global State**: All state in handles or objects
✅ **No Heap Allocation**: Stack-allocated handles
✅ **No STL**: Plain C++ without standard library containers
✅ **No Exceptions**: Error handling via return codes
✅ **No RTTI**: No `dynamic_cast` or `typeid`
✅ **Preserves Functionality**: Same electrical and timing behavior

## Benefits

1. **Reusability**: C drivers portable to other MCUs
2. **Maintainability**: Clear boundaries, single responsibility
3. **Testability**: Mock interfaces, isolated unit tests
4. **Interchangeability**: Swap sensors via interfaces
5. **Clarity**: Separation of concerns (hardware vs. logic)
6. **Type Safety**: C++ compile-time checks
7. **No Overhead**: Zero-cost abstractions, inline virtual calls

## Future Enhancements

- Add more sensor types (accelerometer, gyroscope, etc.)
- Create sensor factory for runtime selection
- Add calibration manager
- Implement sensor fusion algorithms
- Support multiple instances of same sensor
- Add power management interfaces
