/*
 * Mail Relay JSON Configuration Loading Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "json_mail_relay.h"
#include "../mailrelay/config_mail_relay.h"
#include "../env/config_env.h"
#include "../env/config_env_utils.h"
#include "../../logging/logging.h"

// Load a single outbound server configuration
static bool load_outbound_server(json_t* server_json, OutboundServer* server) {
    if (!server_json || !server) return false;

    // Get host with environment variable substitution
    json_t* host = json_object_get(server_json, "Host");
    if (host && json_is_string(host)) {
        server->Host = get_config_string_with_env("Host", host, NULL);
        if (!server->Host) return false;
    }

    // Get port with environment variable substitution
    json_t* port = json_object_get(server_json, "Port");
    if (port && json_is_string(port)) {
        server->Port = get_config_string_with_env("Port", port, NULL);
        if (!server->Port) goto error;
    }

    // Get username with environment variable substitution
    json_t* username = json_object_get(server_json, "Username");
    if (username && json_is_string(username)) {
        server->Username = get_config_string_with_env("Username", username, NULL);
        if (!server->Username) goto error;
    }

    // Get password with environment variable substitution
    json_t* password = json_object_get(server_json, "Password");
    if (password && json_is_string(password)) {
        server->Password = get_config_string_with_env("Password", password, NULL);
        if (!server->Password) goto error;
    }

    // Get TLS setting
    json_t* use_tls = json_object_get(server_json, "UseTLS");
    if (use_tls && json_is_boolean(use_tls)) {
        server->UseTLS = json_boolean_value(use_tls);
    }

    return true;

error:
    free(server->Host);
    free(server->Port);
    free(server->Username);
    free(server->Password);
    server->Host = NULL;
    server->Port = NULL;
    server->Username = NULL;
    server->Password = NULL;
    return false;
}

// Load queue settings configuration
static bool load_queue_settings(json_t* queue_json, QueueSettings* queue) {
    if (!queue_json || !queue) return false;

    // Get max queue size
    json_t* max_size = json_object_get(queue_json, "MaxQueueSize");
    if (max_size && json_is_integer(max_size)) {
        queue->MaxQueueSize = json_integer_value(max_size);
    }

    // Get retry attempts
    json_t* retries = json_object_get(queue_json, "RetryAttempts");
    if (retries && json_is_integer(retries)) {
        queue->RetryAttempts = json_integer_value(retries);
    }

    // Get retry delay
    json_t* delay = json_object_get(queue_json, "RetryDelaySeconds");
    if (delay && json_is_integer(delay)) {
        queue->RetryDelaySeconds = json_integer_value(delay);
    }

    return true;
}

/*
 * Load mail relay configuration from JSON
 */
bool load_json_mail_relay(json_t* root, AppConfig* config) {
    if (!config) return false;

    // Initialize with defaults
    if (config_mailrelay_init(&config->mail_relay) != 0) {
        log_this("Config", "Failed to initialize mail relay config", LOG_LEVEL_ERROR);
        return false;
    }

    // If no JSON, keep defaults
    if (!root) {
        log_this("Config", "No JSON provided, using mail relay defaults", LOG_LEVEL_ALERT);
        return true;
    }

    // Get mail relay section
    json_t* mail_relay = json_object_get(root, "MailRelay");
    if (!mail_relay) {
        log_this("Config", "No MailRelay section in JSON, using defaults", LOG_LEVEL_ALERT);
        return true;
    }

    // Get enabled status
    json_t* enabled = json_object_get(mail_relay, "Enabled");
    if (enabled && json_is_boolean(enabled)) {
        config->mail_relay.Enabled = json_boolean_value(enabled);
    }

    // Get listen port
    json_t* port = json_object_get(mail_relay, "ListenPort");
    if (port && json_is_integer(port)) {
        config->mail_relay.ListenPort = json_integer_value(port);
    }

    // Get worker count
    json_t* workers = json_object_get(mail_relay, "Workers");
    if (workers && json_is_integer(workers)) {
        config->mail_relay.Workers = json_integer_value(workers);
    }

    // Load queue settings
    json_t* queue = json_object_get(mail_relay, "QueueSettings");
    if (queue && json_is_object(queue)) {
        if (!load_queue_settings(queue, &config->mail_relay.Queue)) {
            log_this("Config", "Failed to load mail relay queue settings", LOG_LEVEL_ERROR);
            return false;
        }
    }

    // Load outbound servers
    json_t* servers = json_object_get(mail_relay, "OutboundServers");
    if (servers && json_is_array(servers)) {
        size_t index;
        json_t* server;

        config->mail_relay.OutboundServerCount = 0;
        json_array_foreach(servers, index, server) {
            if (index >= MAX_OUTBOUND_SERVERS) {
                log_this("Config", "Too many outbound servers defined (max %d)", LOG_LEVEL_ERROR, MAX_OUTBOUND_SERVERS);
                break;
            }

            if (!load_outbound_server(server, &config->mail_relay.Servers[index])) {
                log_this("Config", "Failed to load outbound server %zu", LOG_LEVEL_ERROR, index);
                return false;
            }
            config->mail_relay.OutboundServerCount++;
        }
    }

    // Validate the configuration
    if (config_mailrelay_validate(&config->mail_relay) != 0) {
        log_this("Config", "Mail relay configuration validation failed", LOG_LEVEL_ERROR);
        return false;
    }

    return true;
}