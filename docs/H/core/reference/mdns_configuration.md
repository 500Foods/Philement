# mDNS Configuration Guide

## Overview

The mDNS subsystem provides automatic discovery of services on local networks. It has two halves:

- **mDNS Server** (`mDNSServer`): advertises Hydrogen's services and answers queries.
- **mDNS Client** (`mDNSClient`): browses configured service types, resolves and caches discovered services, and runs optional TCP health checks.

Both are configured through `hydrogen.json`.

## mDNS Server — `mDNSServer`

### Core Settings

| Setting | Description | Default | Notes |
| --------- | ------------- | --------- | ------- |
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
| --------- | ------------- | --------- | ------- |
| `Name` | Service instance name | - | Required |
| `Type` | Service type | - | Required |
| `Port` | Service port | - | Required |
| `TxtRecords` | Additional information | - | Optional |

The server probes for each instance name and the hostname before announcing. On conflict it renames (`Name (2)`, `host-2.local`) up to 8 times, then fails to launch.

### Example

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
        "TxtRecords": ["path=/api", "version=1.0.0"]
      },
      {
        "Name": "Hydrogen_WebSocket",
        "Type": "_websocket._tcp",
        "Port": 5001,
        "TxtRecords": ["path=/websocket"]
      }
    ]
  }
}
```

## mDNS Client — `mDNSClient`

### Core Client Settings

| Setting | Description | Default | Notes |
| --------- | ------------- | --------- | ------- |
| `Enabled` | Turns mDNS client on/off | `true` | Required |
| `EnableIPv6` | Enables IPv6 support | `false` | Optional |
| `ScanIntervalMs` | Browse interval | `5000` | Milliseconds |
| `MaxServices` | Maximum cached services | `100` | Cap |
| `RetryCount` | Query retry count | `3` | Optional |

### Service Types

`ServiceTypes` accepts either an array of strings or an array of objects:

```json
{
  "ServiceTypes": [
    "_http._tcp",
    { "Type": "_octoprint._tcp", "Required": true, "AutoConnect": false }
  ]
}
```

When an object is used, `Required` and `AutoConnect` are stored; `AutoConnect` is not acted upon (no HTTP client).

### Monitored Services

```json
{
  "MonitoredServices": {
    "OwnServices": true,
    "PrinterServices": true,
    "CustomServices": ["_custom._tcp"]
  }
}
```

- `OwnServices=false`: skip instances the local server claimed.
- `PrinterServices=true`: limit extra types to `_http._tcp`, `_octoprint._tcp`, `_hydrogen._tcp`, `_ipp._tcp`, `_printer._tcp`.
- `CustomServices`: appended to the browse list.
- `LoadBalancers`: parsed and ignored (logged once) until a later plan defines a service type.

### Health Check

```json
{
  "HealthCheck": {
    "Enabled": true,
    "IntervalMs": 30000,
    "TimeoutMs": 5000,
    "RetryCount": 3
  }
}
```

All intervals are **milliseconds**. Health checking is a TCP connect to the advertised host:port; there is no HTTP GET.

### Client Example

```json
{
  "mDNSClient": {
    "Enabled": true,
    "EnableIPv6": true,
    "ScanIntervalMs": 5000,
    "MaxServices": 100,
    "RetryCount": 3,
    "ServiceTypes": [
      "_http._tcp",
      "_octoprint._tcp",
      "_hydrogen._tcp"
    ],
    "MonitoredServices": {
      "OwnServices": true,
      "PrinterServices": true,
      "CustomServices": []
    },
    "HealthCheck": {
      "Enabled": true,
      "IntervalMs": 30000,
      "TimeoutMs": 5000,
      "RetryCount": 3
    }
  }
}
```

## Environment Variables

You can use environment variables for any setting:

```json
{
  "mDNSServer": {
    "DeviceId": "${env.HYDROGEN_DEVICE_ID}",
    "FriendlyName": "${env.HYDROGEN_FRIENDLY_NAME}"
  }
}
```

## Network Considerations

1. **Multicast Traffic**: ensure multicast is enabled; allow UDP port 5353.
2. **IPv6**: enable only if the network supports it.
3. **Port Configuration**: advertised ports must match actual service ports.

## Troubleshooting

1. **Discovery Problems**: check `Enabled`, verify multicast, test with `avahi-browse -ar` (Linux) or `dns-sd -B _http._tcp` (macOS).
2. **Service Issues**: verify ports and TXT record format.
3. **No mDNS Activity**: check the `mDNSServer` / `mDNSClient` subsystem logs.

## Related Documentation

- [mDNS Server](/docs/H/core/subsystems/mdnsserver/mdnsserver.md)
- [mDNS Client](/docs/H/core/subsystems/mdnsclient/mdnsclient.md)
- [mDNS Client Architecture](/docs/H/core/reference/mdns_client_architecture.md)
