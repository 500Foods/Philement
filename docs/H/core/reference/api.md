# Hydrogen API Documentation

This document provides details about the available API endpoints in the Hydrogen Project server.

## System Endpoints

### Get System Information

Retrieves comprehensive system status and information about the Hydrogen server, including version information, system details, resource usage, service status, and active file descriptors.

**Endpoint:** `/api/system/info`  
**Method:** GET

[Detailed Documentation](./system_info.md)

### Health Check

Simple health check endpoint used by load balancers to verify service availability in distributed deployments.

```bash
curl http://localhost:5000/api/system/health
```

**Endpoint:** `/api/system/health`  
**Method:** GET  
**Content-Type:** application/json

Response example:

```json
{
    "status": "Yes, I'm alive, thanks!"
}
```

### Version Information

Retrieves the current version information for the API and server.

```bash
curl http://localhost:5000/api/version
```

**Endpoint:** `/api/version`  
**Method:** GET  
**Content-Type:** application/json

Response example:

```json
{
    "api": "0.1",
    "server": "1.1.0",
    "text": "OctoPrint 1.1.0"
}
```

## Print Queue Management

### Get Queue Status

Retrieves the current state of the print queue, including active and pending jobs.

```bash
curl http://localhost:5000/api/queue/status
```

**Endpoint:** `/api/queue/status`  
**Method:** GET  
**Content-Type:** application/json

Response example:

```json
{
    "active_jobs": 1,
    "pending_jobs": 2,
    "queue_length": 3,
    "current_job": {
        "id": "job_123",
        "filename": "part.gcode",
        "progress": 45.2,
        "time_remaining": 1800,
        "started_at": "2025-02-21T15:30:00Z"
    },
    "pending_jobs": [
        {
            "id": "job_124",
            "filename": "next_part.gcode",
            "estimated_time": 3600
        }
    ]
}
```

### Add Print Job

Adds a new print job to the queue.

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"filename": "part.gcode", "priority": 1}' \
     http://localhost:5000/api/queue/add
```

**Endpoint:** `/api/queue/add`  
**Method:** POST  
**Content-Type:** application/json

Request body:

```json
{
    "filename": "part.gcode",
    "priority": 1,
    "material": "PLA",
    "notify_on_complete": true
}
```

### Modify Queue

Updates the print queue order or job settings.

```bash
curl -X PUT -H "Content-Type: application/json" \
     -d '{"job_id": "job_124", "position": 1}' \
     http://localhost:5000/api/queue/modify
```

**Endpoint:** `/api/queue/modify`  
**Method:** PUT  
**Content-Type:** application/json

Request body:

```json
{
    "job_id": "job_124",
    "position": 1,
    "priority": 2,
    "notify_on_complete": true
}
```

### Remove Print Job

Removes a job from the print queue.

```bash
curl -X DELETE http://localhost:5000/api/queue/remove/job_124
```

**Endpoint:** `/api/queue/remove/{job_id}`  
**Method:** DELETE  
**Content-Type:** application/json

### Pause Queue

Temporarily stops processing new jobs from the queue.

```bash
curl -X POST http://localhost:5000/api/queue/pause
```

**Endpoint:** `/api/queue/pause`  
**Method:** POST  
**Content-Type:** application/json

### Resume Queue

Resumes processing jobs from the queue.

```bash
curl -X POST http://localhost:5000/api/queue/resume
```

**Endpoint:** `/api/queue/resume`  
**Method:** POST  
**Content-Type:** application/json

### Clear Queue

Removes all pending jobs from the queue.

```bash
curl -X DELETE http://localhost:5000/api/queue/clear
```

**Endpoint:** `/api/queue/clear`  
**Method:** DELETE  
**Content-Type:** application/json

## Additional Resources

- [Configuration Guide](./configuration.md) - Complete guide for server configuration
- [WebSocket API](./web_socket.md) - Real-time communication interface
- [Print Queue Management](../print_queue.md) - Detailed queue management guide

## Contributing to Documentation

When adding new API endpoints, please follow this format:

```markdown
### Endpoint Name

Brief description of what the endpoint does.

\```bash
curl example command
\```

**Endpoint:** `/api/path/to/endpoint`  
**Method:** GET/POST/etc  
**Content-Type:** content type

Optional additional details about:
- Request parameters
- Response format
- Examples
