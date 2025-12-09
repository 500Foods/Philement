# Health Check Endpoint

Simple health check endpoint used by load balancers to verify service availability.

## Overview

This endpoint provides a basic health check mechanism that can be used by load balancers, monitoring systems, or other infrastructure components to verify that the Hydrogen server is operational and responding to requests.

## Request

**Endpoint:** `/api/system/health`  
**Method:** GET  
**Content-Type:** application/json

Example:

```bash
curl http://localhost:5000/api/system/health
```

## Response

A successful response indicates that the server is operational.

### Success Response

**Status Code:** 200 OK  
**Content-Type:** application/json

```json
{
    "status": "Yes, I'm alive, thanks!"
}
```

## Error Responses

The endpoint will return non-200 status codes if the server is experiencing issues.

| Status Code | Description |
|------------|-------------|
| 503 | Service Unavailable - Server is shutting down or in maintenance mode |
| 500 | Internal Server Error - Unexpected error occurred |

### Error Response Example

```json
{
    "error": {
        "code": "service_unavailable",
        "message": "Server is shutting down",
        "details": {
            "maintenance_mode": true,
            "estimated_duration": "5m"
        }
    }
}
```

## Notes

- This endpoint is lightweight and designed for frequent polling
- No authentication is required
- Response time is monitored and logged
- Recommended polling interval: 30 seconds
