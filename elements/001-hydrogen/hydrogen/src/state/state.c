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

// Core state flags

volatile sig_atomic_t server_starting = 1;  // Start as true, will be set to false once startup complete
volatile sig_atomic_t server_running = 0;
volatile sig_atomic_t server_stopping = 0;

pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component-specific shutdown flags with dependency awareness

volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;
volatile sig_atomic_t mdns_server_system_shutdown = 0;
volatile sig_atomic_t mdns_client_system_shutdown = 0;
volatile sig_atomic_t smtp_relay_system_shutdown = 0;
volatile sig_atomic_t swagger_system_shutdown = 0;
volatile sig_atomic_t terminal_system_shutdown = 0;
volatile sig_atomic_t print_system_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;

// System thread handles with lifecycle management

pthread_t log_thread;
pthread_t web_thread;
pthread_t websocket_thread;
pthread_t mdns_server_thread;
pthread_t print_queue_thread;

// Thread tracking structures 

ServiceThreads logging_threads;
ServiceThreads web_threads;
ServiceThreads websocket_threads;
ServiceThreads mdns_server_threads;
ServiceThreads print_threads;

// Shared resource handles

AppConfig *app_config = NULL;
mdns_server_t *mdns_server = NULL;
network_info_t *net_info = NULL;