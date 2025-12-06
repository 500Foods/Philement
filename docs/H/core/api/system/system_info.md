# System Information Endpoint

Retrieves comprehensive system status and information about the Hydrogen server.

## Overview

This endpoint provides detailed information about the server's current state, including version information, system details, resource usage, service status, and active file descriptors.

## Request

**Endpoint:** `/api/system/info`  
**Method:** GET  
**Content-Type:** application/json

Example:

```bash
curl http://localhost:5000/api/system/info
```

## Response

The response includes detailed system information in JSON format.

### Fields

- `version`: Server version information
  - `server`: Server software version
  - `api`: API version number
- `system`: Operating system details
  - `sysname`: Operating system name
  - `nodename`: Network node hostname
  - `release`: OS release version
  - `version`: OS version details
  - `machine`: Hardware architecture
- `status`: Server status information
  - `server_running`: Whether the server is currently running
  - `server_stopping`: Whether the server is in the process of stopping
  - `server_starting`: Whether the server is in startup phase
  - `server_started`: ISO-formatted UTC datetime when the server became ready
  - `server_runtime`: Number of seconds the server has been running
  - `server_runtime_formatted`: Human-readable uptime (e.g. "4d 1h 22m 0s")
  - `totalThreads`: Total number of active threads
  - `totalVirtualMemoryBytes`: Total virtual memory usage in bytes
  - `totalResidentMemoryBytes`: Total resident memory usage in bytes
  - `resources`: Detailed resource usage by component
- `queues`: Queue system status
- `enabledServices`: List of currently enabled services
- `services`: Detailed status of each service

### Example Response

```json
{
  "version": {
    "server": "0.1.0",
    "api": "1.0"
  },
  "system": {
    "sysname": "Linux",
    "nodename": "angrin.500foods.com",
    "release": "5.10.15-200.fc33.x86_64",
    "version": "#1 SMP Wed Feb 10 17:46:55 UTC 2021",
    "machine": "x86_64"
  },
  "status": {
    "server_running": true,
    "server_stopping": false,
    "server_starting": false,
    "server_started": "2025-02-19T09:03:42.000Z",
    "server_runtime": 22,
    "server_runtime_formatted": "0d 0h 0m 22s",
    "totalThreads": 7,
    "totalVirtualMemoryBytes": 483659776,
    "totalResidentMemoryBytes": 17301504,
    "resources": {
      "serviceResources": {
        "threads": 6,
        "virtualMemoryBytes": 696320,
        "residentMemoryBytes": 696320,
        "allocationPercent": "4.025"
      },
      "queueResources": {
        "entries": 0,
        "virtualMemoryBytes": 0,
        "residentMemoryBytes": 0,
        "allocationPercent": "0.000"
      },
      "otherResources": {
        "threads": 1,
        "virtualMemoryBytes": 482963456,
        "residentMemoryBytes": 16605184,
        "allocationPercent": "95.975"
      }
    }
  }
}
```

## Error Responses

| Status Code | Description |
|------------|-------------|
| 500 | Internal server error while gathering system information |
| 503 | One or more required services are unavailable |

### Error Response Example

```json
{
    "error": {
        "code": "service_unavailable",
        "message": "Unable to gather system metrics",
        "details": {
            "service": "metrics_collector",
            "state": "stopped"
        }
    }
}
```

## Notes

- Resource usage information is sampled at request time
- Memory values are in bytes
- All timestamps are in UTC and ISO 8601 format
- Server runtime is provided in both seconds and human-readable format
- Resource percentages are rounded to 3 decimal places
- Thread counts include the main server thread
- Service status is updated every 5 seconds
