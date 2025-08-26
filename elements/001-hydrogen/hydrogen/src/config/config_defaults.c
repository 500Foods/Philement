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
#include "../hydrogen.h"

// Configuration includes
#include "config.h"
#include "config_server.h"
#include "config_network.h"
#include "config_webserver.h"
#include "config_api.h"
#include "config_swagger.h"
#include "config_websocket.h"
#include "config_terminal.h"
#include "config_resources.h"
#include "config_logging.h"
#include "config_databases.h"
#include "config_mdns_server.h"
#include "config_mdns_client.h"
#include "config_mail_relay.h"
#include "config_print.h"
#include "config_oidc.h"
#include "config_notify.h"

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
        log_this("Config-Defaults", "Cannot initialize NULL config", LOG_LEVEL_ERROR);
        return false;
    }

    // Zero out the entire structure first
    memset(config, 0, sizeof(AppConfig));

    // A. Server Configuration Defaults (Section A)
    config->server.server_name = strdup("Philement/hydrogen");
    config->server.exec_file = NULL; // Will be set by caller if needed
    config->server.config_file = NULL; // Will be set by caller if needed
    config->server.log_file = strdup("/var/log/hydrogen/hydrogen.log");
    config->server.payload_key = strdup("${env.PAYLOAD_KEY}");
    config->server.startup_delay = 5;

    if (!config->server.server_name || !config->server.log_file || !config->server.payload_key) {
        log_this("Config-Defaults", "Failed to allocate server config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // B. Network Configuration Defaults (Section B)
    config->network.max_interfaces = 16;
    config->network.max_ips_per_interface = 32;
    config->network.max_interface_name_length = 32;
    config->network.max_ip_address_length = 50;
    config->network.start_port = 1024;
    config->network.end_port = 65535;

    // Allocate initial reserved ports array
    config->network.reserved_ports = malloc(16 * sizeof(int));
    if (!config->network.reserved_ports) {
        log_this("Config-Defaults", "Failed to allocate network reserved ports", LOG_LEVEL_ERROR);
        return false;
    }
    config->network.reserved_ports_count = 0;

    // Set up default interface availability ("all" enabled)
    config->network.available_interfaces = malloc(sizeof(*config->network.available_interfaces));
    if (!config->network.available_interfaces) {
        log_this("Config-Defaults", "Failed to allocate network interfaces", LOG_LEVEL_ERROR);
        return false;
    }
    config->network.available_interfaces[0].interface_name = strdup("all");
    if (!config->network.available_interfaces[0].interface_name) {
        log_this("Config-Defaults", "Failed to allocate network interface name", LOG_LEVEL_ERROR);
        return false;
    }
    config->network.available_interfaces[0].available = true;
    config->network.available_interfaces_count = 1;

    // E. WebServer Configuration Defaults (Section E)
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

    if (!config->webserver.web_root || !config->webserver.upload_path || !config->webserver.upload_dir) {
        log_this("Config-Defaults", "Failed to allocate webserver config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // F. API Configuration Defaults (Section F)
    config->api.enabled = true;
    config->api.prefix = strdup("/api");
    config->api.jwt_secret = strdup("${env.JWT_SECRET}");

    if (!config->api.prefix || !config->api.jwt_secret) {
        log_this("Config-Defaults", "Failed to allocate API config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // G. Swagger Configuration Defaults (Section G)
    config->swagger.enabled = true;
    config->swagger.prefix = strdup("/apidocs");
    config->swagger.payload_available = false;

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

    if (!config->swagger.prefix || !config->swagger.metadata.title ||
        !config->swagger.metadata.description || !config->swagger.metadata.version ||
        !config->swagger.ui_options.doc_expansion || !config->swagger.ui_options.syntax_highlight_theme) {
        log_this("Config-Defaults", "Failed to allocate Swagger config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // H. WebSocket Configuration Defaults (Section H)
    config->websocket.enabled = false;
    config->websocket.enable_ipv6 = false;
    config->websocket.lib_log_level = 2;
    config->websocket.port = 5001;
    config->websocket.max_message_size = 2048;

    // Connection timeouts
    config->websocket.connection_timeouts.shutdown_wait_seconds = 2;
    config->websocket.connection_timeouts.service_loop_delay_ms = 50;
    config->websocket.connection_timeouts.connection_cleanup_ms = 500;
    config->websocket.connection_timeouts.exit_wait_seconds = 3;

    // String fields
    config->websocket.protocol = strdup("hydrogen");
    config->websocket.key = strdup("${env.WEBSOCKET_KEY}");

    if (!config->websocket.protocol || !config->websocket.key) {
        log_this("Config-Defaults", "Failed to allocate WebSocket config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // I. Terminal Configuration Defaults (Section I)
    config->terminal.enabled = true;
    config->terminal.max_sessions = 4;
    config->terminal.idle_timeout_seconds = 300; // 5 minutes

    // String fields
    config->terminal.web_path = strdup("/terminal");
    config->terminal.shell_command = strdup("/bin/bash");

    if (!config->terminal.web_path || !config->terminal.shell_command) {
        log_this("Config-Defaults", "Failed to allocate terminal config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // N. Resources Configuration Defaults (Section N)
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

    // D. Logging Configuration Defaults (Section D)
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

    // C. Database Configuration Defaults (Section C)
    config->databases.default_workers = 1;
    config->databases.connection_count = 1; // Start with 1 connection (Acuranzo)

    // Initialize Acuranzo connection (first connection)
    DatabaseConnection* acuranzo = &config->databases.connections[0];
    memset(acuranzo, 0, sizeof(DatabaseConnection));
    acuranzo->enabled = true;
    acuranzo->workers = 1;
    acuranzo->name = strdup("Acuranzo");
    acuranzo->connection_name = strdup("Acuranzo");
    acuranzo->type = strdup("${env.ACURANZO_DB_TYPE}");
    acuranzo->database = strdup("${env.ACURANZO_DATABASE}");
    acuranzo->host = strdup("${env.ACURANZO_DB_HOST}");
    acuranzo->port = strdup("${env.ACURANZO_DB_PORT}");
    acuranzo->user = strdup("${env.ACURANZO_DB_USER}");
    acuranzo->pass = strdup("${env.ACURANZO_DB_PASS}");

    if (!acuranzo->name || !acuranzo->connection_name || !acuranzo->type ||
        !acuranzo->database || !acuranzo->host || !acuranzo->port ||
        !acuranzo->user || !acuranzo->pass) {
        log_this("Config-Defaults", "Failed to allocate database config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize remaining connections to NULL state
    for (int i = 1; i < 5; i++) {
        memset(&config->databases.connections[i], 0, sizeof(DatabaseConnection));
    }

    // J. mDNS Server Configuration Defaults (Section J)
    config->mdns_server.enabled = false;
    config->mdns_server.enable_ipv6 = false;
    config->mdns_server.device_id = strdup("hydrogen-server");
    config->mdns_server.friendly_name = strdup("Hydrogen Server");
    config->mdns_server.model = strdup("Hydrogen");
    config->mdns_server.manufacturer = strdup("Philement");
    config->mdns_server.version = strdup("1.0.0");
    config->mdns_server.services = NULL;
    config->mdns_server.num_services = 0;

    if (!config->mdns_server.device_id || !config->mdns_server.friendly_name ||
        !config->mdns_server.model || !config->mdns_server.manufacturer ||
        !config->mdns_server.version) {
        log_this("Config-Defaults", "Failed to allocate mDNS server config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // K. mDNS Client Configuration Defaults (Section K)
    config->mdns_client.enabled = false;
    config->mdns_client.enable_ipv6 = false;
    config->mdns_client.scan_interval = 30;
    config->mdns_client.max_services = 100;
    config->mdns_client.retry_count = 3;
    config->mdns_client.health_check_enabled = true;
    config->mdns_client.health_check_interval = 60;
    config->mdns_client.service_types = NULL;
    config->mdns_client.num_service_types = 0;

    // L. Mail Relay Configuration Defaults (Section L)
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

    if (!config->mail_relay.Servers[0].Host || !config->mail_relay.Servers[0].Port) {
        log_this("Config-Defaults", "Failed to allocate mail relay config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize remaining servers to NULL state
    for (int i = 1; i < MAX_OUTBOUND_SERVERS; i++) {
        memset(&config->mail_relay.Servers[i], 0, sizeof(OutboundServer));
    }

    // M. Print Configuration Defaults (Section M)
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

    // O. OIDC Configuration Defaults (Section O)
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

    if (!config->oidc.redirect_uri || !config->oidc.scope ||
        !config->oidc.auth_method || !config->oidc.keys.storage_path ||
        !config->oidc.tokens.signing_alg) {
        log_this("Config-Defaults", "Failed to allocate OIDC config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    // P. Notify Configuration Defaults (Section P)
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

    if (!config->notify.notifier || !config->notify.smtp.host ||
        !config->notify.smtp.from_address) {
        log_this("Config-Defaults", "Failed to allocate notify config defaults", LOG_LEVEL_ERROR);
        return false;
    }

    log_this("Config-Defaults", "Successfully initialized configuration defaults", LOG_LEVEL_STATE);
    return true;
}
