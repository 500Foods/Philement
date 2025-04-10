/*
 * Default configuration generation with secure baselines
 * 
 * Configuration sections are organized in strict A-P order:
 * A. Server
 * B. System
 * C. Network
 * D. Database
 * E. Logging
 * F. WebServer
 * G. API
 * H. Swagger
 * I. WebSocket
 * J. Terminal
 * K. mDNS Server
 * L. mDNS Client
 * M. Mail Relay
 * N. System Resources
 * O. OIDC
 * P. Notify
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

// Project headers
#include "config_defaults.h"
#include "config.h"
#include "../logging/logging.h"

/*
 * A. Generate default server configuration
 */
static json_t* create_default_server_config(void) {
    json_t* server = json_object();
    if (!server) {
        log_this("Configuration", "Failed to create server config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Core Identity
    json_object_set_new(server, "ServerName", json_string("${env.HYDROGEN_SERVER_NAME:-Philement/hydrogen}"));
    json_object_set_new(server, "Version", json_string(VERSION));
    json_object_set_new(server, "BuildType", json_string("${env.BUILD_TYPE:-release}"));

    // Essential Paths
    json_object_set_new(server, "LogFile", json_string("${env.HYDROGEN_LOG_PATH:-/var/log/hydrogen.log}"));
    json_object_set_new(server, "ConfigDir", json_string("${env.HYDROGEN_CONFIG_DIR:-/etc/hydrogen}"));
    json_object_set_new(server, "DataDir", json_string("${env.HYDROGEN_DATA_DIR:-/var/lib/hydrogen}"));
    json_object_set_new(server, "TempDir", json_string("${env.HYDROGEN_TEMP_DIR:-/tmp/hydrogen}"));

    // Security
    json_object_set_new(server, "PayloadKey", json_string("${env.HYDROGEN_PAYLOAD_KEY}"));
    json_object_set_new(server, "FileMode", json_integer(0640));
    json_object_set_new(server, "DirMode", json_integer(0750));

    // Runtime Behavior
    json_t* startup = json_object();
    json_object_set_new(startup, "DelayMs", json_string("${env.HYDROGEN_STARTUP_DELAY:-5000}"));
    json_object_set_new(startup, "MaxAttempts", json_integer(3));
    json_object_set_new(startup, "RetryDelayMs", json_integer(1000));
    json_object_set_new(server, "Startup", startup);

    json_t* shutdown = json_object();
    json_object_set_new(shutdown, "GracePeriodMs", json_integer(5000));
    json_object_set_new(shutdown, "ForceTimeoutMs", json_integer(10000));
    json_object_set_new(server, "Shutdown", shutdown);

    return server;
}

/*
 * B. Generate default system configuration
 */
static json_t* create_default_system_config(void) {
    json_t* system = json_object();
    if (!system) {
        log_this("Configuration", "Failed to create system config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(system, "Hostname", json_string("${env.HOSTNAME:-hydrogen}"));
    json_object_set_new(system, "ProcessId", json_integer(0));  // Set at runtime
    json_object_set_new(system, "StartTime", json_integer(0));  // Set at runtime

    json_t* limits = json_object();
    json_object_set_new(limits, "MaxCPUPercent", json_integer(80));
    json_object_set_new(limits, "MaxMemoryMB", json_integer(1024));
    json_object_set_new(limits, "MaxDiskUsagePercent", json_integer(90));
    json_object_set_new(system, "Limits", limits);

    json_t* monitoring = json_object();
    json_object_set_new(monitoring, "Enabled", json_boolean(true));
    json_object_set_new(monitoring, "IntervalMs", json_integer(5000));
    json_object_set_new(system, "Monitoring", monitoring);

    return system;
}

/*
 * C. Generate default network configuration
 */
static json_t* create_default_network_config(void) {
    json_t* network = json_object();
    if (!network) {
        log_this("Configuration", "Failed to create network config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_t* interfaces = json_object();
    json_object_set_new(interfaces, "MaxInterfaces", json_integer(DEFAULT_MAX_INTERFACES));
    json_object_set_new(interfaces, "MaxIPsPerInterface", json_integer(DEFAULT_MAX_IPS_PER_INTERFACE));
    json_object_set_new(interfaces, "MaxInterfaceNameLength", json_integer(DEFAULT_MAX_INTERFACE_NAME_LENGTH));
    json_object_set_new(interfaces, "MaxIPAddressLength", json_integer(DEFAULT_MAX_IP_ADDRESS_LENGTH));
    json_object_set_new(network, "Interfaces", interfaces);

    json_t* ports = json_object();
    json_object_set_new(ports, "StartPort", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(ports, "EndPort", json_integer(65535));
    
    json_t* reserved = json_array();
    json_array_append_new(reserved, json_integer(22));   // SSH
    json_array_append_new(reserved, json_integer(80));   // HTTP
    json_array_append_new(reserved, json_integer(443));  // HTTPS
    json_object_set_new(ports, "ReservedPorts", reserved);
    
    json_object_set_new(network, "PortAllocation", ports);

    return network;
}

/*
 * D. Generate default database configuration
 */
static json_t* create_default_database_config(void) {
    json_t* database = json_object();
    if (!database) {
        log_this("Configuration", "Failed to create database config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_t* postgres = json_object();
    json_object_set_new(postgres, "Host", json_string("${env.POSTGRES_HOST:-localhost}"));
    json_object_set_new(postgres, "Port", json_integer(5432));
    json_object_set_new(postgres, "Database", json_string("${env.POSTGRES_DB:-hydrogen}"));
    json_object_set_new(postgres, "User", json_string("${env.POSTGRES_USER:-hydrogen}"));
    json_object_set_new(postgres, "Password", json_string("${env.POSTGRES_PASSWORD}"));
    json_object_set_new(postgres, "SSLMode", json_string("prefer"));
    json_object_set_new(database, "Postgres", postgres);

    json_t* pool = json_object();
    json_object_set_new(pool, "MinConnections", json_integer(2));
    json_object_set_new(pool, "MaxConnections", json_integer(10));
    json_object_set_new(pool, "ConnectionTimeout", json_integer(30));
    json_object_set_new(database, "Pool", pool);

    return database;
}

/*
 * E. Generate default logging configuration
 */
static json_t* create_default_logging_config(void) {
    json_t* logging = json_object();
    if (!logging) {
        log_this("Configuration", "Failed to create logging config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_t* levels = json_array();
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "TRACE", "Value", 0));
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "DEBUG", "Value", 1));
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "STATE", "Value", 2));
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "ALERT", "Value", 3));
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "ERROR", "Value", 4));
    json_array_append_new(levels, json_pack("{s:s, s:i}", "Name", "FATAL", "Value", 5));
    json_object_set_new(logging, "LogLevels", levels);

    json_t* console = json_object();
    json_object_set_new(console, "Enabled", json_boolean(true));
    json_object_set_new(console, "Level", json_string("DEBUG"));
    json_object_set_new(logging, "Console", console);

    json_t* file = json_object();
    json_object_set_new(file, "Enabled", json_boolean(true));
    json_object_set_new(file, "Level", json_string("STATE"));
    json_object_set_new(file, "Path", json_string("${env.LOG_FILE:-/var/log/hydrogen.log}"));
    json_object_set_new(file, "MaxSize", json_integer(10 * 1024 * 1024));  // 10MB
    json_object_set_new(file, "MaxFiles", json_integer(5));
    json_object_set_new(logging, "File", file);

    return logging;
}

/*
 * F. Generate default web server configuration
 */
static json_t* create_default_web_config(void) {
    json_t* web = json_object();
    if (!web) {
        log_this("Configuration", "Failed to create web server config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(web, "EnableIPv4", json_boolean(true));
    json_object_set_new(web, "EnableIPv6", json_boolean(true));
    json_object_set_new(web, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(web, "WebRoot", json_string(DEFAULT_WEB_ROOT));
    json_object_set_new(web, "UploadPath", json_string(DEFAULT_UPLOAD_PATH));
    json_object_set_new(web, "UploadDir", json_string(DEFAULT_UPLOAD_DIR));
    json_object_set_new(web, "MaxUploadSize", json_integer(DEFAULT_MAX_UPLOAD_SIZE));
    json_object_set_new(web, "ThreadPoolSize", json_integer(4));
    json_object_set_new(web, "MaxConnections", json_integer(100));
    json_object_set_new(web, "ConnectionTimeout", json_integer(30));

    return web;
}

/*
 * G. Generate default API configuration
 */
static json_t* create_default_api_config(void) {
    json_t* api = json_object();
    if (!api) {
        log_this("Configuration", "Failed to create API config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(api, "Enabled", json_boolean(true));
    json_object_set_new(api, "Prefix", json_string("/api"));
    json_object_set_new(api, "JWTSecret", json_string("${env.JWT_SECRET}"));
    json_object_set_new(api, "TokenExpiration", json_integer(3600));  // 1 hour

    return api;
}

/*
 * H. Generate default Swagger configuration
 */
static json_t* create_default_swagger_config(void) {
    json_t* swagger = json_object();
    if (!swagger) {
        log_this("Configuration", "Failed to create Swagger config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(swagger, "Enabled", json_boolean(true));
    json_object_set_new(swagger, "Prefix", json_string("/apidocs"));

    json_t* ui = json_object();
    json_object_set_new(ui, "TryItEnabled", json_boolean(true));
    json_object_set_new(swagger, "UIOptions", ui);

    json_t* metadata = json_object();
    json_object_set_new(metadata, "Title", json_string("Hydrogen API"));
    json_object_set_new(metadata, "Version", json_string(VERSION));
    json_object_set_new(swagger, "Metadata", metadata);

    return swagger;
}

/*
 * I. Generate default WebSocket configuration
 */
static json_t* create_default_websocket_config(void) {
    json_t* websocket = json_object();
    if (!websocket) {
        log_this("Configuration", "Failed to create WebSocket config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(websocket, "Enabled", json_boolean(DEFAULT_WEBSOCKET_ENABLED));
    json_object_set_new(websocket, "EnableIPv6", json_boolean(DEFAULT_WEBSOCKET_ENABLE_IPV6));
    json_object_set_new(websocket, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(websocket, "Key", json_string(DEFAULT_WEBSOCKET_KEY));
    json_object_set_new(websocket, "Protocol", json_string(DEFAULT_WEBSOCKET_PROTOCOL));
    json_object_set_new(websocket, "MaxMessageSize", json_integer(10 * 1024 * 1024));

    json_t* timeouts = json_object();
    json_object_set_new(timeouts, "ExitWaitSeconds", json_integer(10));
    json_object_set_new(websocket, "ConnectionTimeouts", timeouts);

    return websocket;
}

/*
 * J. Generate default Terminal configuration
 */
static json_t* create_default_terminal_config(void) {
    json_t* terminal = json_object();
    if (!terminal) {
        log_this("Configuration", "Failed to create terminal config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(terminal, "Enabled", json_boolean(true));
    json_object_set_new(terminal, "WebPath", json_string("/terminal"));
    json_object_set_new(terminal, "ShellCommand", json_string("/bin/bash"));
    json_object_set_new(terminal, "MaxSessions", json_integer(4));
    json_object_set_new(terminal, "IdleTimeoutSeconds", json_integer(300));  // 5 minutes

    return terminal;
}

/*
 * K. Generate default mDNS Server configuration
 */
static json_t* create_default_mdns_server_config(void) {
    json_t* mdns = json_object();
    if (!mdns) {
        log_this("Configuration", "Failed to create mDNS server config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(mdns, "Enabled", json_boolean(true));
    json_object_set_new(mdns, "EnableIPv6", json_boolean(false));
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string(VERSION));

    json_t* services = json_array();
    json_array_append_new(services, json_pack("{s:s, s:s, s:i, s:s}",
        "Name", "hydrogen",
        "Type", "_http._tcp.local",
        "Port", DEFAULT_WEB_PORT,
        "TxtRecords", "path=/api/upload"
    ));
    json_array_append_new(services, json_pack("{s:s, s:s, s:i, s:s}",
        "Name", "Hydrogen",
        "Type", "_websocket._tcp.local",
        "Port", DEFAULT_WEBSOCKET_PORT,
        "TxtRecords", "path=/websocket"
    ));
    json_object_set_new(mdns, "Services", services);

    return mdns;
}

/*
 * L. Generate default mDNS Client configuration
 */
static json_t* create_default_mdns_client_config(void) {
    json_t* mdns = json_object();
    if (!mdns) {
        log_this("Configuration", "Failed to create mDNS client config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(mdns, "Enabled", json_boolean(true));
    json_object_set_new(mdns, "EnableIPv6", json_boolean(false));
    json_object_set_new(mdns, "ScanInterval", json_integer(30));  // 30 seconds

    json_t* types = json_array();
    json_array_append_new(types, json_string("_http._tcp.local"));
    json_array_append_new(types, json_string("_websocket._tcp.local"));
    json_object_set_new(mdns, "ServiceTypes", types);

    json_t* health = json_object();
    json_object_set_new(health, "Enabled", json_boolean(true));
    json_object_set_new(health, "Interval", json_integer(60));  // 60 seconds
    json_object_set_new(mdns, "HealthCheck", health);

    return mdns;
}

/*
 * M. Generate default Mail Relay configuration
 */
static json_t* create_default_mail_relay_config(void) {
    json_t* mail = json_object();
    if (!mail) {
        log_this("Configuration", "Failed to create mail relay config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(mail, "Enabled", json_boolean(true));
    json_object_set_new(mail, "ListenPort", json_integer(25));
    json_object_set_new(mail, "Workers", json_integer(2));

    json_t* servers = json_array();
    json_t* server = json_object();
    json_object_set_new(server, "Host", json_string("${env.SMTP_HOST:-smtp.example.com}"));
    json_object_set_new(server, "Port", json_integer(587));
    json_object_set_new(server, "Username", json_string("${env.SMTP_USER}"));
    json_object_set_new(server, "Password", json_string("${env.SMTP_PASS}"));
    json_object_set_new(server, "UseTLS", json_boolean(true));
    json_array_append_new(servers, server);
    json_object_set_new(mail, "OutboundServers", servers);

    return mail;
}

/*
 * N. Generate default Print configuration
 */
static json_t* create_default_print_config(void) {
    json_t* print = json_object();
    if (!print) {
        log_this("Configuration", "Failed to create print config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(print, "Enabled", json_boolean(true));

    json_t* settings = json_object();
    json_object_set_new(settings, "DefaultPriority", json_integer(1));
    json_object_set_new(settings, "EmergencyPriority", json_integer(0));
    json_object_set_new(settings, "MaintenancePriority", json_integer(2));
    json_object_set_new(settings, "SystemPriority", json_integer(3));
    json_object_set_new(print, "QueueSettings", settings);

    json_t* timeouts = json_object();
    json_object_set_new(timeouts, "ShutdownWaitMs", json_integer(DEFAULT_SHUTDOWN_WAIT_MS));
    json_object_set_new(timeouts, "JobProcessingTimeoutMs", json_integer(DEFAULT_JOB_PROCESSING_TIMEOUT_MS));
    json_object_set_new(print, "Timeouts", timeouts);

    json_t* buffers = json_object();
    json_object_set_new(buffers, "JobMessageSize", json_integer(256));
    json_object_set_new(buffers, "StatusMessageSize", json_integer(256));
    json_object_set_new(print, "Buffers", buffers);

    return print;
}

/*
 * N. Generate default System Resources configuration
 */
static json_t* create_default_system_resources_config(void) {
    json_t* resources = json_object();
    if (!resources) {
        log_this("Configuration", "Failed to create system resources config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_t* memory = json_object();
    json_object_set_new(memory, "MaxHeapSize", json_integer(1024 * 1024 * 1024));  // 1GB
    json_object_set_new(memory, "MinFreeMemory", json_integer(64 * 1024 * 1024));  // 64MB
    json_object_set_new(memory, "GCThreshold", json_integer(85));  // 85% usage triggers GC
    json_object_set_new(resources, "Memory", memory);

    json_t* threads = json_object();
    json_object_set_new(threads, "MaxThreads", json_integer(100));
    json_object_set_new(threads, "ThreadStackSize", json_integer(8 * 1024 * 1024));  // 8MB
    json_object_set_new(threads, "ThreadPoolSize", json_integer(4));
    json_object_set_new(resources, "Threads", threads);

    json_t* files = json_object();
    json_object_set_new(files, "MaxOpenFiles", json_integer(1024));
    json_object_set_new(files, "MaxFileSize", json_integer(100 * 1024 * 1024));  // 100MB
    json_object_set_new(files, "TempFileLifetime", json_integer(3600));  // 1 hour
    json_object_set_new(resources, "Files", files);

    return resources;
}


/*
 * P. Generate default Notify configuration
 */
static json_t* create_default_notify_config(void) {
    json_t* notify = json_object();
    if (!notify) {
        log_this("Configuration", "Failed to create notify config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(notify, "Enabled", json_boolean(true));
    json_object_set_new(notify, "MaxRetries", json_integer(3));
    json_object_set_new(notify, "RetryDelayMs", json_integer(1000));

    json_t* channels = json_array();
    json_array_append_new(channels, json_string("email"));
    json_array_append_new(channels, json_string("websocket"));
    json_object_set_new(notify, "Channels", channels);

    json_t* templates = json_object();
    json_object_set_new(templates, "Path", json_string("${env.NOTIFY_TEMPLATES:-/etc/hydrogen/templates}"));
    json_object_set_new(templates, "DefaultLang", json_string("en"));
    json_object_set_new(notify, "Templates", templates);

    return notify;
}


/*
 * Generate complete default configuration
 * 
 * Creates a complete configuration file with all sections A-P
 * in the correct order as specified in the header comments.
 */
void create_default_config(const char* config_path) {
    json_t* root = json_object();
    if (!root) {
        log_this("Configuration", "Failed to create root config object", LOG_LEVEL_ERROR);
        return;
    }

    // Add sections in A-P order
    json_t* sections[] = {
        create_default_server_config(),        // A. Server
        create_default_system_config(),        // B. System
        create_default_network_config(),       // C. Network
        create_default_database_config(),      // D. Database
        create_default_logging_config(),       // E. Logging
        create_default_web_config(),          // F. WebServer
        create_default_api_config(),          // G. API
        create_default_swagger_config(),       // H. Swagger
        create_default_websocket_config(),     // I. WebSocket
        create_default_terminal_config(),      // J. Terminal
        create_default_mdns_server_config(),   // K. mDNS Server
        create_default_mdns_client_config(),   // L. mDNS Client
        create_default_mail_relay_config(),    // M. Mail Relay
        create_default_print_config(),         // N. Print
        create_default_system_resources_config(), // O. System Resources
        create_default_notify_config()         // P. Notify
    };

    const char* section_names[] = {
        "Server",           // A. Server
        "System",           // B. System
        "Network",          // C. Network
        "Database",         // D. Database
        "Logging",          // E. Logging
        "WebServer",        // F. WebServer
        "API",              // G. API
        "Swagger",          // H. Swagger
        "WebSocketServer",  // I. WebSocket
        "Terminal",         // J. Terminal
        "mDNSServer",       // K. mDNS Server
        "mDNSClient",       // L. mDNS Client
        "MailRelay",        // M. Mail Relay
        "PrintQueue",       // N. Print
        "SystemResources",  // O. System Resources
        "Notify"           // P. Notify
    };

    for (size_t i = 0; i < sizeof(sections) / sizeof(sections[0]); i++) {
        if (sections[i]) {
            json_object_set_new(root, section_names[i], sections[i]);
        }
    }

    // Write configuration to file
    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        log_this("Configuration", "Error: Unable to create default config at %s", 
                 LOG_LEVEL_ERROR, config_path);
    } else {
        log_this("Configuration", "Created default config at %s", 
                 LOG_LEVEL_STATE, config_path);
    }

    json_decref(root);
}