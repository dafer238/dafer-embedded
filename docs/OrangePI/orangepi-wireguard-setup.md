# Orange Pi WireGuard & Network Configuration Summary

This document is a complete reference of the VPN, firewall, DNS, and routing setup on the Orange Pi.
It is intended as future-facing documentation to understand, audit, or rebuild the system.

---

## 1. WireGuard (VPN Core)

### Installed package
- **wireguard**
  - Provides the kernel module and tools (`wg`, `wg-quick`) implementing the VPN.

---

### Directory: `/etc/wireguard/`

#### `server_private.key`
- **Purpose**: Private cryptographic identity of the WireGuard server.
- **Notes**:
  - Must never leave the Orange Pi.
  - File permissions are restricted.
  - Losing it invalidates all clients.

#### `server_public.key`
- **Purpose**: Public key derived from the server private key.
- **Usage**:
  - Copied into client configurations to authenticate the server.

#### `wg0.conf`
- **Purpose**: Main and authoritative WireGuard interface configuration.
- **Defines**:
  - VPN address (`10.8.0.1/24`)
  - Listening port (`51820/udp`)
  - Server private key
  - All allowed peers (clients)

- **Important**:
  - `SaveConfig = true` was removed.
  - The file is the single source of truth.
  - Changes require restarting the service.

---

### Systemd Service

- **Service name**: `wg-quick@wg0`
- **Purpose**:
  - Brings up interface `wg0`
  - Applies routes and firewall rules
  - Loads configuration from `wg0.conf`

- **Commands**:
```bash
systemctl enable wg-quick@wg0
systemctl start wg-quick@wg0
systemctl restart wg-quick@wg0
```

---

### Runtime inspection

- **Command**:
```bash
wg show
```

- **Purpose**:
  - Shows active interface state
  - Lists peers
  - Displays handshake timestamps and traffic counters

---

## 2. Kernel Networking

### File: `/etc/sysctl.conf`

- **Setting**:
```ini
net.ipv4.ip_forward=1
```

- **Purpose**:
  - Allows packet forwarding between interfaces.
  - Required for VPN-to-LAN access.

- **Applied via**:
```bash
sysctl -p
```

---

## 3. Firewall (UFW)

### Status
- UFW enabled

---

### File: `/etc/default/ufw`

- **Setting changed**:
```ini
DEFAULT_FORWARD_POLICY="ACCEPT"
```

- **Purpose**:
  - Allows routed/forwarded traffic.

---

### UFW Rules Added

#### Allow WireGuard
```bash
ufw allow 51820/udp
```
- Opens only the VPN port to the internet.

#### Allow VPN ↔ LAN routing
```bash
ufw route allow in on wg0 out on eth0
ufw route allow in on eth0 out on wg0
```
- Permits traffic between VPN and local network.

---

### Reload
```bash
ufw reload
```

---

## 4. Router Configuration (External)

### Port Forwarding Rule

- **Protocol**: UDP
- **WAN Port**: 51820
- **LAN IP**: <ORANGE_PI_IP>
- **LAN Port**: 51820

- **Purpose**:
  - Allows incoming WireGuard connections through NAT.
  - Only one port is exposed.

---

## 5. DNS & Dynamic IP (Cloudflare)

### DNS Record

- **Type**: A
- **Name**: `vpn.example.com`
- **Content**: Home public IP
- **Proxy**: OFF (DNS only)

---

### Dynamic DNS Script

#### File: `/usr/local/bin/cloudflare-ddns.sh`

- **Purpose**:
  - Automatically updates the DNS record if the public IP changes.
- **Uses**:
  - Cloudflare API
  - `curl` for HTTP
  - `jq` for JSON parsing

---

### Dependencies
```bash
apt install curl jq
```

---

### Cron Job

- **Location**: root crontab
- **Entry**:
```
*/5 * * * * /usr/local/bin/cloudflare-ddns.sh
```

- **Purpose**:
  - Keeps DNS synchronized every 5 minutes.

---

## 6. Client Routing Behavior

### WireGuard Client Config

```ini
AllowedIPs = 192.168.1.0/24, 10.8.0.0/24
```

- **Effect**:
  - Split tunneling
  - Home network traffic uses VPN
  - Regular internet traffic remains local

---

## 7. What This Enables

When connected to the VPN:

- SSH access to the Orange Pi
- Access to internal web services, e.g.:
  - `http://<ORANGE_PI_IP>/admin`
  - `http://<ORANGE_PI_IP>:8080/meteo`
- No internal services exposed publicly

---

## 8. Security Posture Summary

- Only UDP port 51820 is exposed externally
- All internal services are private
- VPN provides cryptographic authentication
- Firewall explicitly controls routing

---

## 9. Debugging Guide

- VPN issues → `/etc/wireguard`, `wg show`
- Routing issues → `sysctl`, UFW rules
- Reachability issues → router port forwarding, DNS record

---

## Final Note

This system is built in layers:
1. Identity (WireGuard keys)
2. Path (routing & forwarding)
3. Policy (firewall rules)

Each layer is explicit, inspectable, and maintainable.
