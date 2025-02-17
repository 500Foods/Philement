/*
 * Safe Initialization Sequence for 3D Printer Control
 * 
 * Why Careful Startup Matters:
 * 1. Hardware Protection
 *    - Temperature sensor validation
 *    - Motor controller checks
 *    - End-stop verification
 *    - Power system testing
 * 
 * 2. Component Dependencies
 *    Why This Order?
 *    - Safety systems first
 *    - Core services next
 *    - Network last
 *    - User interfaces last
 * 
 * 3. Resource Validation
 *    Why These Checks?
 *    - Configuration integrity
 *    - File system access
 *    - Network availability
 *    - Memory requirements
 * 
 * 4. Error Prevention
 *    Why So Thorough?
 *    - Prevent partial starts
 *    - Validate all subsystems
 *    - Ensure safe states
 *    - Enable recovery
 * 
 * 5. System Health
 *    Why These Tests?
 *    - Component readiness
 *    - Resource availability
 *    - Service stability
 *    - Communication paths
 */

#ifndef HYDROGEN_STARTUP_H
#define HYDROGEN_STARTUP_H

// Initialize all system components in dependency order
// Performs safety checks and resource validation
// Returns 1 on successful initialization, 0 on any failure
int startup_hydrogen(const char *config_path);

#endif // HYDROGEN_STARTUP_H