# Test 25: mDNS Server and Client Script Documentation

## Overview

The `test_25_mdns.sh` script is a comprehensive mDNS (multicast DNS) service discovery testing tool within the Hydrogen test suite. It validates both the mDNS server announcement functionality and the mDNS client discovery capabilities, ensuring proper local network service discovery.

## Purpose

This script tests the Hydrogen server's mDNS implementation, which enables zero-configuration networking and automatic service discovery on local networks. It validates:

- mDNS server service announcements
- mDNS client service discovery
- External mDNS discovery tool compatibility
- Log output verification for both server and client components
- Network service accessibility and proper mDNS protocol usage

## Script Details

- **Script Name**: `test_25_mdns.sh`
- **Test Name**: mDNS Server and Client
- **Version**: 1.0.0
- **Dependencies**: Uses modular libraries from `lib/` directory
- **Hardware/Network Requirements**: Requires multicast UDP support on network interfaces

## Key Features

### Core Functionality

- **Server Announcement Testing**: Validates mDNS server properly advertises configured services
- **Client Discovery Testing**: Validates mDNS client can discover services on the network
- **External Tool Compatibility**: Tests with avahi-browse (Linux) and dns-sd (macOS) tools
- **Log Analysis**: Monitors both mDNS server and client log output for proper operation
- **Network Interface Validation**: Ensures mDNS works across available network interfaces

### mDNS Services Tested

- **HTTP Service**: `_http._tcp` service advertisement and discovery
- **WebSocket Service**: `_websocket._tcp` service advertisement and discovery
- **Hydrogen Custom Service**: `_hydrogen._tcp` service advertisement and discovery
- **Custom Test Services**: `_test._tcp` service types configured for testing

### Advanced Testing

- **Configuration File Validation**: Verifies mDNS-specific configuration exists
- **Port Management**: Extracts and manages assigned web server and WebSocket ports
- **Service Initialization Delay**: Allows time for mDNS services to initialize and announce
- **Cross-Platform Compatibility**: Works with different mDNS implementations (Avahi, Bonjour)
- **Graceful Degradation**: Continues testing if external tools are not available

## Technical Implementation

### Library Dependencies

- `framework.sh` - Test framework and result tracking
- `log_output.sh` - Formatted output and logging
- `lifecycle.sh` - Process lifecycle management (server start/stop)
- `file_utils.sh` - File operations and path management
- `network_utils.sh` - Network utilities including wait_for_server_ready

### Configuration Files

- **mDNS Test Configuration**: `hydrogen_test_25_mdns.json` (enables both mDNS server and client)
- **Service Announcements**: HTTP, WebSocket, and custom Hydrogen services
- **Client Configuration**: Service type filtering and health check settings

### Test Sequence (8 Subtests)

1. **Binary Location**: Locates Hydrogen binary
2. **Configuration Validation**: Verifies mDNS test configuration file exists
3. **Server Startup**: Starts Hydrogen with mDNS configuration
4. **Server Readiness**: Waits for server to be ready and mDNS services to initialize
5. **mDNS Server Logging**: Tests mDNS server log output and activity
6. **mDNS Client Logging**: Tests mDNS client log output and activity
7. **External Tool Testing**: Tests with avahi-browse/dns-sd if available
8. **Server Shutdown**: Stops server and verifies clean shutdown

### Validation Criteria

- **Server Announcements**: Must detect mDNS server announcement activities in logs
- **Client Discovery**: Must detect mDNS client query/discovery activities in logs
- **External Verification**: avahi-browse or dns-sd must detect announced services
- **Service Types**: Must verify specific service types (_http._tcp, _websocket._tcp, _hydrogen._tcp)
- **Log Activity**: Must find relevant mDNS server and client log entries

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_25_mdns.sh
```

## Expected Results

### Successful Test Criteria

- Hydrogen binary located successfully
- mDNS configuration file found and validated
- Server starts with mDNS configuration and becomes ready
- mDNS server log shows announcement activities for configured services
- mDNS client log shows discovery/query activities
- External mDNS tools can detect Hydrogen services (when available)
- Server shuts down cleanly with proper mDNS service withdrawal

### Key Validation Points

- **Service Discovery**: Zero-configuration networking works correctly
- **Log Monitoring**: Both server and client show expected mDNS activities
- **External Compatibility**: Services are discoverable by standard mDNS browsers
- **Network Configuration**: mDNS works with current network interface configuration
- **Protocol Compliance**: Services follow mDNS standards and naming conventions

## Troubleshooting

### Common Issues

- **No mDNS Activity**: Check if mDNS is enabled in configuration
- **External Tools Not Found**: avahi-browse (Linux) or dns-sd (macOS) not installed
- **Service Not Discovered**: Firewall may block multicast UDP port 5353
- **Network Issues**: Multicast may be disabled on network interface
- **Log Activity Missing**: mDNS components may not be enabled in configuration

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Server Logs**: `tests/logs/test_25_mdns_[timestamp]_server.log`
- **Diagnostic Files**: Available in `tests/diagnostics/test_25_mdns_[timestamp]/`
- **External Tool Output**: Captures avahi-browse or dns-sd command output
- **Log Analysis**: Detailed analysis of mDNS server and client log entries

## Configuration Requirements

### mDNS Test Configuration (`hydrogen_test_25_mdns.json`)

```json
{
  "mDNSServer": {
    "Enabled": true,
    "EnableIPv6": true,
    "DeviceId": "hydrogen-mdns-test",
    "FriendlyName": "Hydrogen mDNS Test Server",
    "Services": [
      {
        "Name": "Hydrogen_Test_Web_Interface",
        "Type": "_http._tcp",
        "Port": 5225,
        "TxtRecords": ["test=true", "version=25.0.0-test"]
      }
    ]
  },
  "mDNSClient": {
    "Enabled": true,
    "EnableIPv6": true,
    "ServiceTypes": ["_http._tcp", "_websocket._tcp", "_hydrogen._tcp"],
    "HealthCheck": {
      "Enabled": true
    }
  }
}
```

### Network Requirements

- **Multicast Support**: Network interface must support IP multicast
- **UDP Port 5353**: Must be available for mDNS traffic
- **IPv4/IPv6**: Depending on configuration, multicast groups 224.0.0.251 / FF02::FB
- **Local Network**: mDNS works on local subnet only

## mDNS Components Validated

- **mDNS Server**:
  - Service announcement initialization
  - Periodic re-announcements
  - Service withdrawal on shutdown
  - Multi-interface support

- **mDNS Client**:
  - Service query and discovery
  - Service caching and monitoring
  - Health check functionality
  - Service type filtering

- **External Tools**:
  - avahi-browse compatibility
  - dns-sd compatibility
  - Service discovery verification

## Version History

- **1.0.0** (2025-08-28): Initial creation for Test 25 - mDNS

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [mdns_server.md](../../docs/mdns_server.md) - mDNS server implementation details
- [configuration.md](../../docs/configuration.md) - Configuration file documentation
- [testing.md](../../docs/testing.md) - Overall testing strategy
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation

## mDNS Standards and Protocols

- **RFC 6762**: Multicast DNS specification
- **RFC 6763**: DNS-Based Service Discovery
- **Network Discovery**: Zero-configuration networking paradigm
- **Service Types**: Standard service type naming conventions
