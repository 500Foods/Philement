# mDNS Server

This document describes the multicast DNS (mDNS) service discovery system implemented in the Hydrogen server, which allows printers to be automatically discovered on local networks.

## Overview

The mDNS Server module enables automatic service discovery, eliminating the need for manual IP address configuration in most network environments. It implements the DNS-SD protocol (RFC 6762 and RFC 6763) for advertising the following:

- Printer availability
- Service endpoints (web, WebSocket)
- Printer capabilities
- Network location information

## Configuration

The mDNS Server is configured through the `mdns_server` section of the configuration file:

```json
{
  "mdns_server": {
    "enabled": true,
    "EnableIPv6": true,
    "device_id": "hydrogen_01",
    "friendly_name": "My Hydrogen Printer",
    "model": "Voron 2.4",
    "manufacturer": "Self-built",
    "version": "1.0",
    "services": [
      {
        "name": "hydrogen-printer",
        "type": "_http._tcp.local",
        "port": 5000,
        "txt_records": [
          "version=1.0",
          "api=/api",
          "path=/",
          "type=printer"
        ]
      },
      {
        "name": "hydrogen-ws",
        "type": "_websocket._tcp.local",
        "port": 5001,
        "txt_records": [
          "version=1.0",
          "protocol=hydrogen-protocol"
        ]
      }
    ]
  }
}
```

For complete configuration options, see the [Configuration Guide](./configuration.md#mdns-server).

## Service Advertisement

The mDNS Server advertises services using the following DNS record types:

- **A/AAAA records**: Advertise IPv4/IPv6 addresses
- **PTR records**: Map service types to specific instances
- **SRV records**: Provide host and port information
- **TXT records**: Include additional service metadata

When the server starts, it sends initial announcements with a brief delay between them, then periodically re-broadcasts service information according to the mDNS specification.

### Advertisement Format

Services are advertised using the following format:

```service
[instance name].[service type]
```

For example: `hydrogen-printer._http._tcp.local`

## Network Interface Management

The mDNS Server automatically:

1. Detects all available network interfaces
2. Excludes loopback interfaces
3. Sets up multicast sockets for both IPv4 and IPv6 (if enabled)
4. Monitors interface-specific IP addresses
5. Manages interface-specific announcements

This ensures services are properly advertised on all relevant network interfaces, even in complex multi-network environments.

## Implementation Details

The mDNS Server uses a modular architecture designed for reliability, efficiency, and standards compliance:

### Component Architecture

The implementation consists of several key components:

- **Socket Management**:
  - Dual-stack IPv4/IPv6 support
  - Interface-specific binding
  - Multicast group management

- **Packet Construction**:
  - DNS record creation (A, AAAA, PTR, SRV, TXT)
  - Name compression
  - TTL management

- **Service Announcement**:
  - Periodic broadcasts
  - Goodbye packets during shutdown
  - Interface-specific announcements

- **Query Handling**:
  - Question parsing
  - Targeted responses
  - Request filtering

### Thread Management

The mDNS Server operates two primary threads:

1. **Announcement Thread (`mdns_server_announce_loop`)**:
   - Broadcasts service information at regular intervals
   - Sends initial announcements at server startup
   - Handles graceful service withdrawal at shutdown

2. **Responder Thread (`mdns_server_responder_loop`)**:
   - Listens for incoming mDNS queries
   - Filters relevant requests
   - Generates appropriate responses

Both threads register with the service thread tracking system for proper monitoring and cleanup.

### Shutdown Sequence

The mDNS Server implements a robust, standards-compliant shutdown process:

1. **Signaling Phase**
   - Set `mdns_server_system_shutdown = 1` flag
   - Broadcast on condition variable to wake waiting threads
   - Allow threads to detect shutdown state and begin cleanup

2. **Service Withdrawal**
   - Send RFC-compliant "goodbye" packets (TTL=0) for all services
   - Send on all network interfaces (IPv4 and IPv6)
   - Repeat 3 times with 250ms delay for reliability
   - Explicitly log each packet for verification

3. **Thread Coordination**
   - Join the announcement and responder threads
   - Verify thread exit with progressive timeouts
   - Use thread tracking to ensure all mDNS threads exit

4. **Socket Cleanup**
   - Explicitly close all sockets on each interface
   - Close IPv4 and IPv6 sockets separately
   - Verify socket closure

5. **Resource Cleanup**
   - Free interface structures in correct order
   - Release all dynamically allocated memory
   - Brief delay before final cleanup to prevent race conditions

This approach ensures proper service withdrawal from the network and clean resource deallocation, even under error conditions.

## Debugging and Monitoring

To verify proper mDNS Server operation, you can:

1. Use the `/api/system/info` endpoint to check mDNS Server status
2. View mDNS-specific log messages with the "mDNSServer" subsystem tag
3. Use external tools to verify service announcement:
   - `avahi-browse -a` on Linux
   - `dns-sd -B _http._tcp` on macOS
   - Bonjour Browser on Windows

## Security Considerations

The mDNS Server follows these security practices:

1. **Access Control**: mDNS is limited to local networks by design
2. **Minimal Information**: Only advertises necessary service information
3. **DoS Protection**: Ignores malformed queries
4. **Input Validation**: Sanitizes all incoming data

## Troubleshooting

Common mDNS Server issues and solutions:

1. **Service Not Visible**
   - Check if mDNS Server is enabled in configuration
   - Ensure multicast is allowed on your network
   - Verify no firewall is blocking UDP port 5353

2. **Port Conflicts**
   - Check if another mDNS service is running (Avahi, Bonjour)
   - If necessary, stop conflicting services

3. **Performance Issues**
   - Limit the number of advertised services
   - Tune announcement intervals for your network

## References

- [Shutdown Architecture](./shutdown_architecture.md) - Complete shutdown sequence overview
- [WebSocket Interface](./web_socket.md) - WebSocket server implementation
- [Configuration Guide](./reference/configuration.md) - Configuration options
- [Thread Monitoring](./thread_monitoring.md) - Thread tracking and diagnostics
- [Release Notes](../RELEASES.md) - History of mDNS server improvements
