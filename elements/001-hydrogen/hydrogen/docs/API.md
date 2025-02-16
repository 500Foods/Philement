# Hydrogen API Documentation

This document provides details about the available API endpoints in the Hydrogen Project server.

## System Endpoints

### Get System Information

Retrieves system status and information about the Hydrogen server.

```bash
curl http://localhost:5000/api/system/info
```

**Endpoint:** `/api/system/info`  
**Method:** GET  
**Content-Type:** application/json

**Response Format:**

```json
{
  "system": {
    "status": "...",
    "uptime": "...",
    "version": "..."
  },
  // Additional system information fields
}
```

## Additional Resources

- [Configuration Guide](./Configuration.md) (TODO)
- [WebSocket API](./WebSocket.md) (TODO)
- [Print Queue Management](./PrintQueue.md) (TODO)

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
