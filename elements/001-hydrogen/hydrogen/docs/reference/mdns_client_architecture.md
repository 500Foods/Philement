# mDNS Client Architecture

The mDNS Client subsystem provides service discovery and monitoring capabilities, enabling Hydrogen to discover and track both its own services and other network services for load balancing and high availability.

## System Overview

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     mDNS Client System                        │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Service     │         │   Service     │                 │
│   │   Scanner     │────────►│   Monitor     │                 │
│   └───────┬───────┘         └───────┬───────┘                 │
│           │                         │                         │
│           ▼                         ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Service     │         │   Health      │                 │
│   │   Registry    │◄────────┤   Checker     │                 │
│   └───────┬───────┘         └───────────────┘                 │
│           │                                                   │
│           ▼                                                   │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Event       │────────►│   Load        │                 │
│   │   Dispatcher  │         │   Balancer    │                 │
│   └───────────────┘         └───────────────┘                 │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Key Components

### Service Scanner

- Periodic network scanning
- Service type filtering
- Discovery protocol handling
- Initial service detection

### Service Monitor

- Real-time service tracking
- State change detection
- Service metadata updates
- Event generation

### Service Registry

- Service state storage
- Service metadata caching
- Query interface
- State persistence

### Health Checker

- Service availability testing
- Response time monitoring
- Error detection
- Health status tracking

### Event Dispatcher

- Service state notifications
- Health status updates
- Load balancer events
- System notifications

## Configuration

The mDNS Client subsystem is configured through the following settings in hydrogen.json:

```json
{
    "mDNSClient": {
        "Enabled": true,
        "EnableIPv6": true,
        "ScanIntervalMs": 5000,
        "ServiceTypes": [
            "_http._tcp",
            "_octoprint._tcp",
            "_websocket._tcp",
            "_hydrogen._tcp"
        ],
        "MonitoredServices": {
            "OwnServices": true,
            "LoadBalancers": true,
            "PrinterServices": true,
            "CustomServices": []
        },
        "HealthCheck": {
            "Enabled": true,
            "IntervalMs": 30000,
            "TimeoutMs": 5000,
            "RetryCount": 3
        }
    }
}
```

## Service Discovery Flow

```sequence
Scanner->Network: Browse for services
Network->Scanner: Service found
Scanner->Registry: Register service
Registry->Monitor: Start monitoring
Monitor->HealthChecker: Check service health
HealthChecker->Registry: Update health status
Registry->EventDispatcher: Service state change
EventDispatcher->LoadBalancer: Update service pool
```

### Service States

1. **Discovery**
   - Initial detection
   - Metadata collection
   - Type verification
   - Registry addition

2. **Monitoring**
   - Health checking
   - State tracking
   - Metadata updates
   - Event generation

3. **Termination**
   - Service removal
   - Resource cleanup
   - Event notification
   - Registry update

## Error Handling

1. **Network Errors**
   - Connection failures
   - Timeout handling
   - Retry logic
   - Event notification

2. **Service Errors**
   - Health check failures
   - State inconsistencies
   - Protocol errors
   - Recovery procedures

3. **System Errors**
   - Resource exhaustion
   - Thread failures
   - Memory issues
   - Error reporting

## Integration Points

### mDNS Server

- Service verification
- State coordination
- Event synchronization
- Health monitoring

### Network Interface

- Service discovery
- Protocol handling
- Connection management
- Error detection

### Load Balancer

- Service selection
- Health status
- Availability updates
- Failover handling

## Performance Considerations

1. **Resource Management**
   - Memory utilization
   - Thread allocation
   - Network bandwidth
   - CPU usage

2. **Scalability**
   - Service count limits
   - Monitor distribution
   - Event throughput
   - Registry size

3. **Efficiency**
   - Cache optimization
   - Health check timing
   - Event batching
   - State updates

## Testing

1. **Unit Tests**
   - Service discovery
   - Health checking
   - State management
   - Event handling

2. **Integration Tests**
   - End-to-end flow
   - Error recovery
   - Load balancing
   - Performance metrics

3. **Network Tests**
   - Protocol compliance
   - Timeout handling
   - Error conditions
   - Recovery behavior

## Future Considerations

1. **Feature Enhancements**
   - Additional protocols
   - Advanced monitoring
   - Custom health checks
   - Extended metrics

2. **Performance Improvements**
   - Optimized scanning
   - Efficient monitoring
   - Better caching
   - Reduced overhead

3. **Integration Extensions**
   - Cloud services
   - External monitors
   - Custom protocols
   - Advanced routing

## Service Types

### Standard Services

- HTTP servers
- WebSocket endpoints
- OctoPrint instances
- Hydrogen nodes

### Load Balancers

- HTTP load balancers
- WebSocket balancers
- Print job distributors
- Service proxies

### Custom Services

- User-defined types
- Special protocols
- Test services
- Development aids

## Health Checking

### Methods

1. **TCP Connection**
   - Port availability
   - Connection time
   - Response validation
   - Error detection

2. **HTTP Health Check**
   - Endpoint testing
   - Status verification
   - Response timing
   - Content validation

3. **Custom Checks**
   - Protocol-specific
   - Service-defined
   - User-configured
   - Extended validation

### Metrics

- Response time
- Success rate
- Error frequency
- State duration

## Event System

### Event Types

1. **Discovery Events**
   - Service found
   - Service lost
   - Type matched
   - Metadata updated

2. **Health Events**
   - Status change
   - Check result
   - Timeout occurred
   - Retry attempted

3. **State Events**
   - Registry updated
   - Monitor started
   - Service removed
   - Error occurred

### Event Handling

- Asynchronous processing
- Priority queuing
- Error recovery
- State synchronization

## Load Balancing

### Strategies

1. **Round Robin**
   - Simple distribution
   - Equal weighting
   - Basic fairness
   - No health consideration

2. **Health Weighted**
   - Response time based
   - Health status aware
   - Error rate conscious
   - Load balanced

3. **Custom**
   - User-defined rules
   - Service specific
   - Context aware
   - Dynamic adjustment

### Failover

- Health monitoring
- Service switching
- State preservation
- Error recovery
