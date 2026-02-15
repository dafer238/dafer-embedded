# Embedded IoT Weather Monitoring System

A full-stack IoT environmental monitoring platform built from scratch — from bare-metal ESP32 firmware in C/C++ to a Python-based backend with a real-time web dashboard, all running on self-hosted infrastructure.

## What This Project Does

Multiple ESP32 microcontrollers collect environmental data (temperature, humidity, atmospheric pressure, altitude) from hardware sensors and publish it over MQTT to an Orange Pi SBC acting as the central hub. The Orange Pi runs an MQTT broker, a subscriber that persists data into SQLite, and a FastAPI web dashboard for real-time visualization.

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   ESP32 #1   │     │   ESP32 #2   │     │   ESP32 #N   │
│  BMP280      │     │  AHT20       │     │  DHT22       │
│  AHT20       │     │  BMP280      │     │  BMP280      │
└──────┬───────┘     └──────┬───────┘     └──────┬───────┘
       │ MQTT publish       │ MQTT publish       │ MQTT publish
       └────────────────────┼────────────────────┘
                            │  WiFi / LAN
                    ┌───────▼────────┐
                    │   Orange Pi    │
                    │  ┌──────────┐  │
                    │  │Mosquitto │  │  ← MQTT Broker
                    │  └────┬─────┘  │
                    │  ┌────▼─────┐  │
                    │  │ main.py  │  │  ← MQTT Subscriber → SQLite
                    │  └────┬─────┘  │
                    │  ┌────▼─────┐  │
                    │  │ FastAPI  │  │  ← Web Dashboard + REST API
                    │  └──────────┘  │
                    └────────────────┘
                            │
                    ┌───────▼────────┐
                    │    Browser     │
                    │  📊 Dashboard  │
                    └────────────────┘
```

## Key Technical Highlights

### ESP32 Firmware (C/C++ on ESP-IDF)

- **Bare-metal sensor drivers** written from scratch in C — direct I2C register manipulation for BMP280, AHT20, and bit-banged 1-wire protocol for DHT22
- **Clean C/C++ architecture** with abstract sensor interfaces (`TemperatureSensor`, `HumiditySensor`, `PressureSensor`) enabling sensor interchangeability through polymorphism
- **Component-based build system** — each driver is an isolated ESP-IDF component with its own `CMakeLists.txt`, making them portable to other MCU projects
- **Deep sleep power management** — the ESP32 wakes, reads sensors, publishes over MQTT, and goes back to deep sleep to minimize power consumption
- **Runtime-configurable** via ESP-IDF's `menuconfig` (Kconfig) — WiFi credentials, MQTT broker, sensor selection, GPIOs, and publish interval are all configurable without touching source code
- **NeoPixel RGB & GPIO LED signaling** for status indication during boot, sensor reads, and MQTT publish
- **Altitude calculation** from barometric pressure using the standard atmosphere formula

### Orange Pi Backend (Python)

- **MQTT subscriber** (`paho-mqtt`) that parses structured JSON payloads and persists measurements into SQLite
- **FastAPI web server** with a real-time dashboard featuring:
  - Device status cards with online/offline indicators
  - Interactive Chart.js graphs (temperature, humidity, pressure, RSSI)
  - Time range filters (1h to 1 week) and per-device filtering
  - Auto-refresh every 10 seconds
- **REST API** — device status, latest readings, historical data, aggregated data, health checks, and statistics
- **Systemd integration** — both the MQTT listener and web server run as managed services with auto-restart
- **Nginx reverse proxy** configuration for production deployment

### Infrastructure & Networking

- **Self-hosted MQTT broker** (Mosquitto) with user authentication
- **Cloudflare Tunnel** to expose the dashboard publicly at [meteo.neodafer.com](https://meteo.neodafer.com) — no open ports required, accessible from anywhere as a live demo
- **WireGuard VPN** setup for secure remote access
- **Firewall hardening** (UFW) with minimal open ports
- **Dynamic DNS** integration with Cloudflare API
- **Custom PCB design** (KiCad) for sensor breakout boards

## Project Structure

```
├── ESP32/
│   └── meteo_publisher/
│       ├── components/          # Reusable C sensor drivers
│       │   ├── aht20/           # AHT20 I2C driver
│       │   ├── bmp280/          # BMP280 I2C driver
│       │   ├── dht22/           # DHT22 1-wire driver
│       │   ├── led/             # LED & NeoPixel driver
│       │   ├── mqtt_pub/        # MQTT publish module
│       │   └── wifi/            # WiFi connection module
│       ├── main/
│       │   ├── app.cpp          # Application entry point
│       │   ├── SensorInterface.hpp  # Abstract sensor interfaces
│       │   ├── BMP280Sensor.cpp/hpp # C++ wrapper
│       │   ├── AHT20Sensor.cpp/hpp  # C++ wrapper
│       │   ├── DHT22Sensor.cpp/hpp  # C++ wrapper
│       │   └── Kconfig          # Build-time configuration menu
│       └── docs/                # Architecture & wiring docs
├── orangepi/
│   └── meteo_subscriber/
│       ├── main.py              # MQTT listener + SQLite logger
│       ├── web_server.py        # FastAPI dashboard & REST API
│       ├── templates/           # Dashboard HTML (Chart.js)
│       ├── static/              # CSS & JS assets
│       ├── meteo-mqtt.service   # Systemd service definitions
│       ├── meteo-web.service
│       └── setup.sh             # Automated deployment script
├── docs/
│   ├── OrangePI/                # Server setup guides
│   ├── ESP32/                   # ESP32 reference docs
│   └── BMP280/                  # Sensor datasheets
└── pcb/                         # KiCad PCB designs
```

## Sensors Supported

| Sensor     | Interface       | Measurements          | Notes                                |
| ---------- | --------------- | --------------------- | ------------------------------------ |
| **BMP280** | I2C (0x76/0x77) | Temperature, Pressure | Altitude derived from pressure       |
| **AHT20**  | I2C (0x38)      | Temperature, Humidity | High precision (±0.3°C, ±2% RH)      |
| **DHT22**  | 1-wire (GPIO)   | Temperature, Humidity | Bit-banged protocol, no dependencies |

Sensors can be enabled/disabled independently via `menuconfig`. Multiple sensor configurations are supported — from single-sensor nodes to full redundancy setups with all three sensors.

## Applied Knowledge & Competencies

Core engineering disciplines required to design, build, and deploy this system end-to-end:

- **Embedded Systems Programming** — bare-metal I2C and 1-wire driver development, timing-critical protocol implementation, hardware register manipulation, and sensor calibration/compensation algorithms
- **Real-Time Operating Systems** — FreeRTOS task orchestration, event groups, and synchronization primitives on ESP-IDF
- **IoT System Architecture** — MQTT publish/subscribe topology design, structured JSON payload schemas, and fault-tolerant handling of unreliable sensor data
- **C/C++ Interoperability** — clean boundary design between C hardware drivers and C++ application logic using `extern "C"` linkage and abstract interfaces without STL/RTTI/exceptions (embedded constraints)
- **Low-Power Design** — deep sleep wake-cycle implementation to minimize power consumption on battery-operated ESP32 nodes
- **Linux Server Administration** — systemd service management, Nginx reverse proxy, UFW firewall hardening, WireGuard VPN, and dynamic DNS via Cloudflare API on an ARM SBC
- **Full-Stack Web Development** — FastAPI REST API with SQLite persistence, and a responsive real-time dashboard built with Chart.js
- **PCB Design** — custom sensor breakout boards designed in KiCad
- **DevOps & Deployment** — automated provisioning scripts, service health monitoring, and structured logging

## Getting Started

### ESP32 Firmware

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) (v5.x).

```bash
cd ESP32/meteo_publisher
idf.py menuconfig       # Configure WiFi, MQTT, sensors, GPIOs
idf.py build
idf.py flash monitor
```

### Orange Pi Subscriber

```bash
cd orangepi/meteo_subscriber
cp .env.example .env     # Edit with your MQTT credentials
pip install -r requirements.txt
python main.py           # Start MQTT listener
uvicorn web_server:app --host 0.0.0.0 --port 8080  # Start dashboard
```

See [orangepi/meteo_subscriber/README.md](orangepi/meteo_subscriber/README.md) for full deployment instructions including systemd and Nginx setup.

## License

MIT

