# Test 36: WebSockets

**Version:** 1.0.0  
**Test Script:** [test_23_websockets.sh](/elements/001-hydrogen/hydrogen/tests/test_23_websockets.sh)  
**Test Type:** Integration Testing  
**Dependencies:** WebSocket server, libwebsockets, Network subsystem

## Overview

This test verifies the WebSocket server functionality in the Hydrogen system, ensuring proper initialization, connection handling, and protocol validation across different configurations.

## Test Objectives

- **Primary Goal:** Validate WebSocket server integration and functionality
- **Secondary Goals:**
  - Test configuration handling and defaults
  - Verify network protocol support (IPv4/IPv6)
  - Validate connection establishment and communication
  - Test immediate restart capability (SO_REUSEADDR)

## Test Configurations

### Configuration 1: Default WebSocket Settings

- **File:** `hydrogen_test_websocket_test_1.json`
- **WebSocket Port:** 5001
- **Protocol:** "hydrogen"
- **IPv6:** Enabled
- **Purpose:** Test standard WebSocket configuration

### Configuration 2: Custom WebSocket Settings  

- **File:** `hydrogen_test_websocket_test_2.json`
- **WebSocket Port:** 5002
- **Protocol:** "hydrogen-test"
- **IPv6:** Enabled
- **Purpose:** Test custom port and protocol configuration

## Test Scenarios

### Prerequisites (Subtests 1-3)

1. **Hydrogen Binary Detection**
   - Locates the Hydrogen executable
   - Validates binary exists and is accessible

2. **Configuration File Validation**
   - Validates test configuration files exist
   - Checks JSON syntax and required sections

### WebSocket Functionality Tests (Subtests 4-13)

#### Per Configuration Tests (5 subtests each)

1. **Server Startup**
   - Starts Hydrogen with WebSocket configuration
   - Validates successful server initialization
   - Confirms process ID assignment

2. **WebSocket Connection Testing**
   - Tests WebSocket connection establishment
   - Uses wscat for full protocol testing
   - Falls back to curl for basic HTTP upgrade testing
   - Validates WebSocket handshake (HTTP 101 response)

3. **Port Accessibility Validation**
   - Verifies WebSocket port is listening
   - Tests network accessibility using netstat
   - Fallback to direct TCP connection testing

4. **Protocol Validation**
   - Tests WebSocket subprotocol negotiation
   - Validates correct protocol acceptance
   - Ensures protocol validation is working

5. **Log Verification**
   - Confirms WebSocket initialization in server logs
   - Validates successful launch messages
   - Checks for proper subsystem registration

## Technical Requirements

### External Dependencies

- **wscat** (preferred): Full WebSocket client testing
- **curl**: Fallback for HTTP upgrade testing
- **netstat**: Port listening verification
- **timeout**: Connection timeout handling

### Network Requirements

- IPv4 and IPv6 interface availability
- Ports 5001 and 5002 available for testing
- SO_REUSEADDR support for immediate restart

### WebSocket Protocol Features

- HTTP/1.1 upgrade mechanism
- WebSocket handshake validation
- Subprotocol negotiation
- Connection lifecycle management

## Expected Outcomes

### Success Criteria

- **All 13 subtests pass** (3 prerequisites + 5×2 configurations)
- WebSocket server binds to configured ports
- Successful WebSocket upgrade (HTTP 101)
- Protocol validation working correctly
- Clean server startup and shutdown

### Performance Expectations

- **Server Startup:** < 15 seconds
- **WebSocket Handshake:** < 5 seconds
- **Port Binding:** Immediate with SO_REUSEADDR
- **Graceful Shutdown:** < 10 seconds

## Failure Analysis

### Common Failure Points

1. **WebSocket Library Issues**
   - Missing libwebsockets dependency
   - Version compatibility problems
   - SSL/TLS configuration issues

2. **Network Configuration**
   - Port conflicts or unavailability
   - Firewall blocking WebSocket ports
   - IPv6 configuration problems

3. **Protocol Issues**
   - Incorrect subprotocol configuration
   - WebSocket key validation failures
   - Message size limit violations

4. **Integration Problems**
   - Registry subsystem not initialized
   - Network subsystem dependencies not met
   - Threading issues in server startup

### Debugging Information

- **Server Logs:** Preserved in `results/` directory on failure
- **Connection Traces:** WebSocket handshake debugging
- **Network State:** Port binding and TIME_WAIT analysis
- **Process Status:** PID tracking and cleanup verification

## Integration with Test Suite

### Test Framework Integration

- Uses standard Hydrogen test library functions
- Integrated with `test_00_all.sh` for comprehensive testing
- Exports results for aggregate reporting
- Follows established test patterns and conventions

### Coverage Integration

- Blackbox coverage collection via hydrogen_coverage binary
- Integration test coverage for WebSocket subsystem
- Real-world usage pattern testing

## Related Documentation

- **Main Test Suite:** [tests/TESTING.md](/docs/H/tests/TESTING.md)
- **Project Overview:** [SITEMAP.md](/docs/H/SITEMAP.md)
- **WebSocket Documentation:** [docs/web_socket.md](/docs/H/core/web_socket.md)
- **Configuration Guide:** [docs/configuration.md](/docs/H/core/configuration.md)
