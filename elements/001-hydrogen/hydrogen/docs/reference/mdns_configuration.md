# mDNS Configuration Guide

## Overview

The mDNS (Multicast DNS) server enables automatic discovery of your Hydrogen printer on local networks. It advertises various services including the web interface, WebSocket endpoint, and OctoPrint compatibility layer. This guide explains how to configure the mDNS server through the `mDNSServer` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration to enable mDNS discovery:

```json
{
    "mDNSServer": {
        "Enabled": true,
        "DeviceId": "hydrogen-printer",
        "FriendlyName": "Hydrogen 3D Printer",
        "Services": [
            {
                "Name": "Hydrogen_Web_Interface",
                "Type": "_http._tcp",
                "Port": 5000
            }
        ]
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Enabled` | Turns mDNS server on/off | `true` | Required |
| `EnableIPv6` | Enables IPv6 support | `false` | Optional |
| `DeviceId` | Unique device identifier | `"hydrogen-printer"` | Required |
| `FriendlyName` | Human-readable name | `"Hydrogen 3D Printer"` | Required |
| `Model` | Device model | `"Hydrogen"` | Optional |
| `Manufacturer` | Device manufacturer | `"Philement"` | Optional |
| `Version` | Software version | `"0.1.0"` | Optional |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### Service Configuration

Each service in the `Services` array can have:

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Name` | Service name | - | Required |
| `Type` | Service type | - | Required |
| `Port` | Service port | - | Required |
| `TxtRecords` | Additional information | - | Optional |

## Standard Service Types

Hydrogen supports several standard service types:

1. **HTTP Interface**:

   ```json
   {
       "Name": "Hydrogen_Web_Interface",
       "Type": "_http._tcp",
       "Port": 5000,
       "TxtRecords": "path=/api/upload"
   }
   ```

2. **OctoPrint Compatibility**:

   ```json
   {
       "Name": "Hydrogen_OctoPrint",
       "Type": "_octoprint._tcp",
       "Port": 5000,
       "TxtRecords": "path=/api,version=1.1.0"
   }
   ```

3. **WebSocket Updates**:

   ```json
   {
       "Name": "Hydrogen_WebSocket",
       "Type": "_websocket._tcp",
       "Port": 5001,
       "TxtRecords": "path=/websocket"
   }
   ```

## Common Configurations

### Basic Development Setup

```json
{
    "mDNSServer": {
        "Enabled": true,
        "DeviceId": "hydrogen-dev",
        "FriendlyName": "Hydrogen Dev Printer",
        "LogLevel": "DEBUG",
        "Services": [
            {
                "Name": "Hydrogen_Web",
                "Type": "_http._tcp",
                "Port": 8080
            }
        ]
    }
}
```

### Production Setup

```json
{
    "mDNSServer": {
        "Enabled": true,
        "EnableIPv6": true,
        "DeviceId": "hydrogen-printer",
        "FriendlyName": "Hydrogen 3D Printer",
        "Model": "Hydrogen",
        "Manufacturer": "Philement",
        "Version": "1.0.0",
        "LogLevel": "WARN",
        "Services": [
            {
                "Name": "Hydrogen_Web_Interface",
                "Type": "_http._tcp",
                "Port": 5000,
                "TxtRecords": "path=/api/upload"
            },
            {
                "Name": "Hydrogen_OctoPrint",
                "Type": "_octoprint._tcp",
                "Port": 5000,
                "TxtRecords": "path=/api,version=1.1.0"
            },
            {
                "Name": "Hydrogen_WebSocket",
                "Type": "_websocket._tcp",
                "Port": 5001,
                "TxtRecords": "path=/websocket"
            }
        ]
    }
}
```

### Multiple Printer Setup

```json
{
    "mDNSServer": {
        "Enabled": true,
        "DeviceId": "${env.HYDROGEN_DEVICE_ID}",
        "FriendlyName": "${env.HYDROGEN_FRIENDLY_NAME}",
        "Services": [
            {
                "Name": "${env.HYDROGEN_DEVICE_ID}_web",
                "Type": "_http._tcp",
                "Port": "${env.HYDROGEN_WEB_PORT}"
            }
        ]
    }
}
```

## Environment Variable Support

You can use environment variables for any setting:

```json
{
    "mDNSServer": {
        "DeviceId": "${env.HYDROGEN_DEVICE_ID}",
        "FriendlyName": "${env.HYDROGEN_FRIENDLY_NAME}",
        "Model": "${env.HYDROGEN_MODEL}"
    }
}
```

Common environment variable configurations:

```bash
# Development
export HYDROGEN_DEVICE_ID="hydrogen-dev"
export HYDROGEN_FRIENDLY_NAME="Development Printer"

# Production
export HYDROGEN_DEVICE_ID="hydrogen-prod"
export HYDROGEN_FRIENDLY_NAME="Production Printer"
```

## Network Considerations

1. **Multicast Traffic**:
   - Ensure multicast is enabled on your network
   - Check firewall rules for mDNS traffic (UDP port 5353)
   - Consider network segmentation impact

2. **IPv6 Support**:
   - Enable `EnableIPv6` only if network supports it
   - Test discovery on both IPv4 and IPv6
   - Consider dual-stack implications

3. **Port Configuration**:
   - Ensure advertised ports match actual service ports
   - Check port availability
   - Consider firewall rules

## Security Considerations

1. **Information Disclosure**:
   - Consider what information to expose in TXT records
   - Use generic names in public environments
   - Monitor for unauthorized service registration

2. **Network Access**:
   - Control mDNS traffic at network boundaries
   - Monitor for excessive queries
   - Consider disabling in sensitive environments

3. **Service Advertisement**:
   - Only advertise necessary services
   - Use clear, but secure naming conventions
   - Regular audit of advertised services

## Troubleshooting

### Common Issues

1. **Discovery Problems**:
   - Check if mDNS is enabled
   - Verify multicast network settings
   - Test with standard mDNS tools

2. **Service Issues**:
   - Verify service ports are correct
   - Check TXT record format
   - Ensure services are running

3. **Network Problems**:
   - Check firewall settings
   - Verify network supports multicast
   - Test IPv6 if enabled

### Diagnostic Steps

#### Enable debug logging

```json
{
    "mDNSServer": {
        "LogLevel": "DEBUG"
    }
}
```

#### Test service discovery

```bash
# Linux/macOS
avahi-browse -ar

# Windows
dns-sd -B _http._tcp
```

#### Verify service registration

```bash
dns-sd -L "Hydrogen_Web_Interface" _http._tcp local.
```

## Related Documentation

- [Network Configuration](/docs/reference/network_configuration.md) - General network settings
- [Web Server Configuration](webserver_configuration.md) - HTTP server settings
- [WebSocket Configuration](websocket_configuration.md) - WebSocket settings
- [Logging Configuration](logging_configuration.md) - Logging setup
