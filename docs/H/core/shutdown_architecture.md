# Shutdown Architecture

This document describes the shutdown architecture of the Hydrogen server, focusing on the mechanisms that ensure safe, reliable, and complete termination of all system components.

## Core Principles

The Hydrogen server's shutdown mechanism is designed with several critical principles in mind:

1. **Hardware Safety**: 3D printers contain heating elements, motors, and other physically active components that must be powered down safely to prevent damage or hazards.

2. **Print Job Preservation**: When possible, print job state should be saved to enable recovery.

3. **Resource Cleanup**: All system resources (memory, file handles, network connections) must be properly released to prevent leaks.

4. **Thread Coordination**: All threads must terminate cleanly without deadlocks or race conditions.

5. **Graceful Service Termination**: Network services must complete pending operations and notify clients before terminating.

6. **Fallback Mechanisms**: If preferred shutdown methods fail, progressive fallbacks ensure eventual system termination.

## Shutdown Sequence

The shutdown process follows a carefully orchestrated sequence:

1. **Initiation**:
   - Triggered by SIGINT (Ctrl+C) or programmatic call to `graceful_shutdown()`
   - Sets `server_running = 0` and signals all threads via condition variable

2. **Service Announcement Termination**:
   - mDNS Server shutdown initiated first
   - "Goodbye" packets sent with TTL=0 to notify network of service withdrawal
   - Sockets closed and resources freed

3. **Network Services Termination**:
   - Web server shutdown
   - WebSocket server shutdown with client notification
   - Waiting for active connections to complete

4. **Print System Termination**:
   - Queue processing halted
   - Current job status preserved if possible
   - Resources released

5. **Network Resource Cleanup**:
   - Network interfaces freed
   - Shared network information released

6. **Thread Monitoring and Cleanup**:
   - Wait for threads to exit with progressive timeouts
   - Thread status verification
   - Thread cleanup with fallback mechanisms

7. **Final Resource Cleanup**:
   - Queue system destruction
   - Synchronization primitives destruction
   - Configuration cleanup

## Thread Management

Hydrogen employs a sophisticated thread tracking system:

1. **Thread Registration**:
   - Threads register using `add_service_thread()` when started
   - Thread IDs and metrics stored in service-specific thread pools

2. **Thread Metrics**:
   - Regular calls to `update_service_thread_metrics()` maintain current thread status
   - Dead threads automatically pruned from metrics

3. **Thread Termination**:
   - Primary mechanism: Signal threads to self-terminate via condition variables
   - Secondary mechanism: Join threads with timeout
   - Fallback mechanism: Thread cancellation for stuck threads

4. **Thread Verification**:
   - Shutdown process verifies actual thread termination
   - Checks thread state via `/proc/{tid}/status` when available
   - Identifies threads in uninterruptible sleep states (D state)

## Resource Cleanup

Resources are released in a specific order to prevent use-after-free scenarios:

1. **Two-Phase Pointer Handling**:
   - Store local copy of resource pointer
   - Set global pointer to NULL
   - Free resource using local copy

2. **Memory Cleanup**:
   - Free dynamically allocated structures in reverse allocation order
   - Recursively free complex data structures
   - NULL pointers after freeing to prevent double-free

3. **Handle Closing**:
   - Close file descriptors and sockets explicitly
   - Verify closure success

4. **State Reset**:
   - Reset volatile flags
   - Clear shared state structures

## WebSocket Server Shutdown

The WebSocket server uses libwebsockets, which requires special handling:

1. **Initial Signal Phase**:
   - Set `websocket_server_shutdown = 1`
   - Broadcast on condition variable

2. **Connection Closure Phase**:
   - Wait for connections to close gracefully (2s timeout)
   - Call `stop_websocket_server()` to initiate server thread termination

3. **Thread Termination Phase**:
   - Join server thread with 5s timeout
   - Use `pthread_timedjoin_np()` for controlled waiting

4. **Resource Cleanup Phase**:
   - Wait for thread exit before context destruction
   - Local copy of context pointer to prevent race conditions
   - Clean service loops to process all pending events
   - Context destruction with proper synchronization

5. **Fallback Mechanisms**:
   - Force connection count to zero after timeout
   - Cancel service explicitly if normal termination fails
   - Delay final resource destruction to ensure callbacks complete

## mDNS Server Shutdown

The mDNS Server requires special network-aware shutdown:

1. **Service Withdrawal**:
   - Send RFC-compliant "goodbye" packets (TTL=0) for all services
   - Send on all network interfaces (IPv4 and IPv6)
   - Repeat 3 times with 250ms delay for reliability

2. **Thread Coordination**:
   - Set `mdns_server_system_shutdown = 1`
   - Wait for mDNS threads to notice shutdown flag
   - Progressive timeouts (10 x 200ms) for thread exit

3. **Socket Cleanup**:
   - Explicit socket closing for each interface
   - Both IPv4 and IPv6 sockets properly closed

4. **Resource Cleanup**:
   - Free interface structures in correct order
   - Release all dynamically allocated memory
   - Brief delay before final cleanup

## Synchronization Primitives

Shutdown relies on several synchronization mechanisms:

1. **Mutex Protection**:
   - `terminate_mutex` protects shared state
   - Component-specific mutexes protect subsystem state

2. **Condition Variables**:
   - `terminate_cond` for signaling shutdown
   - Component-specific condition variables for subsystem coordination

3. **Atomic Operations**:
   - Volatile sig_atomic_t flags for signal-safe state changes
   - Thread-safe counter operations

## Error Handling During Shutdown

The shutdown process is designed to be resilient to errors:

1. **Progressive Timeouts**:
   - Initial generous timeouts for normal operation
   - Shorter secondary timeouts for forced cleanup

2. **Isolation of Components**:
   - Each component shutdown isolated to prevent cascading failures
   - Failures in one component don't prevent others from shutting down

3. **Extensive Logging**:
   - Detailed logging of each shutdown step
   - Error conditions recorded for post-mortem analysis

4. **Shutdown State Tracking**:
   - `server_stopping` flag differentiates normal and shutdown states
   - Prevents inappropriate resource acquisition during shutdown

## Shutdown Monitoring

The shutdown process itself is monitored:

1. **Timing Statistics**:
   - `record_shutdown_start_time()` and `record_shutdown_end_time()`
   - Total shutdown duration available for performance analysis

2. **Thread State Analysis**:
   - Advanced state analysis via /proc filesystem
   - Detection of stuck threads in uninterruptible states
   - Syscall identification for blocked threads

3. **Resource Usage Tracking**:
   - Memory leaks detection
   - File descriptor tracking
   - Network socket monitoring

## Best Practices for Component Developers

When adding new components to Hydrogen, follow these shutdown guidelines:

1. **Register Threads**: Always register threads with the appropriate service thread pool.

2. **Check Shutdown Flags**: Regularly check relevant shutdown flags in thread loops.

3. **Support Condition Variables**: Wait on the appropriate condition variable with timeout.

4. **Two-Phase Resource Cleanup**:
   - First phase: Signal intent to shut down
   - Second phase: Actual resource cleanup

5. **Handle Timeouts**: Implement fallback mechanisms for timeout scenarios.

6. **Verify Resource Release**: Confirm that all resources are properly released.

7. **Document Shutdown Behavior**: Add shutdown details to component documentation.

## Troubleshooting Shutdown Issues

Common shutdown issues and solutions:

1. **Hung Threads**:
   - Check for infinite loops missing shutdown flag checks
   - Look for blocking I/O operations without timeout
   - Verify proper mutex unlock in all code paths

2. **Resource Leaks**:
   - Use `valgrind` to identify memory leaks
   - Check for unclosed file descriptors with `lsof`
   - Monitor socket states with `ss` or `netstat`

3. **Race Conditions**:
   - Add diagnostic logging to identify sequence issues
   - Increase delays between shutdown phases for debugging
   - Use thread sanitizers to detect race conditions

4. **Driver Integration Issues**:
   - Hardware drivers may need special shutdown handling
   - Check for device-specific cleanup requirements

## Subsystem Registry Integration

The Subsystem Registry plays a crucial role in the shutdown process by managing dependencies and coordinating the shutdown sequence:

1. **Dependency-Aware Termination**:
   - Subsystems are shut down in reverse dependency order
   - The registry prevents termination of subsystems that others depend on
   - Ensures all dependent subsystems are stopped before their dependencies

2. **State Tracking**:
   - Registry maintains real-time state information for all subsystems
   - Each subsystem transitions through states: RUNNING → STOPPING → INACTIVE
   - State transitions are logged and timestamped for diagnostics

3. **Centralized Shutdown Control**:
   - `update_subsystem_registry_on_shutdown()` coordinates shutdown of all registered subsystems
   - Registry ensures all subsystems receive shutdown signals
   - Verifies all subsystems have terminated properly

4. **Shutdown Sequence Management**:
   - Registry maintains a consistent shutdown order based on dependencies
   - Components with no dependents are stopped first
   - Core systems with many dependents are stopped last

The registry provides real-time monitoring of the shutdown progress, allowing the main shutdown sequence to adapt to conditions and ensure all subsystems are properly terminated.

For detailed information about the Subsystem Registry design and implementation, see the [Subsystem Registry Architecture](/docs/H/core/reference/subsystem_registry_architecture.md) document.

## References

- [Subsystem Registry Architecture](/docs/H/core/reference/subsystem_registry_architecture.md) - Subsystem lifecycle management
- [Thread Monitoring](/docs/H/core/reference/thread_monitoring.md) - Detailed thread diagnostic information
- [WebSocket Interface](/docs/H/core/subsystems/websocket/websocket.md) - WebSocket server shutdown implementation
- [mDNS Server](/docs/H/core/subsystems/mdnsserver/mdnsserver.md) - mDNS server shutdown implementation
- [System Info API](/docs/H/api/system/system_info.md) - System health and status monitoring
- [Release Notes](/RELEASES.md) - History of shutdown improvements
