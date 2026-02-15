# Meteo Dashboard

Complete monitoring solution for ESP32 weather sensors with MQTT data collection and web visualization.

## Features

- **MQTT Listener**: Collects sensor data from ESP32 devices via MQTT
- **SQLite Database**: Stores all measurements with timestamps
- **Web Dashboard**: Real-time monitoring interface with:
  - Device status and health monitoring
  - Latest measurements display
  - Interactive charts with historical data
  - Time range filters (1 hour to 1 week)
  - Per-device filtering
  - Statistics overview
- **REST API**: FastAPI endpoints for data access
- **Systemd Services**: Runs as background services on Orange Pi

## Architecture

```
ESP32 Devices → MQTT Broker → main.py (MQTT Listener) → SQLite DB
                                                            ↓
                                                     web_server.py (FastAPI)
                                                            ↓
                                                     Web Dashboard
```

## Installation

### Prerequisites

- Orange Pi Zero3 (or similar Linux SBC)
- Python environment (ape with default venv)
- MQTT Broker (Mosquitto) - see MQTT Broker Setup section below
- SQLite3

### MQTT Broker Setup

If you haven't already set up Mosquitto MQTT broker, follow these steps:

1. **Install Mosquitto**:
   ```bash
   sudo apt update
   sudo apt install mosquitto mosquitto-clients
   sudo systemctl enable mosquitto
   sudo systemctl start mosquitto
   ```

2. **Create password for ESP32 authentication**:
   ```bash
   sudo mosquitto_passwd -c /etc/mosquitto/passwd <your_mqtt_user>
   ```

3. **Configure authentication**:
   Create/edit the configuration file:
   ```bash
   sudo nvim /etc/mosquitto/conf.d/auth.conf
   ```
   
   Add the following content:
   ```conf
   allow_anonymous false
   password_file /etc/mosquitto/passwd
   ```

4. **Set proper permissions**:
   ```bash
   sudo chown root:mosquitto /etc/mosquitto/passwd
   sudo chmod 640 /etc/mosquitto/passwd
   ```

5. **Restart Mosquitto service**:
   ```bash
   sudo systemctl restart mosquitto
   ```

### Setup Steps

1. **Clone repository to Orange Pi**:
   ```bash
   cd ~
   git clone <repository-url> dafer-embedded
   cd dafer-embedded/orangepi/meteo_subscriber
   ```

2. **Configure environment**:
   Edit `.env` file with your MQTT credentials:
   ```
   MQTT_USERNAME=your_username
   MQTT_PASSWORD=your_password
   MQTT_BROKER=<your_broker_ip>
   MQTT_PORT=1883
   MQTT_TOPIC=sensors/#
   SQLITE_DB=environment_data.db
   LOG_LEVEL=INFO
   ```

3. **Run setup script**:
   ```bash
   chmod +x setup.sh
   sudo ./setup.sh
   ```

   This will:
   - Install Python dependencies
   - Copy systemd service files
   - Enable and start both services

4. **Optional: Setup Nginx reverse proxy**:
   ```bash
   chmod +x setup-nginx.sh
   sudo ./setup-nginx.sh
   ```

## Services

### meteo-mqtt.service
- Runs `main.py` to listen for MQTT messages
- Stores sensor data in SQLite database
- Auto-restarts on failure

### meteo-web.service
- Runs FastAPI web server on port 8080
- Serves dashboard at http://<BROKER_IP>:8080/meteo
- Provides REST API endpoints

## Usage

### Access Dashboard

**Direct access** (without Nginx):
```
http://<BROKER_IP>:8080/meteo
```

**With Nginx** (after running setup-nginx.sh):
```
http://<BROKER_IP>/meteo
```

### Service Management

Check status:
```bash
sudo systemctl status meteo-mqtt
sudo systemctl status meteo-web
```

View logs:
```bash
sudo journalctl -u meteo-mqtt -f
sudo journalctl -u meteo-web -f
```

Restart services:
```bash
sudo systemctl restart meteo-mqtt
sudo systemctl restart meteo-web
```

Stop services:
```bash
sudo systemctl stop meteo-mqtt
sudo systemctl stop meteo-web
```

## API Endpoints

### Device Management
- `GET /api/devices/status` - Get all connected devices and their status
- `GET /api/devices/{device_id}/latest` - Get latest data from specific device

### Data Retrieval
- `GET /api/data/latest?limit=N` - Get N latest measurements
- `GET /api/data/history?device_id=X&hours=H&limit=N` - Get historical data
- `GET /api/data/aggregated?device_id=X&hours=H&interval_minutes=M` - Get aggregated data

### Monitoring
- `GET /api/health` - Health check endpoint
- `GET /api/stats` - Overall statistics

## Dashboard Features

### Device Status Cards
- Real-time online/offline status
- Last seen timestamp
- Message count
- Firmware version
- RSSI signal strength

### Data Visualization
- DHT22 Temperature chart
- BMP280 Temperature chart
- Humidity chart
- Pressure chart
- RSSI signal strength chart

### Filters
- Device selector (all devices or specific device)
- Time range selector (1 hour to 1 week)
- Auto-refresh every 10 seconds

### Statistics
- Total measurements
- Total devices
- First/last measurement timestamps
- Per-device averages

## Service Management Commands

### Check service status:
```bash
sudo systemctl status meteo-mqtt
sudo systemctl status meteo-web
```

### View logs:
```bash
# Follow logs in real-time
sudo journalctl -u meteo-mqtt -f
sudo journalctl -u meteo-web -f

# View recent logs
sudo journalctl -u meteo-mqtt -n 50
sudo journalctl -u meteo-web -n 50
```

### Restart services:
```bash
sudo systemctl restart meteo-mqtt
sudo systemctl restart meteo-web

# Or use the restart script
./restart.sh
```

### Stop services:
```bash
sudo systemctl stop meteo-mqtt
sudo systemctl stop meteo-web
```

### Reload service configuration (after editing .service files):
```bash
sudo systemctl daemon-reload
sudo systemctl restart meteo-mqtt
sudo systemctl restart meteo-web
```

## Database Schema

**measurements table**:
- `id` - Auto-increment primary key
- `device_id` - ESP32 device identifier
- `topic` - MQTT topic
- `dht22_temperature_c` - DHT22 temperature (°C)
- `dht22_humidity_percent` - DHT22 humidity (%)
- `bmp280_temperature_c` - BMP280 temperature (°C)
- `bmp280_pressure_pa` - BMP280 pressure (Pa)
- `timestamp_device` - Timestamp from device
- `timestamp_server` - Server timestamp
- `firmware_version` - Device firmware version
- `rssi` - WiFi signal strength (dBm)

Indexes:
- `idx_device_time` on (device_id, timestamp_server)
- `idx_time` on (timestamp_server)

## File Structure

```
sub/
├── main.py                  # MQTT listener
├── web_server.py           # FastAPI web server
├── requirements.txt        # Python dependencies
├── .env                    # Environment configuration
├── meteo-mqtt.service      # Systemd service for MQTT
├── meteo-web.service       # Systemd service for web
├── setup.sh                # Installation script
├── restart.sh              # Quick restart script
├── README.md               # This file
├── templates/
│   └── dashboard.html      # Dashboard HTML template
└── static/
    ├── style.css           # Dashboard styles
    └── dashboard.js        # Dashboard JavaScript
```

**Note**: The project is part of the `dafer-embedded` repository; adjust paths to match your user account (e.g. `/home/<your_user>/dafer-embedded/orangepi/meteo_subscriber`).

## Troubleshooting

### Services won't start
```bash
# Check service logs
sudo journalctl -u meteo-mqtt -n 50
sudo journalctl -u meteo-web -n 50

# Check if Python environment is correct
ls -la /path/to/your/venv/bin/activate
```

### Database issues
```bash
# Check database file
ls -lh environment_data.db

# Access database directly
sqlite3 environment_data.db
> SELECT COUNT(*) FROM measurements;
> SELECT * FROM measurements ORDER BY timestamp_server DESC LIMIT 5;
```

### Web dashboard not loading
```bash
# Check if service is running
sudo systemctl status meteo-web

# Check if port 8080 is listening
sudo netstat -tlnp | grep 8080

# Test API directly
curl http://localhost:8080/api/health
```

### MQTT not receiving data
```bash
# Check MQTT broker status
sudo systemctl status mosquitto

# Test MQTT subscription
mosquitto_sub -h <BROKER_IP> -t "sensors/#" -u <MQTT_USER> -P <MQTT_PASS>

# Check if main.py is running
sudo systemctl status meteo-mqtt
```

## Development

To run locally for development:

```bash
# Activate Python environment
source /path/to/your/venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Run MQTT listener
python main.py

# Run web server (in another terminal)
uvicorn web_server:app --host 0.0.0.0 --port 8080 --reload
```

## Project Paths

This project is part of the `dafer-embedded` repository.

## License

MIT License
