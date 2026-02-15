# ESP32-C3 Consumer‑Grade IoT Initialization & Configuration Guide

This document captures **consumer‑grade best practices** for initializing, provisioning, configuring, and operating an ESP32‑C3 device.
It is intentionally **architecture‑level**, not tied to a specific library or framework, so you can refine implementation details later.

The goal:  
A device that behaves like a **polished, well‑established IoT product**, not a hobby board.

---

## 1. Design Philosophy

Think of the ESP32‑C3 as an **appliance with a lifecycle**, not a microcontroller running code.

Lifecycle phases:

1. Boot & identity
2. First‑time provisioning
3. Network discovery
4. Normal operation
5. Configuration & control
6. OTA updates
7. Recovery & reset

Each phase must be explicit, deterministic, and recoverable.

---

## 2. Device Identity (First Boot)

On first power‑up, the device must establish a **stable identity**.

### Identity Rules
- Identity is created once and persisted
- Identity does not change across reboots
- Identity only resets on explicit factory reset

### Recommended Identity Elements
- Unique device ID (derived from MAC / chip ID / UUID)
- Human‑readable hostname derived from ID

Example:
```
Device ID: c3-7f3a91
Hostname: esp32-c3-7f3a91
mDNS name: esp32-c3-7f3a91.local
```

Store identity in **NVS** immediately.

---

## 3. First‑Time Provisioning (Wi‑Fi SoftAP)

Wi‑Fi provisioning via **SoftAP + captive portal** is recommended for first configuration because:
- No phone app required
- Works on any device with a browser
- Matches user expectations for consumer IoT

### Provisioning Flow
1. Device boots with no Wi‑Fi credentials
2. Device starts a temporary Wi‑Fi AP
3. Captive portal opens automatically
4. User enters Wi‑Fi credentials
5. Device validates connection
6. Credentials are stored in NVS
7. Device reboots into normal mode
8. SoftAP is permanently disabled

### Provisioning AP Guidelines
- SSID clearly indicates setup mode  
  Example: `ESP32-C3-Setup-7F3A`
- No internet access required
- Minimal, fast-loading UI
- Clear success/failure feedback

### Failure Handling
- If Wi‑Fi connection fails, return to provisioning mode
- Never brick the device due to bad credentials

---

## 4. Network Join & mDNS Advertisement

Once connected to the user’s Wi‑Fi network, the device should **announce itself**.

### mDNS Configuration
- Hostname: stable, derived from identity
- Advertise `_http._tcp` service
- Publish TXT records

Recommended TXT records:
- `model=esp32-c3`
- `fw_version=1.0.0`
- `state=configured`
- `api_version=1`

This enables zero‑configuration discovery:
```
http://esp32-c3-7f3a91.local
```

### mDNS Rules
- Enable only in infrastructure Wi‑Fi mode
- Do not run mDNS during SoftAP provisioning
- Restart mDNS automatically after Wi‑Fi reconnects

---

## 5. Configuration Interface (Post‑Provisioning)

Once online, the device exposes **one clear configuration surface**.

### Recommended Approach
- HTTP server for human interaction
- JSON REST API for programmatic access

### Web UI Responsibilities
- Status overview
- Network info
- Device configuration
- Firmware update
- Factory reset

### API Responsibilities
- Runtime configuration changes
- Triggering behaviors
- Diagnostics and metrics
- Integration with automation systems

Configuration must:
- Be validated before applying
- Be persisted atomically
- Be versioned for future compatibility

---

## 6. Normal Operation

During normal operation, the device should be:
- Quiet on the network
- Deterministic in behavior
- Resilient to Wi‑Fi loss

### Networking Expectations
- Use DHCP
- Reconnect automatically
- Exponential backoff on failures
- No broadcast spamming

### Runtime Control
Supported via:
- HTTP endpoints
- MQTT topics (optional)
- Local network only by default

---

## 7. OTA Updates (Mandatory)

OTA is not optional for a consumer‑grade device.

### OTA Requirements
- Authenticated update endpoint
- Firmware integrity verification
- Rollback on failure
- Clear version reporting

OTA should be boring, reliable, and rarely noticed.

---

## 8. Recovery & Factory Reset

Every consumer device needs an escape hatch.

### Required Reset Mechanisms (at least one)
- Long‑press hardware button
- Power‑cycle pattern
- Protected network command

### Factory Reset Behavior
- Clear Wi‑Fi credentials
- Clear user configuration
- Preserve device identity (optional)
- Re‑enter provisioning mode

---

## 9. Security Posture

Security should be calm and pragmatic.

### Local Network
- No unauthenticated config endpoints
- Provisioning disabled after setup
- Secrets never logged or exposed

### Internet Access
- Outbound connections only
- TLS where feasible
- No public inbound services

---

## 10. DNS & Naming Expectations

### Supported
- mDNS (`.local`)
- Router‑assigned DHCP IPs

### Not Required
- Static DNS
- Static IPs

If users want permanent DNS names, this is configured on:
- Router
- Local DNS server (Pi‑hole, Unbound)

The device does not manage DNS authority.

---

## 11. Mental Model Summary

A polished ESP32‑C3 behaves like a **networked appliance**:

- It introduces itself
- It is easy to configure
- It is discoverable
- It is updateable
- It is recoverable
- It never traps the user

If you design around lifecycle instead of features, even a tiny ESP32‑C3 feels professional.

---

## 12. Next Steps

This document defines *what* to build, not *how*.

Future refinement areas:
- Exact ESP‑IDF components
- Provisioning UI design
- API schema
- Security model
- OTA backend

Treat this as the contract your firmware must honor.
