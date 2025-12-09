# System Service Endpoints

This document details the system-related endpoints that provide information about the Hydrogen server's status and health.

## System Information Endpoint

The `/api/system/info` endpoint provides comprehensive information about the Hydrogen server's status, including version information, system details, resource usage, and service status.

## Health Check Endpoint

The `/api/system/health` endpoint provides a simple health check mechanism used primarily by load balancers in distributed deployments to verify service availability.

**URL:** `/api/system/health`  
**Method:** GET  
**Content-Type:** application/json

### Example Request 1

```bash
curl http://localhost:5000/api/system/health
```

### Example Response 1

```json
{
    "status": "Yes, I'm alive, thanks!"
}
```

### Use Cases

1. **Load Balancer Health Checks**
   - Used by Apache and other load balancers to verify service availability
   - Supports round-robin deployment strategies
   - Quick response time for efficient health monitoring

2. **Service Monitoring**
   - Simple endpoint for monitoring tools
   - Minimal overhead for frequent polling
   - Clear status indication

3. **Deployment Verification**
   - Quick verification of service deployment
   - Simple integration into deployment scripts
   - Immediate feedback on service availability

## Endpoint Details

**URL:** `/api/system/info`  
**Method:** GET  
**Content-Type:** application/json

## Example Request 2

```bash
curl http://localhost:5000/api/system/info
```

## Example Response 2

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
    "running": true,
    "shutting_down": false,
    "totalThreads": 7,
    "totalVirtualMemoryBytes": 483663872,
    "totalResidentMemoryBytes": 17154048,
    "resources": {
      "serviceResources": {
        "threads": 6,
        "virtualMemoryBytes": 696320,
        "residentMemoryBytes": 696320,
        "allocationPercent": "4.059"
      },
      "queueResources": {
        "entries": 0,
        "virtualMemoryBytes": 0,
        "residentMemoryBytes": 0,
        "allocationPercent": "0.000"
      },
      "otherResources": {
        "threads": 1,
        "virtualMemoryBytes": 482967552,
        "residentMemoryBytes": 16457728,
        "allocationPercent": "95.941"
      }
    }
  }
}
```

## Response Fields Explanation

### Version Information

- **version**: Contains server version details
  - `server`: The version of the Hydrogen server software
  - `api`: The version of the API specification

### System Information

- **system**: Operating system details
  - `sysname`: Name of the operating system
  - `nodename`: Network node hostname
  - `release`: Operating system release version
  - `version`: Detailed OS version information
  - `machine`: Hardware architecture type

### Server Status

- **status**: Current server status and resource usage
  - `running`: Boolean indicating if server is operational
  - `shutting_down`: Boolean indicating if server is shutting down
  - `totalThreads`: Total number of active threads
  - `totalVirtualMemoryBytes`: Total virtual memory allocated
  - `totalResidentMemoryBytes`: Total physical memory in use
  - `resources`: Breakdown of resource usage by component

## Advanced Monitoring

For more detailed system monitoring, including real-time thread information, memory analysis, and file descriptor tracking, see the [Thread Monitoring Documentation](/docs/H/core/reference/thread_monitoring.md). This documentation includes a bash script that provides deeper insights into the Hydrogen server's runtime behavior, complementing the information available through this API endpoint.
