/*
 * Configuration Defaults Implementation
 *
 * This file centralizes default configuration values for the Hydrogen server.
 * It provides a starting point for configuration initialization that can be
 * overridden by JSON configuration files and environment variables.
 *
 * This is a simplified initial implementation that demonstrates the concept.
 * Additional config sections can be added as their structures are confirmed.
 */

// Global includes
#include <src/hydrogen.h>

// Configuration includes
#include "config.h"

/**
 * Initialize app_config with configuration defaults
 *
 * This function sets up the AppConfig structure with secure baseline defaults
 * for multiple configuration sections (A, B, E, F). Additional sections can be
 * added as their structures are confirmed and default values are identified.
 *
 * Currently implemented sections:
 * A. Server - Basic server identification and logging
 * B. Network - Network interface and port configuration
 * C. Database - Database connection settings
 * D. Logging - Logging destinations and levels
 * E. WebServer - HTTP server settings and paths
 * F. API - REST API configuration and JWT settings
 * G. Swagger - API documentation and UI configuration
 * H. WebSocket - WebSocket server and connection settings
 * I. Terminal - Terminal access and session management
 * J. mDNS Server - Service discovery server
 * K. mDNS Client - Service discovery client
 * L. Mail Relay - Email relay configuration
 * M. Print - Print server configuration
 * N. Resources - System resource limits and monitoring
 * O. OIDC - OpenID Connect authentication
 * P. Notify - Notification system
 *
 * @param config Pointer to the AppConfig structure to initialize
 * @return true on success, false on failure (memory allocation errors)
 */
bool initialize_config_defaults(AppConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Cannot initialize NULL config", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Zero out the entire structure first
    memset(config, 0, sizeof(AppConfig));

    initialize_config_defaults_server(config);
    initialize_config_defaults_network(config);
    initialize_config_defaults_database(config);
    initialize_config_defaults_logging(config);
    initialize_config_defaults_webserver(config);
    initialize_config_defaults_api(config);
    initialize_config_defaults_swagger(config);
    initialize_config_defaults_websocket(config);
    initialize_config_defaults_terminal(config);
    initialize_config_defaults_mdns_server(config);
    initialize_config_defaults_mdns_client(config);
    initialize_config_defaults_mail_relay(config);
    initialize_config_defaults_print(config);
    initialize_config_defaults_resources(config);
    initialize_config_defaults_oidc(config);
    initialize_config_defaults_notify(config);

    log_this(SR_CONFIG, "― Successfully initialized configuration defaults", LOG_LEVEL_DEBUG, 0);
    return true;
}

// A. Server Configuration Defaults
void initialize_config_defaults_server(AppConfig* config) {
    if (config) {
        config->server.server_name = strdup("Philement/hydrogen");
        config->server.exec_file = NULL;
        config->server.config_file = NULL;
        config->server.log_file = strdup("/var/log/hydrogen/hydrogen.log");
        config->server.startup_delay = 5;
        config->server.payload_key = process_env_variable_string("${env.PAYLOAD_KEY}");

        log_this(SR_CONFIG, "――― Applied config defaults for Server", LOG_LEVEL_DEBUG, 0);
    }
}

// B. Network Configuration Defaults 
void initialize_config_defaults_network(AppConfig* config) {
    if (config) {
        config->network.max_interfaces = 16;
        config->network.max_ips_per_interface = 32;
        config->network.max_interface_name_length = 32;
        config->network.max_ip_address_length = 50;
        config->network.start_port = 1024;
        config->network.end_port = 65535;

        // Allocate initial reserved ports array
        config->network.reserved_ports = malloc(16 * sizeof(int));
        config->network.reserved_ports_count = 0;

        // Set up default interface availability ("all" enabled)
        config->network.available_interfaces = malloc(sizeof(*config->network.available_interfaces));
        config->network.available_interfaces[0].interface_name = strdup("all");
        config->network.available_interfaces[0].available = true;
        config->network.available_interfaces_count = 1;
        
        log_this(SR_CONFIG, "――― Applied config defaults for Network", LOG_LEVEL_DEBUG, 0);
    }
}

// C. Database Configuration Defaults
void initialize_config_defaults_database(AppConfig* config) {
    if (config) {
        // No default databases configured - all database configuration comes from JSON config files
        config->databases.connection_count = 0;       // No databases configured by default

        // Set default queue scaling configurations
        // Slow queue: conservative scaling
        config->databases.default_queues.slow.start = 1;
        config->databases.default_queues.slow.min = 1;
        config->databases.default_queues.slow.max = 4;
        config->databases.default_queues.slow.up = 10;
        config->databases.default_queues.slow.down = 2;
        config->databases.default_queues.slow.inactivity = 300;

        // Medium queue: moderate scaling
        config->databases.default_queues.medium.start = 2;
        config->databases.default_queues.medium.min = 1;
        config->databases.default_queues.medium.max = 8;
        config->databases.default_queues.medium.up = 15;
        config->databases.default_queues.medium.down = 3;
        config->databases.default_queues.medium.inactivity = 240;

        // Fast queue: aggressive scaling
        config->databases.default_queues.fast.start = 4;
        config->databases.default_queues.fast.min = 2;
        config->databases.default_queues.fast.max = 16;
        config->databases.default_queues.fast.up = 20;
        config->databases.default_queues.fast.down = 5;
        config->databases.default_queues.fast.inactivity = 180;

        // Cache queue: minimal scaling
        config->databases.default_queues.cache.start = 1;
        config->databases.default_queues.cache.min = 1;
        config->databases.default_queues.cache.max = 4;
        config->databases.default_queues.cache.up = 5;
        config->databases.default_queues.cache.down = 1;
        config->databases.default_queues.cache.inactivity = 600;

        // Set default prepared statement cache size for all connections (redundant, memset will overwrite)
        // for (int i = 0; i < 5; i++) {
        //     config->databases.connections[i].prepared_statement_cache_size = 1000;
        // }

        // Clear all database connection slots
        for (int i = 0; i < 5; i++) {
            memset(&config->databases.connections[i], 0, sizeof(DatabaseConnection));
            // Restore the cache size after memset
            config->databases.connections[i].prepared_statement_cache_size = 1000;
        }

        log_this(SR_CONFIG, "――― Applied config defaults for Database (no default connections)", LOG_LEVEL_DEBUG, 0);
    }
}

// D. Logging Configuration Defaults 
void initialize_config_defaults_logging(AppConfig* config) {
    if (config) {
        // Initialize with basic defaults - can be expanded to include full logging levels and subsystems
        config->logging.level_count = 0;
        config->logging.levels = NULL;

        // Console logging defaults
        config->logging.console.enabled = true;
        config->logging.console.default_level = LOG_LEVEL_STATE;
        config->logging.console.subsystem_count = 0;
        config->logging.console.subsystems = NULL;

        // File logging defaults
        config->logging.file.enabled = true;
        config->logging.file.default_level = LOG_LEVEL_DEBUG;
        config->logging.file.subsystem_count = 0;
        config->logging.file.subsystems = NULL;

        // Database logging defaults (disabled by default)
        config->logging.database.enabled = false;
        config->logging.database.default_level = LOG_LEVEL_ERROR;
        config->logging.database.subsystem_count = 0;
        config->logging.database.subsystems = NULL;

        // Notification logging defaults (disabled by default)
        config->logging.notify.enabled = false;
        config->logging.notify.default_level = LOG_LEVEL_ERROR;
        config->logging.notify.subsystem_count = 0;
        config->logging.notify.subsystems = NULL;
        
        log_this(SR_CONFIG, "――― Applied config defaults for Logging", LOG_LEVEL_DEBUG, 0);
    }
}

// E. WebServer Configuration Defaults
void initialize_config_defaults_webserver(AppConfig* config) {
    if (config) {
        config->webserver.enable_ipv4 = true;
        config->webserver.enable_ipv6 = false;
        config->webserver.port = 5000;
        config->webserver.web_root = strdup("/tmp/hydrogen");
        config->webserver.upload_path = strdup("/upload");
        config->webserver.upload_dir = strdup("/tmp/hydrogen");
        config->webserver.max_upload_size = 100 * 1024 * 1024; // 100MB
        config->webserver.thread_pool_size = 20;
        config->webserver.max_connections = 200;
        config->webserver.max_connections_per_ip = 100;
        config->webserver.connection_timeout = 60;

        // NEW: Global CORS default for WebServer
        config->webserver.cors_origin = strdup("*");  // Allow all origins as default

        log_this(SR_CONFIG, "――― Applied config defaults for Webserver", LOG_LEVEL_DEBUG, 0);
    }
}

// F. API Configuration Defaults
void initialize_config_defaults_api(AppConfig* config) {
    if (config) {
        config->api.enabled = true;
        config->api.prefix = strdup("/api");
        config->api.jwt_secret = strdup("${env.JWT_SECRET}");

        // NEW: CORS defaults for API
        config->api.cors_origin = strdup("*");  // Allow all origins as default

        log_this(SR_CONFIG, "――― Applied config defaults for API", LOG_LEVEL_DEBUG, 0);
    }
}

// G. Swagger Configuration Defaults
void initialize_config_defaults_swagger(AppConfig* config) {
    if (config) {
        config->swagger.enabled = true;
        config->swagger.prefix = strdup("/apidocs");
        config->swagger.payload_available = false;

        // NEW: WebRoot defaults
        config->swagger.webroot = strdup("PAYLOAD:/swagger");
        config->swagger.cors_origin = strdup("*");  // Allow all origins by default
        config->swagger.index_page = strdup("swagger.html");  // Configurable index page

        // Metadata defaults
        config->swagger.metadata.title = strdup("Hydrogen API");
        config->swagger.metadata.description = strdup("Hydrogen Server API");
        config->swagger.metadata.version = strdup("1.0.0");

        // Contact and license are NULL by default
        config->swagger.metadata.contact.name = NULL;
        config->swagger.metadata.contact.email = NULL;
        config->swagger.metadata.contact.url = NULL;
        config->swagger.metadata.license.name = NULL;
        config->swagger.metadata.license.url = NULL;

        // UI options defaults
        config->swagger.ui_options.try_it_enabled = true;
        config->swagger.ui_options.always_expanded = false;
        config->swagger.ui_options.display_operation_id = false;
        config->swagger.ui_options.default_models_expand_depth = 1;
        config->swagger.ui_options.default_model_expand_depth = 1;
        config->swagger.ui_options.show_extensions = false;
        config->swagger.ui_options.show_common_extensions = true;
        config->swagger.ui_options.doc_expansion = strdup("list");
        config->swagger.ui_options.syntax_highlight_theme = strdup("agate");

        log_this(SR_CONFIG, "――― Applied config defaults for Swagger", LOG_LEVEL_DEBUG, 0);
    }
}

// H. WebSocket Configuration Defaults
void initialize_config_defaults_websocket(AppConfig* config) {
    if (config) {
        config->websocket.enable_ipv4 = false;
        config->websocket.enable_ipv6 = false;
        config->websocket.lib_log_level = 2;
        config->websocket.port = 5001;
        config->websocket.max_message_size = 8192;  // 8KB to accommodate terminal output with JSON overhead

        // Connection timeouts
        config->websocket.connection_timeouts.shutdown_wait_seconds = 2;
        config->websocket.connection_timeouts.service_loop_delay_ms = 50;
        config->websocket.connection_timeouts.connection_cleanup_ms = 500;
        config->websocket.connection_timeouts.exit_wait_seconds = 3;

        // String fields
        config->websocket.protocol = strdup("hydrogen");
        config->websocket.key = strdup("${env.WEBSOCKET_KEY}");
        
        log_this(SR_CONFIG, "――― Applied config defaults for Websockets", LOG_LEVEL_DEBUG, 0);
    }
}

// I. Terminal Configuration Defaults
void initialize_config_defaults_terminal(AppConfig* config) {
    if (config) {
        config->terminal.enabled = true;
        config->terminal.max_sessions = 4;
        config->terminal.idle_timeout_seconds = 300; // 5 minutes
        config->terminal.buffer_size = 1024;  // 1KB PTY read buffer (conservative for WebSocket)

        // String fields
        config->terminal.web_path = strdup("/terminal");
        config->terminal.shell_command = strdup("/bin/zsh");  // Default to zsh

        // NEW: WebRoot defaults for terminal
        config->terminal.webroot = strdup("PAYLOAD:/terminal");
        config->terminal.cors_origin = strdup("*");  // Allow all origins by default
        config->terminal.index_page = strdup("terminal.html");  // Configurable index page

        log_this(SR_CONFIG, "――― Applied config defaults for Terminal", LOG_LEVEL_DEBUG, 0);
    }
}

// J. mDNS Server Configuration Defaults 
void initialize_config_defaults_mdns_server(AppConfig* config) {
    if (config) {
        config->mdns_server.enable_ipv4 = false;
        config->mdns_server.enable_ipv6 = false;
        config->mdns_server.device_id = strdup("hydrogen-server");
        config->mdns_server.friendly_name = strdup("Hydrogen Server");
        config->mdns_server.model = strdup("Hydrogen");
        config->mdns_server.manufacturer = strdup("Philement");
        config->mdns_server.version = strdup("1.0.0");
        config->mdns_server.services = NULL;
        config->mdns_server.num_services = 0;
        config->mdns_server.retry_count = 1;
            
        log_this(SR_CONFIG, "――― Applied config defaults for mDNS Server", LOG_LEVEL_DEBUG, 0);
    }
}

// K. mDNS Client Configuration Defaults 
void initialize_config_defaults_mdns_client(AppConfig* config) {
    if (config) {
        config->mdns_client.enable_ipv4 = false;
        config->mdns_client.enable_ipv6 = false;
        config->mdns_client.scan_interval = 30;
        config->mdns_client.max_services = 100;
        config->mdns_client.retry_count = 3;
        config->mdns_client.health_check_enabled = true;
        config->mdns_client.health_check_interval = 60;
        config->mdns_client.service_types = NULL;
        config->mdns_client.num_service_types = 0;
        
        log_this(SR_CONFIG, "――― Applied config defaults for mDNS Client", LOG_LEVEL_DEBUG, 0);
    }
}

// L. Mail Relay Configuration Defaults
void initialize_config_defaults_mail_relay(AppConfig* config) {
    if (config) {
        config->mail_relay.Enabled = false;
        config->mail_relay.ListenPort = 25; // Standard SMTP port
        config->mail_relay.Workers = 2;

        // Queue configuration
        config->mail_relay.Queue.MaxQueueSize = 1000;
        config->mail_relay.Queue.RetryAttempts = 3;
        config->mail_relay.Queue.RetryDelaySeconds = 300; // 5 minutes

        // Default outbound server (first one)
        config->mail_relay.OutboundServerCount = 1;
        config->mail_relay.Servers[0].Host = strdup("localhost");
        config->mail_relay.Servers[0].Port = strdup("587");
        config->mail_relay.Servers[0].Username = NULL;
        config->mail_relay.Servers[0].Password = NULL;
        config->mail_relay.Servers[0].UseTLS = true;

        // Initialize remaining servers to NULL state
        for (int i = 1; i < MAX_OUTBOUND_SERVERS; i++) {
            memset(&config->mail_relay.Servers[i], 0, sizeof(OutboundServer));
        }
        
        log_this(SR_CONFIG, "――― Applied config defaults for Mail Relay", LOG_LEVEL_DEBUG, 0);
    }
}

// M. Print Configuration Defaults
void initialize_config_defaults_print(AppConfig* config) {
    if (config) {
        config->print.enabled = false;
        config->print.max_queued_jobs = 100;
        config->print.max_concurrent_jobs = 2;

        // Priority configuration
        config->print.priorities.default_priority = DEFAULT_PRIORITY_DEFAULT;
        config->print.priorities.emergency_priority = DEFAULT_PRIORITY_EMERGENCY;
        config->print.priorities.maintenance_priority = DEFAULT_PRIORITY_MAINTENANCE;
        config->print.priorities.system_priority = DEFAULT_PRIORITY_SYSTEM;

        // Timeout configuration
        config->print.timeouts.shutdown_wait_ms = 30000; // 30 seconds
        config->print.timeouts.job_processing_timeout_ms = 3600000; // 1 hour

        // Buffer configuration
        config->print.buffers.job_message_size = 16384; // 16KB
        config->print.buffers.status_message_size = 2048; // 2KB

        // Motion control configuration
        config->print.motion.max_speed = 1000.0;
        config->print.motion.max_speed_xy = 750.0;
        config->print.motion.max_speed_z = 100.0;
        config->print.motion.max_speed_travel = 1500.0;
        config->print.motion.acceleration = 5000.0;
        config->print.motion.z_acceleration = 1000.0;
        config->print.motion.e_acceleration = 10000.0;
        config->print.motion.jerk = 10.0;
        config->print.motion.smooth_moves = true;
        
        log_this(SR_CONFIG, "――― Applied config defaults for Print", LOG_LEVEL_DEBUG, 0);
    }
}

// N. Resources Configuration Defaults
void initialize_config_defaults_resources(AppConfig* config) {
    if (config) {
        config->resources.max_memory_mb = 1024; // 1GB
        config->resources.max_buffer_size = 1048576; // 1MB
        config->resources.min_buffer_size = 1024; // 1KB
        config->resources.max_queue_size = 10000;
        config->resources.max_queue_memory_mb = 100; // 100MB
        config->resources.max_queue_blocks = 1000;
        config->resources.queue_timeout_ms = 5000; // 5 seconds
        config->resources.post_processor_buffer_size = 65536; // 64KB
        config->resources.min_threads = 1;
        config->resources.max_threads = 64;
        config->resources.thread_stack_size = 1048576; // 1MB
        config->resources.max_open_files = 1024;
        config->resources.max_file_size_mb = 100; // 100MB
        config->resources.max_log_size_mb = 50; // 50MB
        config->resources.enforce_limits = true;
        config->resources.log_usage = false;
        config->resources.check_interval_ms = 60000; // 1 minute
        
        log_this(SR_CONFIG, "――― Applied config defaults for Resources", LOG_LEVEL_DEBUG, 0);
    }
}

// O. OIDC Configuration Defaults (Section O)
void initialize_config_defaults_oidc(AppConfig* config) {
    if (config) {
        config->oidc.enabled = false;
        config->oidc.issuer = NULL;
        config->oidc.client_id = NULL;
        config->oidc.client_secret = NULL;
        config->oidc.redirect_uri = strdup("http://localhost:8080/auth/callback");
        config->oidc.port = 8080;
        config->oidc.auth_method = strdup("client_secret_basic");
        config->oidc.scope = strdup("openid profile email");
        config->oidc.verify_ssl = true;

        // Endpoints configuration
        memset(&config->oidc.endpoints, 0, sizeof(OIDCEndpointsConfig));

        // Keys configuration
        config->oidc.keys.signing_key = NULL;
        config->oidc.keys.encryption_key = NULL;
        config->oidc.keys.jwks_uri = NULL;
        config->oidc.keys.storage_path = strdup("/var/lib/hydrogen/oidc");
        config->oidc.keys.encryption_enabled = false;
        config->oidc.keys.rotation_interval_days = 90;

        // Tokens configuration
        config->oidc.tokens.access_token_lifetime = 3600; // 1 hour
        config->oidc.tokens.refresh_token_lifetime = 86400; // 24 hours
        config->oidc.tokens.id_token_lifetime = 3600; // 1 hour
        config->oidc.tokens.signing_alg = strdup("RS256");
        config->oidc.tokens.encryption_alg = NULL;
        
        log_this(SR_CONFIG, "――― Applied config defaults for OIDC", LOG_LEVEL_DEBUG, 0);
    }
}

// P. Notify Configuration Defaults 
void initialize_config_defaults_notify(AppConfig* config) {
    if (config) {
        config->notify.enabled = false;
        config->notify.notifier = strdup("SMTP");

        // SMTP configuration
        config->notify.smtp.host = strdup("localhost");
        config->notify.smtp.port = 587;
        config->notify.smtp.username = NULL;
        config->notify.smtp.password = NULL;
        config->notify.smtp.use_tls = true;
        config->notify.smtp.timeout = 30;
        config->notify.smtp.max_retries = 3;
        config->notify.smtp.from_address = strdup("hydrogen@localhost");
        
        log_this(SR_CONFIG, "――― Applied config defaults for Notify", LOG_LEVEL_DEBUG, 0);
    }
}
