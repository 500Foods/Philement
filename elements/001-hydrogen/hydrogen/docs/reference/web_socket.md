# WebSocket Interface

This document describes the WebSocket interface provided by the Hydrogen server for real-time status updates and interactions.

## Connection Details

- **Port:** 5001 (default, see [Configuration Guide](./configuration.md) for details)
- **Protocol:** hydrogen-protocol
- **Authorization:** Required, use `Authorization: Key abc` in header

For complete configuration options including IPv6 support and logging settings, see the [Configuration Guide](./configuration.md#websocket).

## Status Updates

The WebSocket server provides real-time status information including:

- Server uptime
- Active connections
- Total connections
- Request metrics

### Example Connection

```javascript
const ws = new WebSocket('ws://localhost:5001', 'hydrogen-protocol');
ws.onopen = () => {
    console.log('Connected to Hydrogen WebSocket server');
};
ws.onmessage = (event) => {
    const status = JSON.parse(event.data);
    console.log('Received status update:', status);
};
```

## Message Format

Status messages are sent as JSON objects with the following structure:

```json
{
    "server_start_time": "ISO8601 timestamp",
    "active_connections": number,
    "total_connections": number,
    "total_requests": number,
    "system": {
        "sysname": "string",
        "nodename": "string",
        "release": "string",
        "version": "string",
        "machine": "string"
    },
    "status": {
        "running": boolean,
        "shutting_down": boolean,
        "totalThreads": number,
        "totalVirtualMemoryBytes": number,
        "totalResidentMemoryBytes": number
    }
}
```

### Message Fields

- **server_start_time**: ISO8601 formatted timestamp of server start
- **active_connections**: Current number of WebSocket connections
- **total_connections**: Total connections since server start
- **total_requests**: Total WebSocket requests processed
- **system**: Operating system and hardware information
- **status**: Current server status and resource usage

### Error Messages

Error responses follow this format:

```json
{
    "error": {
        "code": "string",
        "message": "string",
        "details": object
    }
}
```

Common error codes:

- `auth_failed`: Authentication failure
- `invalid_message`: Malformed message
- `server_error`: Internal server error

## Security

The WebSocket server requires authentication using an API key. Include the key in the connection headers:

```javascript
const ws = new WebSocket('ws://localhost:5001', 'hydrogen-protocol', {
    headers: {
        'Authorization': 'Key abc'
    }
});
```

## Additional Features

More WebSocket features will be documented here as they are implemented, including:

- Print job status updates
- Temperature monitoring
- Command interface
- Error notifications

## Contributing

When adding new WebSocket features:

1. Document the message format
2. Include connection examples
3. Update security requirements if changed
4. Add error handling information
