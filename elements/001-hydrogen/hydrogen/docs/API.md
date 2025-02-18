# Hydrogen API Documentation

This document provides details about the available API endpoints in the Hydrogen Project server.

## System Endpoints

### Get System Information

Retrieves comprehensive system status and information about the Hydrogen server, including version information, system details, resource usage, service status, and active file descriptors.

**Endpoint:** `/api/system/info`  
**Method:** GET

[Detailed Documentation](./system_info.md)

## Additional Resources

- [Configuration Guide](./Configuration.md) - Complete guide for server configuration
- [WebSocket API](./WebSocket.md) - Real-time communication interface
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
