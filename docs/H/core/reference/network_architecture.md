# Network Interface Architecture

This document provides a comprehensive overview of the Network Interface abstraction layer in Hydrogen, detailing its architecture, implementation patterns, and integration with other system components.

## Overview

The Network Interface layer in Hydrogen provides a unified abstraction for network-related operations, enabling:

- Platform-independent network interface enumeration
- IP address management (IPv4 and IPv6)
- Network service discovery integration
- Network status monitoring
- Hardware address resolution
- Interface metrics collection

This abstraction layer is critical for Hydrogen's cross-platform compatibility and service discovery capabilities.

## System Architecture

The Network Interface layer follows a layered architecture with clear platform abstraction:

```diagram
┌──────────────────────────────────────────────────┐
│               Application Layer                  │
│  ┌───────────────┐  ┌────────────────────────┐   │
│  │ mDNS Service  │  │ WebSocket/HTTP Servers │   │
│  └───────┬───────┘  └───────────┬────────────┘   │
│          │                      │                │
│          ▼                      ▼                │
│  ┌────────────────────────────────────────────┐  │
│  │       Network Interface Abstraction        │  │
│  └─────────────────────┬──────────────────────┘  │
│                        │                         │
└────────────────────────┼─────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────┐
│            Platform-Specific Layer               │
│                                                  │
│  ┌────────────────┐      ┌────────────────┐      │
│  │  Linux Network │      │ Other Platform │      │
│  │ Implementation │      │ Implementations│      │
│  └────────────────┘      └────────────────┘      │
│                                                  │
└──────────────────────────────────────────────────┘
```

### Key Components

1. **Network Information Structure**: Core data structure tracking interface states
2. **Platform Abstraction**: Implementation of network operations for specific platforms
3. **Interface Discovery**: Mechanisms to detect and monitor network interfaces
4. **Address Management**: IP address tracking and validation
5. **Metrics Collection**: Network interface performance statistics

## Data Structures

The Network Interface layer uses several key data structures:

### Network Information Structure

```c
typedef struct {
    // Interface data
    char* if_name;           // Interface name (e.g., "eth0")
    uint8_t hw_addr[6];      // Hardware (MAC) address
    char hw_addr_str[18];    // Hardware address as string
    int if_index;            // Interface index
    bool is_loopback;        // Is loopback interface
    bool is_up;              // Is interface up
    
    // IPv4 data
    bool has_ipv4;           // Has IPv4 address
    struct in_addr ipv4_addr;  // IPv4 address
    char ipv4_addr_str[16];  // IPv4 address as string
    struct in_addr ipv4_netmask;  // IPv4 netmask
    struct in_addr ipv4_broadcast;  // IPv4 broadcast address
    
    // IPv6 data
    bool has_ipv6;           // Has IPv6 address
    int ipv6_count;          // Number of IPv6 addresses
    struct in6_addr* ipv6_addrs;  // IPv6 addresses
    char** ipv6_addr_strs;   // IPv6 addresses as strings
    uint8_t* ipv6_prefixes;  // IPv6 address prefixes
    
    // Metrics
    uint64_t bytes_sent;     // Total bytes sent
    uint64_t bytes_received; // Total bytes received
    uint64_t packets_sent;   // Total packets sent
    uint64_t packets_received; // Total packets received
    uint64_t errors_in;      // Input errors
    uint64_t errors_out;     // Output errors
    
    // Timestamp
    time_t last_updated;     // Last update timestamp
} network_interface_t;

typedef struct {
    int interface_count;     // Number of network interfaces
    network_interface_t** interfaces;  // Array of interface pointers
    pthread_mutex_t mutex;   // Mutex for thread safety
} network_info_t;
```

### Network Status Enum

```c
typedef enum {
    NETWORK_STATUS_UNKNOWN = 0,
    NETWORK_STATUS_DISCONNECTED,
    NETWORK_STATUS_LIMITED,
    NETWORK_STATUS_LOCAL_ONLY,
    NETWORK_STATUS_CONNECTED
} network_status_t;
```

## Data Flow

The Network Interface layer manages data flow across multiple layers:

```diagram
┌───────────────┐      ┌────────────────┐      ┌───────────────┐
│ Interface     │      │ Network        │      │ Application   │
│ Hardware      │─────►│ Interface      │─────►│ Services      │
└───────────────┘      │ Abstraction    │      └───────────────┘
       ▲               └────────────────┘              │
       │                        │                      │
       │                        ▼                      │
       │                ┌────────────────┐             │
       └────────────────┤ Monitoring     │◄────────────┘
                        │ System         │
                        └────────────────┘
```

### Key Data Flows

1. **Discovery Flow**:
   - Platform-specific code enumerates network interfaces
   - Interface details loaded into network_interface_t structures
   - Interface collection built in network_info_t
   - Monitoring system notified of interface changes

2. **Monitoring Flow**:
   - Periodic polling of interface status
   - Event-based notifications for status changes
   - Interface metrics collection
   - Status change event generation

3. **Usage Flow**:
   - Application code queries network information
   - Thread-safe access through mutex protection
   - Interface selection based on capabilities
   - Status evaluation for network connectivity

## Network Discovery Process

The Network Interface layer follows a systematic discovery process:

```diagram
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ Enumerate     │────►│ Initialize    │────►│ Get Interface │
│ Interfaces    │     │ Structures    │     │ Details       │
└───────────────┘     └───────────────┘     └───────┬───────┘
                                                    │
                                                    ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ Return        │◄────┤ Build Network │◄────┤ Process       │
│ Info Structure│     │ Info Structure│     │ Address Info  │
└───────────────┘     └───────────────┘     └───────────────┘
```

This process handles:

- Physical and virtual interfaces
- Active and inactive interfaces
- IPv4 and IPv6 addresses
- Interface capabilities
- Hardware addresses
- Metrics and statistics

## Platform Abstraction Model

The Network Interface layer implements a consistent abstraction model:

```diagram
┌─────────────────────────────────────────────────┐
│          Network Interface Public API           │
│                                                 │
│  ┌────────────────┐     ┌─────────────────┐     │
│  │ network_init() │     │ network_update() │    │
│  └────────────────┘     └─────────────────┘     │
│                                                 │
│  ┌────────────────┐     ┌─────────────────┐     │
│  │ network_free() │     │ network_status() │    │
│  └────────────────┘     └─────────────────┘     │
└─────────────────────────────────────────────────┘
                      │
                      │ (Internal Redirection)
                      ▼
┌─────────────────────────────────────────────────┐
│          Platform Implementation Layer          │
│                                                 │
│  ┌─────────────────┐     ┌─────────────────┐    │
│  │  linux_init()   │     │  linux_update() │    │
│  └─────────────────┘     └─────────────────┘    │
│                                                 │
│  ┌─────────────────┐     ┌─────────────────┐    │
│  │  linux_free()   │     │  linux_status() │    │
│  └─────────────────┘     └─────────────────┘    │
└─────────────────────────────────────────────────┘
```

This separation provides several advantages:

- Clean separation between interface and implementation
- Platform-specific code isolation
- Simplified porting to new platforms
- Testability through mock implementations
- Runtime platform detection

## Thread Safety Approach

The Network Interface layer implements comprehensive thread safety:

1. **Mutex Protection**:
   - Main network_info_t protected by mutex
   - Consistent lock/unlock patterns
   - Prevention of mutex leaks

2. **Copy-On-Access**:
   - Critical read operations return copies
   - Prevents races during iteration
   - Minimizes lock contention

3. **Atomic Status**:
   - Status values use atomic operations
   - Ensures clean transitions between states
   - Prevents partial state reads

4. **Synchronized Updates**:
   - Interface array rebuilding under lock
   - Pointer swap for atomic update
   - Clean deletion of old data

## Interface Selection Algorithm

The Network Interface layer implements a sophisticated interface selection algorithm:

```diagram
┌───────────────────────┐
│  Start Selection      │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ Filter Requirements   │
│ - Interface Status    │
│ - Address Family      │
│ - Interface Type      │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│  Sort Candidates      │
│ 1. Non-loopback       │
│ 2. Interface up       │
│ 3. Global scope       │
│ 4. Physical if        │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ Apply Special Rules   │
│ - Preferred Interface │
│ - Service Target      │
│ - Previous Selection  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│ Return Best Interface │
└───────────────────────┘
```

This algorithm ensures:

- Optimal interface selection for services
- Consistent behavior across system restarts
- Appropriate interfaces for different services
- Handling of dynamic interface changes

## IPv6 Support

The Network Interface layer provides comprehensive IPv6 support:

1. **Multiple Address Handling**:
   - Support for multiple IPv6 addresses per interface
   - Address scope tracking (link-local, unique-local, global)
   - Prefix length tracking for subnet calculation

2. **Address Selection**:
   - RFC 6724 compliant source address selection
   - Scope-based selection logic
   - Temporary vs. permanent address handling

3. **Special Cases**:
   - Link-local address handling
   - IPv4-mapped IPv6 addresses
   - Transition mechanisms (6to4, Teredo)

## Integration with mDNS Server

The Network Interface layer directly integrates with the mDNS server component:

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│    Network    │         │     mDNS      │         │  Application  │
│   Interface   │◄────────┤    Server     │◄────────┤   Services    │
└───────┬───────┘         └───────────────┘         └───────────────┘
        │
        │ Interface Changes
        ▼
┌───────────────┐
│  Network      │
│  Events       │
└───────┬───────┘
        │
        │
        ▼
┌───────────────┐
│  Interface    │
│  Selection    │
└───────────────┘
```

Integration points include:

- Interface selection for service advertising
- Network status change notifications
- Interface binding for multicast operations
- Address selection for outgoing connections

## Error Handling Strategy

The Network Interface layer implements robust error handling:

1. **Categorized Errors**:
   - Permission errors (for raw socket operations)
   - Hardware availability errors
   - Address configuration errors
   - System resource errors

2. **Recovery Mechanisms**:
   - Automatic retry for transient errors
   - Fallback interfaces for critical operations
   - Graceful degradation for partial failures

3. **Error Propagation**:
   - Clear error codes returned to callers
   - Detailed error logging
   - Status tracking for error conditions

## Performance Considerations

The Network Interface layer is optimized for performance:

1. **Caching**:
   - Interface information cached with validity periods
   - Lazy updates based on access patterns
   - Cache invalidation on detected changes

2. **Minimal System Calls**:
   - Batched interface queries
   - Efficient netlink usage on Linux
   - Non-blocking I/O operations

3. **Memory Efficiency**:
   - Custom memory pools for interface structures
   - String storage optimization
   - Pointer-based structure updates

## Implementation Files

The Network Interface layer is implemented in these key files:

| File | Purpose |
|------|---------|
| `src/network/network.h` | Public interface definitions |
| `src/network/network_linux.c` | Linux-specific implementation |
| `src/network/network_common.c` | Platform-independent code |
| `src/network/network_ipv6.c` | IPv6-specific functionality |
| `src/network/network_monitor.c` | Interface monitoring system |

## Usage Examples

### Initializing Network Interface Layer

```c
// Initialize network interface abstraction
network_info_t* net_info = network_init();
if (!net_info) {
    log_this("Network", "Failed to initialize network interface layer", LOG_LEVEL_ERROR, true, true, true);
    return false;
}

// Log discovered interfaces
log_this("Network", "Discovered %d network interfaces", LOG_LEVEL_STATE, net_info->interface_count);
for (int i = 0; i < net_info->interface_count; i++) {
    network_interface_t* iface = net_info->interfaces[i];
    log_this("Network", "Interface %s: %s %s", LOG_LEVEL_STATE, 
             iface->if_name, 
             iface->has_ipv4 ? iface->ipv4_addr_str : "No IPv4",
             iface->is_up ? "UP" : "DOWN");
}

// Store in global state for other components
app_state->net_info = net_info;
```

### Finding Best Interface for Service

```c
// Find best interface for mDNS service
network_interface_t* find_mdns_interface(network_info_t* net_info) {
    if (!net_info) return NULL;
    
    pthread_mutex_lock(&net_info->mutex);
    
    // Look for non-loopback interface with IPv4
    network_interface_t* best_interface = NULL;
    for (int i = 0; i < net_info->interface_count; i++) {
        network_interface_t* iface = net_info->interfaces[i];
        
        // Skip interfaces that don't meet requirements
        if (!iface->is_up || iface->is_loopback || !iface->has_ipv4) {
            continue;
        }
        
        // Found a candidate - make a copy
        best_interface = malloc(sizeof(network_interface_t));
        if (best_interface) {
            memcpy(best_interface, iface, sizeof(network_interface_t));
            
            // Deep copy strings to ensure thread safety
            best_interface->if_name = strdup(iface->if_name);
            if (iface->ipv6_count > 0) {
                // Copy IPv6 addresses (implementation omitted for brevity)
            }
        }
        break;
    }
    
    pthread_mutex_unlock(&net_info->mutex);
    return best_interface;
}
```

### Monitoring Network Status

```c
// Check overall network status
network_status_t check_network_status(network_info_t* net_info) {
    if (!net_info) return NETWORK_STATUS_UNKNOWN;
    
    network_status_t status = network_status(net_info);
    
    switch (status) {
        case NETWORK_STATUS_CONNECTED:
            log_this("Network", "Network is fully connected", LOG_LEVEL_STATE, true, false, false);
            break;
            
        case NETWORK_STATUS_LOCAL_ONLY:
            log_this("Network", "Network has only local connectivity", LOG_LEVEL_ALERTING, true, false, false);
            break;
            
        case NETWORK_STATUS_LIMITED:
            log_this("Network", "Network has limited connectivity", LOG_LEVEL_ALERTING, true, false, false);
            break;
            
        case NETWORK_STATUS_DISCONNECTED:
            log_this("Network", "Network is disconnected", LOG_LEVEL_ERROR, true, false, false);
            break;
            
        default:
            log_this("Network", "Network status is unknown", LOG_LEVEL_ALERTING, true, false, false);
            break;
    }
    
    return status;
}
```

### Updating Network Information

```c
// Periodic network information update
void update_network_info(network_info_t* net_info) {
    if (!net_info) return;
    
    // Perform update
    if (network_update(net_info)) {
        log_this("Network", "Network information updated successfully", LOG_LEVEL_DEBUG, false, true, false);
        
        // Check for interface changes
        for (int i = 0; i < net_info->interface_count; i++) {
            network_interface_t* iface = net_info->interfaces[i];
            
            // Calculate bandwidth
            time_t now = time(NULL);
            double elapsed = difftime(now, iface->last_updated);
            if (elapsed > 0 && iface->last_updated > 0) {
                double tx_rate = 0, rx_rate = 0;
                
                // Implementation of bandwidth calculation omitted for brevity
                
                log_this("Network", "Interface %s bandwidth: TX %.2f KB/s, RX %.2f KB/s", 
                         LOG_LEVEL_DEBUG, iface->if_name, tx_rate, rx_rate);
            }
            
            iface->last_updated = now;
        }
    } else {
        log_this("Network", "Failed to update network information", LOG_LEVEL_ALERTING, true, true, false);
    }
}
```

## Related Documentation

- [System Information](/docs/H/core/system_info.md) - System information endpoint that reports network status
- [mDNS Server](/docs/H/core//subsystems/mdnsserver/mdnsserver.md) - mDNS service that uses the network interface layer
- [Configuration Guide](/docs/H/core/reference/configuration.md) - Network-related configuration options
