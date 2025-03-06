/*
 * Configuration management system with robust fallback handling
 * 
 * The configuration system implements several key design principles:
 * 
 * Fault Tolerance:
 * - Graceful fallback to defaults for missing values
 * - Validation of critical parameters
 * - Type checking for all values
 * - Memory allocation failure recovery
 * 
 * Flexibility:
 * - Runtime configuration changes
 * - Environment-specific overrides
 * - Service-specific settings
 * - Extensible structure
 * 
 * Security:
 * - Sensitive data isolation
 * - Path validation
 * - Size limits enforcement
 * - Access control settings
 * 
 * Maintainability:
 * - Centralized default values
 * - Structured error reporting
 * - Clear upgrade paths
 * - Configuration versioning
 */

#ifndef CONFIG_H
#define CONFIG_H

// Core system headers
#include <sys/types.h>
#include <stddef.h>

// Version information
#ifndef VERSION
#define VERSION "0.1.0"
#endif

// Project headers for type-specific handling
#include "config_env.h"
#include "config_string.h"
#include "config_bool.h"
#include "config_int.h"
#include "config_size.h"
#include "config_double.h"

// Default values
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_LOG_FILE "/var/log/hydrogen.log"
#define DEFAULT_WEB_PORT 5000
#define DEFAULT_WEBSOCKET_PORT 5001
#define DEFAULT_UPLOAD_PATH "/api/upload"
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB

// File descriptor defaults
#define DEFAULT_FD_TYPE_SIZE 32        // Size for file descriptor type strings
#define DEFAULT_FD_DESCRIPTION_SIZE 128 // Size for file descriptor descriptions

// Network defaults
#define DEFAULT_MAX_INTERFACES 16
#define DEFAULT_MAX_IPS_PER_INTERFACE 8
#define DEFAULT_MAX_INTERFACE_NAME_LENGTH 16
#define DEFAULT_MAX_IP_ADDRESS_LENGTH 40

// System monitoring defaults
#define DEFAULT_STATUS_UPDATE_MS 1000
#define DEFAULT_RESOURCE_CHECK_MS 5000
#define DEFAULT_METRICS_UPDATE_MS 1000
#define DEFAULT_MEMORY_WARNING_PERCENT 90
#define DEFAULT_DISK_WARNING_PERCENT 90
#define DEFAULT_LOAD_WARNING 5.0

// Print queue defaults
#define DEFAULT_SHUTDOWN_WAIT_MS 3000
#define DEFAULT_JOB_PROCESSING_TIMEOUT_MS 30000

// Motion system defaults
#define DEFAULT_MAX_LAYERS 100000
#define DEFAULT_ACCELERATION 3000.0
#define DEFAULT_Z_ACCELERATION 100.0
#define DEFAULT_E_ACCELERATION 10000.0
#define DEFAULT_MAX_SPEED_XY 500.0
#define DEFAULT_MAX_SPEED_TRAVEL 500.0
#define DEFAULT_MAX_SPEED_Z 10.0
#define DEFAULT_Z_VALUES_CHUNK 1000

// Configuration structures
#include "config_structs.h"

/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 */
const AppConfig* get_app_config(void);

/*
 * Load and validate configuration with comprehensive error handling
 * 
 * Loads configuration from the specified file, applying:
 * - Type validation
 * - Range checking
 * - Default value fallbacks
 * - Environment variable substitution
 * 
 * @param config_path Path to the configuration file
 * @return Pointer to loaded configuration or NULL on error
 */
AppConfig* load_config(const char* config_path);

/*
 * Generate default configuration with secure baseline
 * 
 * Creates a new configuration file with secure defaults:
 * - Conservative file permissions
 * - Secure network settings
 * - Resource limits
 * - Standard service ports
 * 
 * @param config_path Path where configuration should be created
 */
void create_default_config(const char* config_path);

#endif /* CONFIG_H */