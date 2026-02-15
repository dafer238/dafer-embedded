# Sensor Ecosystem

## Implemented Sensors

### 1. BMP280 - Temperature & Pressure
```
┌──────────────────────────────┐
│      BMP280 Sensor          │
│  I²C Address: 0x76/0x77     │
├──────────────────────────────┤
│ Temperature: -40°C to 85°C   │
│ Accuracy: ±1°C              │
│ Pressure: 300-1100 hPa      │
│ Accuracy: ±1 hPa            │
│ Measurement: ~80-100ms      │
└──────────────────────────────┘
```

**Use Cases:**
- Weather stations
- Altitude calculation
- Atmospheric pressure monitoring
- Indoor air quality

### 2. DHT22 - Temperature & Humidity
```
┌──────────────────────────────┐
│      DHT22 Sensor           │
│  Interface: 1-wire          │
├──────────────────────────────┤
│ Temperature: -40°C to 80°C   │
│ Accuracy: ±0.5°C            │
│ Humidity: 0-100% RH         │
│ Accuracy: ±2-5% RH          │
│ Measurement: ~2 seconds     │
└──────────────────────────────┘
```

**Use Cases:**
- Indoor monitoring
- Greenhouse control
- HVAC systems
- Simple weather stations

### 3. AHT20 - Temperature & Humidity ⭐ NEW
```
┌──────────────────────────────┐
│      AHT20 Sensor           │
│  I²C Address: 0x38          │
├──────────────────────────────┤
│ Temperature: -40°C to 85°C   │
│ Accuracy: ±0.3°C            │
│ Humidity: 0-100% RH         │
│ Accuracy: ±2% RH            │
│ Measurement: ~80ms          │
└──────────────────────────────┘
```

**Use Cases:**
- High-precision monitoring
- Industrial applications
- Medical equipment
- HVAC systems

## Sensor Comparison Matrix

| Feature             | BMP280    | DHT22    | AHT20     |
| ------------------- | --------- | -------- | --------- |
| **Temperature**     | ✅         | ✅        | ✅         |
| **Humidity**        | ❌         | ✅        | ✅         |
| **Pressure**        | ✅         | ❌        | ❌         |
| **Interface**       | I²C       | 1-wire   | I²C       |
| **Speed**           | Fast      | Slow     | Fast      |
| **Accuracy (Temp)** | Good      | Good     | Excellent |
| **Accuracy (Hum)**  | N/A       | Good     | Excellent |
| **CPU Usage**       | Low       | High     | Low       |
| **Cost**            | Low       | Very Low | Low       |
| **Reliability**     | Excellent | Good     | Excellent |

## Recommended Combinations

### Configuration 1: Weather Station
```
BMP280 (I²C 0x76)  → Temperature, Pressure, Altitude
DHT22 (GPIO 4)     → Humidity, Temperature (redundant)
```
**Benefits**: Independent interfaces, redundant temperature readings

### Configuration 2: Precision Indoor Monitoring
```
BMP280 (I²C 0x76)  → Pressure, Altitude
AHT20 (I²C 0x38)   → Temperature, Humidity (high precision)
```
**Benefits**: Both on I²C bus, faster measurements, better accuracy

### Configuration 3: Maximum Redundancy
```
BMP280 (I²C 0x76)  → Temperature #1, Pressure
DHT22 (GPIO 4)     → Temperature #2, Humidity #1
AHT20 (I²C 0x38)   → Temperature #3, Humidity #2
```
**Benefits**: Average readings for maximum accuracy, sensor failure detection

### Configuration 4: Minimal (Low Power)
```
AHT20 (I²C 0x38)   → Temperature, Humidity only
```
**Benefits**: Single sensor, fast measurements, low power consumption

## Interface Summary

### I²C Bus (SDA: GPIO21, SCL: GPIO22)
```
ESP32 I²C Bus
│
├─── BMP280 (Address 0x76 or 0x77)
│     └─── Temperature + Pressure
│
└─── AHT20  (Address 0x38)
      └─── Temperature + Humidity
```

### 1-Wire (GPIO 4)
```
ESP32 GPIO4
│
└─── DHT22
      └─── Temperature + Humidity
```

## Wiring Diagram

```
                    ESP32-DevKit-C
                    ┌─────────────┐
                    │             │
   [BMP280]─────────┤ GPIO21 (SDA)│
   [AHT20 ]─────────┤ GPIO22 (SCL)│
                    │             │
   [DHT22  ]────────┤ GPIO4       │
                    │             │
   [LED    ]────────┤ GPIO2       │
                    │             │
                    │ 3.3V ───────┼───┬─── BMP280 VCC
                    │             │   ├─── AHT20 VCC
                    │             │   └─── DHT22 VCC
                    │             │
                    │ GND  ───────┼───┬─── BMP280 GND
                    │             │   ├─── AHT20 GND
                    │             │   └─── DHT22 GND
                    └─────────────┘
```

## Code Interface

All sensors implement standard interfaces:

```cpp
// Temperature interface
class TemperatureSensor {
    virtual bool read_celsius(float *temp) = 0;
};

// Humidity interface
class HumiditySensor {
    virtual bool read_humidity(float *humidity) = 0;
};

// Pressure interface
class PressureSensor {
    virtual bool read_pressure(float *pressure) = 0;
};

// Combined interfaces
class TempHumiditySensor : public TemperatureSensor, public HumiditySensor {
    virtual bool read_temp_humidity(float *temp, float *humidity) = 0;
};

class TempPressureSensor : public TemperatureSensor, public PressureSensor {
    virtual bool read_temp_pressure(float *temp, float *pressure) = 0;
};
```

## Polymorphic Usage

```cpp
// Create sensors
BMP280Sensor bmp280(...);
DHT22Sensor dht22(...);
AHT20Sensor aht20(...);

// Use via interfaces
TemperatureSensor* temp_sensors[] = {&bmp280, &dht22, &aht20};

// Read all temperature sensors
for (auto sensor : temp_sensors) {
    float temp;
    if (sensor->read_celsius(&temp)) {
        ESP_LOGI(TAG, "Temperature: %.2f°C", temp);
    }
}
```

## Future Sensor Candidates

### Easy to Add (I²C)
- **BME680**: Temperature, Humidity, Pressure, Gas (IAQ)
- **SHT31**: High-precision Temperature + Humidity
- **HDC1080**: Low-power Temperature + Humidity
- **SI7021**: Temperature + Humidity
- **LPS22HB**: Pressure sensor

### Medium Complexity (I²C)
- **CCS811**: Air quality (eCO2, TVOC)
- **SGP30**: Air quality (eCO2, TVOC)
- **BME280**: Like BMP280 but with humidity

### Other Interfaces
- **MQ-135**: Air quality (analog)
- **DS18B20**: Temperature (1-wire)
- **MAX6675**: Thermocouple (SPI)

## Performance Metrics

| Sensor | Init Time | Read Time | CPU Load | Power (mW) |
| ------ | --------- | --------- | -------- | ---------- |
| BMP280 | ~50ms     | ~80ms     | Low      | ~3         |
| DHT22  | ~10ms     | ~2000ms   | High     | ~2.5       |
| AHT20  | ~50ms     | ~80ms     | Low      | ~2.5       |

## Recommended Reading Order

1. **Start**: `ARCHITECTURE.md` - Understand overall design
2. **Migrate**: `REFACTORING.md` - See before/after
3. **Quick**: `QUICK_REFERENCE.md` - Add new sensors
4. **AHT20**: `docs/AHT20_INTEGRATION.md` - Specific example
5. **Status**: `STATUS.md` - Current state
6. **This**: `SENSOR_ECOSYSTEM.md` - Sensor overview
