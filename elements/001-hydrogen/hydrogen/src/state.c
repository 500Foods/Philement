/*
 * Safety-Critical State Management for 3D Printer Control
 * 
 * Why Centralized State Management?
 * 1. Safety Requirements
 *    - Emergency stop coordination
 *    - Temperature limit enforcement
 *    - Motion boundary checking
 *    - End-stop signal handling
 * 
 * 2. Hardware State Tracking
 *    Why This Matters?
 *    - Prevent conflicting movements
 *    - Monitor thermal stability
 *    - Track filament flow
 *    - Detect sensor failures
 * 
 * 3. Real-Time Coordination
 *    Why So Critical?
 *    - Synchronize multiple motors
 *    - Balance heating elements
 *    - Control cooling systems
 *    - Time sensitive operations
 * 
 * 4. Error Recovery
 *    Why This Approach?
 *    - Safe failure modes
 *    - Preserve print progress
 *    - Protect mechanical parts
 *    - Enable manual recovery
 * 
 * 5. Resource Protection
 *    Why These Safeguards?
 *    - Prevent heater runaway
 *    - Avoid motor overload
 *    - Monitor power systems
 *    - Track resource usage
 * 
 * 6. Operational Modes
 *    Why Multiple Modes?
 *    - Normal printing state
 *    - Emergency stop state
 *    - Maintenance mode
 *    - Calibration state
 * 
 * 7. State Transitions
 *    Why So Careful?
 *    - Validate temperature changes
 *    - Ensure safe motion paths
 *    - Coordinate tool changes
 *    - Handle power events
 */

#include "state.h"

// Core state flags implementing coordinated lifecycle management
//
// Design choices for global state:
// 1. Atomic Operations
//    - sig_atomic_t for signal safety
//    - Prevents partial updates
//    - Ensures visibility across cores
//
// 2. Minimal State
//    - Binary flags only
//    - No complex state machines
//    - Clear state transitions
//
// 3. Thread Coordination
//    - Mutex for state changes
//    - Condition variable for waiting
//    - Broadcast notifications
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component-specific shutdown flags with dependency awareness
//
// Shutdown flag design prioritizes:
// 1. Dependency Order
//    - mDNS first (stop advertising)
//    - Network services next
//    - Core systems last
//
// 2. Safety
//    - Atomic operations only
//    - Independent state tracking
//    - Prevents deadlocks
//
// 3. Observability
//    - Clear shutdown progress
//    - Component state tracking
//    - Debugging support
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t mdns_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;

// System thread handles with lifecycle management
//
// Thread handle centralization enables:
// 1. Resource Management
//    - Consistent cleanup
//    - Join operations
//    - Handle validation
//
// 2. Shutdown Coordination
//    - Ordered termination
//    - Resource release
//    - Deadlock prevention
//
// 3. State Tracking
//    - Component health
//    - Resource usage
//    - System monitoring
pthread_t log_thread;
pthread_t print_queue_thread;
pthread_t mdns_thread;
pthread_t web_thread;
pthread_t websocket_thread;

// Shared resource handles with centralized management
//
// Resource management strategy:
// 1. Access Control
//    - Single source of truth
//    - Controlled initialization
//    - Safe deallocation
//
// 2. Lifecycle Management
//    - Dependency tracking
//    - Ordered initialization
//    - Clean shutdown
//
// 3. Memory Safety
//    - Clear ownership
//    - Null safety
//    - Leak prevention
AppConfig *app_config = NULL;
mdns_t *mdns = NULL;
network_info_t *net_info = NULL;