# Threads Subsystem Launch Process

The Threads subsystem is responsible for initializing and tracking thread management in Hydrogen. It serves as a first-class subsystem citizen that monitors thread counts and provides thread management services to other subsystems.

## Launch Order

The Threads subsystem is initialized after the Payload subsystem and before the Network subsystem. This positioning ensures:

1. Basic system services are available (from Payload)
2. Thread tracking is established before network operations begin
3. Other subsystems can rely on thread management being available

## Launch Readiness

The Threads subsystem performs a simple but critical launch readiness check:

```diagram
┌─────────────────┐
│ System State    │
│ Check           │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Thread Count    │
│ Verification    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Launch Decision │
└─────────────────┘
```

### Launch Messages

The launch readiness check produces the following messages:

```launch
Threads
  Go:      System check passed (not shutting down)
  Go:      Current thread count: 1 (main thread)
  Decide:  Launch Threads
```

### Launch Criteria

The Threads subsystem's launch readiness check:

1. Verifies the system is not in shutdown state
2. Counts current threads (including main thread)
3. Always returns "Go" if system is not shutting down

## Integration Points

The Threads subsystem integrates with:

- Main thread tracking from hydrogen.c
- ServiceThreads structures for all subsystems
- Thread monitoring and metrics collection

## Shutdown Process

During shutdown, the Threads subsystem:

1. Reports final thread count
2. Verifies thread count matches startup (typically 1 for main thread)
3. Ensures clean thread state before system exit

## Error Handling

The Threads subsystem handles the following error conditions:

- System shutdown state detection
- Thread counting failures
- Resource cleanup during shutdown

## Configuration

The Threads subsystem requires no specific configuration as it provides core thread management functionality.

## Best Practices

When working with the Threads subsystem:

1. Always account for the main thread in thread counts
2. Use ServiceThreads structures for thread tracking
3. Ensure proper thread cleanup during shutdown
4. Monitor thread counts for unexpected changes

## Related Documentation

- [Launch System Architecture](../launch_system_architecture.md)
- [System Architecture](../system_architecture.md)
- [Thread Monitoring](/docs/reference/thread_monitoring.md)
