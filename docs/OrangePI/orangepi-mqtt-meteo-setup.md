# Orange Pi Meteo System – Setup Summary

## 1. MQTT Broker (Mosquitto)

### Purpose
Central message hub. ESP32 nodes publish sensor data; backend services subscribe.

### Service
- **Service:** mosquitto.service
- **Binary:** /usr/sbin/mosquitto
- **Manager:** systemd

Check status:
```bash
systemctl status mosquitto
```

---

### Configuration Files

#### Main config
**Path**
```
/etc/mosquitto/mosquitto.conf
```

**Key directive**
```
include_dir /etc/mosquitto/conf.d
```

Loads extra configuration files.

---

#### Authentication config
**Path**
```
/etc/mosquitto/conf.d/auth.conf
```

**Contents**
```conf
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Enables network access, disables anonymous connections, and enforces authentication.

---

#### Password file
**Path**
```
/etc/mosquitto/passwd
```

Created with:
```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd <your_mqtt_user>
```

Permissions:
```bash
sudo chown root:mosquitto /etc/mosquitto/passwd
sudo chmod 640 /etc/mosquitto/passwd
```

---

### Verification
```bash
sudo netstat -tuln | grep 1883
mosquitto_sub -h <BROKER_IP> -u <MQTT_USER> -P <MQTT_PASS> -t '#'
```

---

## 2. Firewall (UFW)

### Purpose
Controls inbound network access.

Enabled with:
```bash
sudo ufw enable
```

Allowed ports:
```bash
sudo ufw allow 1883/tcp
sudo ufw allow 8080/tcp
```

Check:
```bash
sudo ufw status
```

---

## 3. MQTT Subscriber + SQLite Logger

### Project directory
```
/home/<your_user>/dafer-embedded/orangepi/meteo_subscriber/
```

---

### Environment file
**Path**
```
.env
```

```env
MQTT_USERNAME=<your_mqtt_user>
MQTT_PASSWORD=<your_mqtt_password>
MQTT_BROKER=<your_broker_ip>
MQTT_PORT=1883
MQTT_TOPIC=sensors/#
SQLITE_DB=environment_data.db
LOG_LEVEL=INFO
```

---

### MQTT → SQLite service
**File**
```
main.py
```

Functionality:
- Connects to MQTT broker
- Subscribes to sensor topics
- Parses JSON payloads
- Stores measurements into SQLite

---

### Database
**File**
```
environment_data.db
```

Inspect:
```bash
sqlite3 environment_data.db
```

```sql
.headers on
.mode column
SELECT * FROM measurements LIMIT 10;
```

---

## 4. Python Virtual Environment

**Path**
```
/home/<your_user>/code/python/venvs/denv/
```

Activate manually:
```bash
source /home/<your_user>/code/python/venvs/denv/bin/activate
```

Used automatically by systemd services.

---

## 5. Web Dashboard (FastAPI + Uvicorn)

### Web server file
**Path**
```
/home/<your_user>/dafer-embedded/orangepi/meteo_subscriber/web_server.py
```

Exports:
```python
app = FastAPI()
```

---

### Systemd service
**Path**
```
/etc/systemd/system/meteo-mqtt.service
```
```ini
[Unit]
Description=Meteo MQTT Listener Service
After=network.target mosquitto.service

[Service]
Type=simple
User=<your_user>
WorkingDirectory=/home/<your_user>/code/embedded/dafer-embedded/orangepi/meteo_subscriber
ExecStart=/bin/bash -c 'source /path/to/your/venv/bin/activate && python main.py'
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=meteo-mqtt

[Install]
WantedBy=multi-user.target
```

```
/etc/systemd/system/meteo-web.service
```

```ini
[Unit]
Description=Meteo Web Dashboard Service
After=network.target meteo-mqtt.service

[Service]
Type=simple
User=<your_user>
WorkingDirectory=/home/<your_user>/code/embedded/dafer-embedded/orangepi/meteo_subscriber
ExecStart=/bin/bash -c 'source /path/to/your/venv/bin/activate && uvicorn web_server:app --host 0.0.0.0 --port 8080'
Restart=always
AmbientCapabilities=CAP_NET_BIND_SERVICE
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=meteo-web

[Install]
WantedBy=multi-user.target
```

Reload and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable meteo-web
sudo systemctl restart meteo-web
```

---

### Access
```
http://<BROKER_IP>:8080/meteo
```

---

## 6. System Architecture Overview

ESP32 nodes  
→ MQTT publish  
→ Mosquitto broker  
→ Python subscriber  
→ SQLite database  
→ FastAPI web server  
→ Browser dashboard
