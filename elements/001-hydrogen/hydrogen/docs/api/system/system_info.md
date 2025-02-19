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
- `system`: Operating system details
- `resources`: Current resource usage
- `services`: Status of running services
- `descriptors`: Active file descriptor information

### Example Response

```json
{
    "version": {
        "api": "0.1",
        "server": "1.1.0"
    },
    "system": {
        "os": "Linux",
        "version": "5.10.0",
        "architecture": "x86_64"
    },
    "resources": {
        "memory": {
            "total": 8589934592,
            "used": 4294967296,
            "free": 4294967296
        },
        "cpu": {
            "cores": 4,
            "usage": 25.5
        }
    },
    "services": {
        "print_queue": "running",
        "websocket": "running",
        "mdns": "running"
    },
    "descriptors": {
        "open": 24,
        "max": 1024
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
- CPU usage is a percentage across all cores
- Service status is updated every 5 seconds