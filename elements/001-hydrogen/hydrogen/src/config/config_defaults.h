/*
 * Default configuration generation with secure baselines
 * 
 * This module handles the generation of default configurations for all subsystems:
 * 
 * Why This Design:
 * 1. Separation of Concerns
 *    - Each subsystem has its own default generator
 *    - Clear responsibility boundaries
 *    - Easier to maintain and update
 * 
 * 2. Security First
 *    - Conservative file permissions
 *    - Secure network settings
 *    - Resource limits to prevent DoS
 *    - Separate ports for services
 * 
 * 3. Operational Safety
 *    - Reasonable resource limits
 *    - Safe default paths
 *    - Controlled access patterns
 *    - Clear upgrade paths
 */

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include <jansson.h>

/*
 * Generate complete default configuration
 * 
 * Creates a new configuration file with secure defaults for all subsystems:
 * - Web server configuration
 * - WebSocket settings
 * - mDNS server setup
 * - System resources
 * - Network configuration
 * - System monitoring
 * - Print queue settings
 * - API configuration
 * 
 * @param config_path Path where configuration should be created
 */
void create_default_config(const char* config_path);

/*
 * Generate default web server configuration
 * 
 * Creates secure defaults for the web server:
 * - Conservative file permissions
 * - Standard ports
 * - Upload limits
 * - API prefixes
 * 
 * @return JSON object with web server configuration or NULL on error
 */
json_t* create_default_web_config(void);

/*
 * Generate default WebSocket configuration
 * 
 * Creates secure defaults for WebSocket:
 * - Secure protocol settings
 * - Message size limits
 * - Connection timeouts
 * 
 * @return JSON object with WebSocket configuration or NULL on error
 */
json_t* create_default_websocket_config(void);

/*
 * Generate default mDNS server configuration
 * 
 * Creates defaults for service discovery:
 * - Device identification
 * - Service advertisements
 * - Network protocols
 * 
 * @return JSON object with mDNS configuration or NULL on error
 */
json_t* create_default_mdns_config(void);

/*
 * Generate default system resources configuration
 * 
 * Creates defaults for resource management:
 * - Queue settings
 * - Buffer sizes
 * - Memory limits
 * 
 * @return JSON object with resources configuration or NULL on error
 */
json_t* create_default_resources_config(void);

/*
 * Generate default network configuration
 * 
 * Creates defaults for network settings:
 * - Interface limits
 * - Port allocations
 * - Reserved ports
 * 
 * @return JSON object with network configuration or NULL on error
 */
json_t* create_default_network_config(void);

/*
 * Generate default system monitoring configuration
 * 
 * Creates defaults for monitoring:
 * - Update intervals
 * - Warning thresholds
 * - Resource checks
 * 
 * @return JSON object with monitoring configuration or NULL on error
 */
json_t* create_default_monitoring_config(void);

/*
 * Generate default print queue configuration
 * 
 * Creates defaults for print management:
 * - Queue priorities
 * - Processing timeouts
 * - Buffer sizes
 * 
 * @return JSON object with print queue configuration or NULL on error
 */
json_t* create_default_print_queue_config(void);

/*
 * Generate default API configuration
 * 
 * Creates secure defaults for API:
 * - JWT settings
 * - Security tokens
 * - Access controls
 * 
 * @return JSON object with API configuration or NULL on error
 */
json_t* create_default_api_config(void);

#endif /* CONFIG_DEFAULTS_H */