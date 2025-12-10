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
    },
    "files": [
      {
        "fd": -1071001664,
        "type": "stdio",
        "description": "stdin: terminal"
      },
      {
        "fd": -1071001664,
        "type": "stdio",
        "description": "stdout: terminal"
      },
      {
        "fd": -1071001664,
        "type": "stdio",
        "description": "stderr: terminal"
      },
      {
        "fd": -1071001664,
        "type": "file",
        "description": "file: /var/log/hydrogen.log"
      },
      {
        "fd": -1071001664,
        "type": "anon_inode",
        "description": "event notification channel"
      },
      {
        "fd": 924852256,
        "type": "socket",
        "description": "socket (tcp port 5000 - web server)"
      },
      {
        "fd": 924852256,
        "type": "device",
        "description": "random number source"
      },
      {
        "fd": 924852256,
        "type": "anon_inode",
        "description": "event notification channel"
      },
      {
        "fd": 859320369,
        "type": "socket",
        "description": "socket (inode: 95115985)"
      },
      {
        "fd": 958406688,
        "type": "socket",
        "description": "socket (tcp port 5002 - websocket server)"
      },
      {
        "fd": 891297824,
        "type": "socket",
        "description": "socket (tcp6 port 5002 - websocket server)"
      },
      {
        "fd": 808924704,
        "type": "socket",
        "description": "socket (udp port 5353 - mDNS)"
      },
      {
        "fd": 959787552,
        "type": "socket",
        "description": "socket (tcp port 5000 - web server)"
      },
      {
        "fd": 959787552,
        "type": "other",
        "description": "/proc/1265598/fd"
      },
      {
        "fd": 859320369,
        "type": "socket",
        "description": "Unix domain socket: roc/net/udp6"
      }
    ]
  },
  "queues": {
    "log": {
      "entryCount": 0,
      "blockCount": 0,
      "totalAllocation": 0,
      "virtualMemoryBytes": 0,
      "residentMemoryBytes": 0
    },
    "print": {
      "entryCount": 0,
      "blockCount": 0,
      "totalAllocation": 0,
      "virtualMemoryBytes": 0,
      "residentMemoryBytes": 0
    }
  },
  "enabledServices": [
    "logging",
    "web",
    "websocket",
    "mdns",
    "print"
  ],
  "services": {
    "logging": {
      "enabled": true,
      "log_file": "/var/log/hydrogen.log",
      "status": {
        "messageCount": 0,
        "threads": 1,
        "virtualMemoryBytes": 139264,
        "residentMemoryBytes": 139264,
        "threadIds": [
          1265599
        ]
      }
    },
    "web": {
      "enabled": true,
      "port": 5000,
      "upload_path": "/api/upload",
      "max_upload_size": 2147483648,
      "log_level": "WARN",
      "status": {
        "activeRequests": 0,
        "totalRequests": 0,
        "threads": 1,
        "virtualMemoryBytes": 139264,
        "residentMemoryBytes": 139264,
        "threadIds": [
          1265753
        ]
      }
    },
    "websocket": {
      "enabled": true,
      "port": 5001,
      "protocol": "hydrogen-protocol",
      "max_message_size": 10485760,
      "log_level": "NONE",
      "status": {
        "threads": 1,
        "uptime": 10,
        "activeConnections": 0,
        "totalConnections": 0,
        "totalRequests": 0,
        "virtualMemoryBytes": 139264,
        "residentMemoryBytes": 139264,
        "threadIds": [
          1265603
        ]
      }
    },
    "mdns": {
      "enabled": true,
      "device_id": "hydrogen-printer",
      "friendly_name": "Hydrogen 3D Printer",
      "model": "Hydrogen",
      "manufacturer": "Philement",
      "log_level": "ALL",
      "status": {
        "discoveryCount": 0,
        "threads": 1,
        "virtualMemoryBytes": 139264,
        "residentMemoryBytes": 139264,
        "threadIds": [
          1265604
        ]
      }
    },
    "print": {
      "enabled": true,
      "log_level": "WARN",
      "status": {
        "queuedJobs": 0,
        "completedJobs": 0,
        "threads": 1,
        "virtualMemoryBytes": 139264,
        "residentMemoryBytes": 139264,
        "threadIds": [
          1265600
        ]
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
  - `resources`: Breakdown of resource usage
    - `serviceResources`: Resources used by services
      - `threads`: Number of threads used by services
      - `virtualMemoryBytes`: Virtual memory used by services
      - `residentMemoryBytes`: Physical memory used by services
      - `allocationPercent`: Percentage of total resources allocated to services
    - `queueResources`: Resources used by queues
      - `entries`: Number of entries in all queues
      - `virtualMemoryBytes`: Virtual memory used by queues
      - `residentMemoryBytes`: Physical memory used by queues
      - `allocationPercent`: Percentage of total resources allocated to queues
    - `otherResources`: Resources used by other components
      - `threads`: Number of threads used by other components
      - `virtualMemoryBytes`: Virtual memory used by other components
      - `residentMemoryBytes`: Physical memory used by other components
      - `allocationPercent`: Percentage of total resources allocated to other components
  - `files`: List of open file descriptors
    - `fd`: File descriptor number
    - `type`: Type of file descriptor (stdio, file, socket, etc.)
    - `description`: Human-readable description of the file descriptor

### Queue Information

- **queues**: Status of various queue systems
  - `log`: Logging queue statistics
  - `print`: Print job queue statistics
  Each queue includes:
    - `entryCount`: Number of entries in queue
    - `blockCount`: Number of memory blocks allocated
    - `totalAllocation`: Total memory allocated
    - `virtualMemoryBytes`: Virtual memory used
    - `residentMemoryBytes`: Physical memory used

### Services

- **enabledServices**: List of currently enabled service names

- **services**: Detailed status of each service
  Each service includes:
  - `enabled`: Whether the service is enabled
  - `status`: Service-specific status information
    - Common fields:
      - `threads`: Number of threads used
      - `virtualMemoryBytes`: Virtual memory used
      - `residentMemoryBytes`: Physical memory used
      - `threadIds`: List of thread IDs
    - Service-specific fields:
      - Logging service:
        - `messageCount`: Number of messages processed
      - Web service:
        - `activeRequests`: Current number of active requests
        - `totalRequests`: Total number of requests handled
      - WebSocket service:
        - `uptime`: Service uptime in seconds
        - `activeConnections`: Current number of active connections
        - `totalConnections`: Total number of connections handled
        - `totalRequests`: Total number of requests processed
      - mDNS service:
        - `discoveryCount`: Number of discovery requests handled
      - Print service:
        - `queuedJobs`: Number of jobs in queue
        - `completedJobs`: Number of completed jobs

## Advanced Monitoring

For more detailed system monitoring, including real-time thread information, memory analysis, and file descriptor tracking, see the [Thread Monitoring Documentation](/docs/H/core/thread_monitoring.md). This documentation includes a bash script that provides deeper insights into the Hydrogen server's runtime behavior, complementing the information available through this API endpoint.
