# WebSocket Interface

This document describes the WebSocket interface provided by the Hydrogen server for real-time status updates and interactions.

## Connection Details

- **Port:** 5001 (default, configurable in hydrogen.json)
- **Protocol:** hydrogen-protocol
- **Authorization:** Required, use `Authorization: Key abc` in header

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
    "total_requests": number
}
```

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