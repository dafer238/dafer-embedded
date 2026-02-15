# Orange Pi (Ubuntu 24.04 Noble) – Network, SSH, and Firewall Configuration

This guide explains how to:

- Configure **static IPs** on an Orange Pi running **Ubuntu 24.04 (Noble)**
  - Ethernet: `<ORANGE_PI_IP>`
  - Wi‑Fi: `192.168.1.101`
- Move **SSH** from the default port to **2525**
- Lock the system down with **UFW**, allowing only:
  - SSH (port 2525)
  - Pi‑hole
  - MQTT
  - Internal web services

All examples assume a private network `192.168.1.0/24` with gateway `192.168.1.1`.

---

## 1. Identify Network Interfaces

Verify interface names (they are usually `eth0` and `wlan0`, but don’t guess):

```bash
ip a
```

Take note of the exact interface names before continuing.

---

## 2. Configure Static IPs with Netplan

Ubuntu Noble uses **Netplan** for network configuration.

### 2.1 Edit Netplan Configuration

Open the Netplan file (the filename may differ slightly):

```bash
sudo nano /etc/netplan/01-netcfg.yaml
```

Example configuration:

```yaml
network:
  version: 2
  renderer: networkd

  ethernets:
    eth0:
      dhcp4: no
      addresses:
        - <ORANGE_PI_IP>/24
      routes:
        - to: default
          via: 192.168.1.1
      nameservers:
        addresses:
          - 127.0.0.1
          - 1.1.1.1

  wifis:
    wlan0:
      dhcp4: no
      addresses:
        - 192.168.1.101/24
      routes:
        - to: default
          via: 192.168.1.1
      nameservers:
        addresses:
          - 127.0.0.1
          - 1.1.1.1
      access-points:
        "YOUR_WIFI_SSID":
          password: "YOUR_WIFI_PASSWORD"
```

Notes:
- `127.0.0.1` is included so Pi‑hole can act as the local DNS resolver.
- Replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD`.

### 2.2 Apply the Configuration

```bash
sudo netplan apply
```

Confirm:

```bash
ip a
ip route
```

---

## 3. Move SSH to Port 2525

### 3.1 Edit SSH Configuration

```bash
sudo nano /etc/ssh/sshd_config
```

Change or add:

```text
Port 2525
PermitRootLogin no
PasswordAuthentication yes
```

If multiple `Port` entries exist, comment out port 22.

### 3.2 Restart SSH

```bash
sudo systemctl restart ssh
```

Do **not** close your current session until UFW is updated.

Test from another machine:

```bash
ssh -p 2525 user@<ORANGE_PI_IP>
```

---

## 4. Configure UFW (Firewall)

UFW defaults to denying incoming traffic and allowing outgoing traffic, which is exactly what we want.

### 4.1 Reset and Enable UFW

```bash
sudo ufw reset
sudo ufw default deny incoming
sudo ufw default allow outgoing
```

### 4.2 Allow SSH on Port 2525

```bash
sudo ufw allow 2525/tcp comment "SSH"
```

### 4.3 Allow Pi‑hole

Pi‑hole requires DNS and its web interface:

```bash
sudo ufw allow 53/tcp comment "Pi-hole DNS"
sudo ufw allow 53/udp comment "Pi-hole DNS"
sudo ufw allow 80/tcp comment "Pi-hole Admin"
```

If you use HTTPS for Pi‑hole:

```bash
sudo ufw allow 443/tcp comment "Pi-hole HTTPS"
```

### 4.4 Allow MQTT

Standard MQTT port:

```bash
sudo ufw allow 1883/tcp comment "MQTT"
```

If you use MQTT over TLS:

```bash
sudo ufw allow 8883/tcp comment "MQTT TLS"
```

### 4.5 Allow Internal Web Services

For self‑hosted dashboards or apps:

```bash
sudo ufw allow 80/tcp comment "Internal Web"
sudo ufw allow 443/tcp comment "Internal Web HTTPS"
```

If services should be **LAN‑only**, restrict them:

```bash
sudo ufw allow from 192.168.1.0/24 to any port 80 proto tcp
sudo ufw allow from 192.168.1.0/24 to any port 443 proto tcp
```

### 4.6 Enable UFW

```bash
sudo ufw enable
```

Verify:

```bash
sudo ufw status verbose
```

---

## 5. Final Sanity Checks

- SSH reachable on port 2525
- DNS resolves through Pi‑hole
- MQTT clients can connect
- Web services accessible only from LAN

```bash
ss -tulpen
```

This setup gives you predictable addressing, minimal exposed surface area, and clean separation between services and the outside world.

The machine now behaves like a well‑trained network appliance rather than a chaotic Linux box improvising its identity every reboot.

