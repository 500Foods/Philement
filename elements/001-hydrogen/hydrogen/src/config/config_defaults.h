/*
 * Default configuration generation with secure baselines
 * 
 * This module handles the generation of default configurations for all subsystems
 * in strict A-O order:
 * 
 * A. Server
 *    - Core system identification
 *    - Essential paths and locations
 *    - Security settings
 *    - Runtime behavior
 * 
 * B. Network
 *    - Interface configuration
 *    - Port allocations
 *    - Reserved ports
 *    - Network protocols
 * 
 * C. Database
 *    - Connection settings
 *    - Pool configuration
 *    - Security parameters
 *    - Performance tuning
 * 
 * D. Logging
 *    - Log levels
 *    - Output targets
 *    - File rotation
 *    - Format settings
 * 
 * E. WebServer
 *    - HTTP configuration
 *    - Upload limits
 *    - Thread pools
 *    - Connection management
 * 
 * F. API
 *    - Endpoint configuration
 *    - Authentication
 *    - Rate limiting
 *    - Version control
 * 
 * G. Swagger
 *    - Documentation
 *    - UI options
 *    - API exploration
 *    - Schema validation
 * 
 * H. WebSockets
 *    - Protocol settings
 *    - Message limits
 *    - Connection timeouts
 *    - Security options
 * 
 * I. Terminal
 *    - Shell access
 *    - Session limits
 *    - Idle timeouts
 *    - Command restrictions
 * 
 * J. mDNS Server
 *    - Service advertisement
 *    - Device discovery
 *    - Network protocols
 *    - Service types
 * 
 * K. mDNS Client
 *    - Service discovery
 *    - Network scanning
 *    - Health checks
 *    - Auto-configuration
 * 
 * L. MailRelay
 *    - SMTP settings
 *    - Server pools
 *    - Authentication
 *    - TLS configuration
 * 
 * M. Print
 *    - Queue management
 *    - Priority system
 *    - Job processing
 *    - Status tracking
 * 
 * N. Resources
 *    - Memory limits
 *    - Buffer sizes
 *    - Queue capacities
 *    - System boundaries
 * 
 * O. OIDC
 *    - Identity providers
 *    - Token management
 *    - Authentication flows
 *    - Session handling
 */

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

/*
 * Generate complete default configuration
 * 
 * Creates a new configuration file with secure defaults for all subsystems
 * in strict A-O order. Each section is generated with appropriate defaults
 * considering:
 * 
 * - Security First
 *   • Conservative file permissions
 *   • Secure network settings
 *   • Resource limits to prevent DoS
 *   • Separate ports for services
 * 
 * - Operational Safety
 *   • Reasonable resource limits
 *   • Safe default paths
 *   • Controlled access patterns
 *   • Clear upgrade paths
 * 
 * - Environment Awareness
 *   • Environment variable support
 *   • System-specific paths
 *   • Resource availability
 *   • Platform compatibility
 * 
 * @param config_path Path where configuration should be created
 */
void create_default_config(const char* config_path);

#endif /* CONFIG_DEFAULTS_H */