/*
 * Mail relay configuration implementation
 */

#include "config_mail_relay.h"
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

/*
 * Initialize mail relay configuration with defaults
 *
 * This function initializes a new MailRelayConfig structure with
 * default values that provide a secure baseline configuration.
 *
 * @param config Pointer to MailRelayConfig structure to initialize
 * @return 0 on success, -1 on failure
 */
int config_mailrelay_init(MailRelayConfig* config) {
    if (!config) return -1;

    // Initialize main configuration
    config->Enabled = DEFAULT_MAILRELAY_ENABLED;
    config->ListenPort = DEFAULT_MAILRELAY_LISTEN_PORT;
    config->Workers = DEFAULT_MAILRELAY_WORKERS;
    
    // Initialize queue settings
    config->Queue.MaxQueueSize = DEFAULT_MAILRELAY_MAX_QUEUE_SIZE;
    config->Queue.RetryAttempts = DEFAULT_MAILRELAY_RETRY_ATTEMPTS;
    config->Queue.RetryDelaySeconds = DEFAULT_MAILRELAY_RETRY_DELAY;

    // Initialize outbound servers array
    config->OutboundServerCount = 2;  // Default to 2 servers for redundancy
    
    // Initialize primary server
    config->Servers[0].Host = strdup("${env.SMTP_SERVER1_HOST}");
    if (!config->Servers[0].Host) goto error;
    
    config->Servers[0].Port = strdup("${env.SMTP_SERVER1_PORT}");
    if (!config->Servers[0].Port) goto error;
    
    config->Servers[0].Username = strdup("${env.SMTP_SERVER1_USER}");
    if (!config->Servers[0].Username) goto error;
    
    config->Servers[0].Password = strdup("${env.SMTP_SERVER1_PASS}");
    if (!config->Servers[0].Password) goto error;
    
    config->Servers[0].UseTLS = true;

    // Initialize backup server
    config->Servers[1].Host = strdup("${env.SMTP_SERVER2_HOST}");
    if (!config->Servers[1].Host) goto error;
    
    config->Servers[1].Port = strdup("${env.SMTP_SERVER2_PORT}");
    if (!config->Servers[1].Port) goto error;
    
    config->Servers[1].Username = strdup("${env.SMTP_SERVER2_USER}");
    if (!config->Servers[1].Username) goto error;
    
    config->Servers[1].Password = strdup("${env.SMTP_SERVER2_PASS}");
    if (!config->Servers[1].Password) goto error;
    
    config->Servers[1].UseTLS = true;

    return 0;

error:
    config_mailrelay_cleanup(config);
    return -1;
}

/*
 * Clean up mail relay configuration
 *
 * This function frees all resources allocated by config_mailrelay_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to MailRelayConfig structure to cleanup
 */
void config_mailrelay_cleanup(MailRelayConfig* config) {
    if (!config) return;

    // Free all outbound server configurations
    for (int i = 0; i < config->OutboundServerCount; i++) {
        free(config->Servers[i].Host);
        free(config->Servers[i].Port);
        free(config->Servers[i].Username);
        free(config->Servers[i].Password);

        // Reset pointers
        config->Servers[i].Host = NULL;
        config->Servers[i].Port = NULL;
        config->Servers[i].Username = NULL;
        config->Servers[i].Password = NULL;
    }

    // Reset counts and settings
    config->OutboundServerCount = 0;
}

/*
 * Validate mail relay configuration values
 *
 * This function performs validation of the configuration:
 * - Verifies enabled status and port ranges
 * - Validates worker count
 * - Checks queue settings
 * - Validates outbound server configurations
 *
 * @param config Pointer to MailRelayConfig structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_mailrelay_validate(const MailRelayConfig* config) {
    if (!config) return -1;

    // If enabled, validate core settings
    if (config->Enabled) {
        // Validate port range
        if (config->ListenPort <= 0 || config->ListenPort > 65535) return -1;
        
        // Validate worker count
        if (config->Workers <= 0) return -1;

        // Validate queue settings
        if (config->Queue.MaxQueueSize <= 0) return -1;
        if (config->Queue.RetryAttempts < 0) return -1;
        if (config->Queue.RetryDelaySeconds <= 0) return -1;

        // Must have at least one outbound server
        if (config->OutboundServerCount <= 0 || 
            config->OutboundServerCount > MAX_OUTBOUND_SERVERS) return -1;

        // Validate each configured server
        for (int i = 0; i < config->OutboundServerCount; i++) {
            if (!config->Servers[i].Host) return -1;
            if (!config->Servers[i].Port) return -1;
            if (!config->Servers[i].Username) return -1;
            if (!config->Servers[i].Password) return -1;
        }
    }

    return 0;
}

/*
 * Load mail relay configuration from JSON
 *
 * This function loads the mail relay configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_mailrelay_config(json_t* root, AppConfig* config) {
    if (!config) {
        log_this("Config", "Invalid parameters for mail relay configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize with defaults
    if (config_mailrelay_init(&config->mail_relay) != 0) {
        log_this("Config", "Failed to initialize mail relay configuration", LOG_LEVEL_ERROR);
        return false;
    }

    bool success = true;
    MailRelayConfig* mail = &config->mail_relay;

    // Process all config items in sequence
    success = PROCESS_SECTION(root, "MailRelay");
    success = success && PROCESS_BOOL(root, mail, Enabled, "MailRelay.Enabled", "MailRelay");
    success = success && PROCESS_INT(root, mail, ListenPort, "MailRelay.ListenPort", "MailRelay");
    success = success && PROCESS_INT(root, mail, Workers, "MailRelay.Workers", "MailRelay");

    // Process queue settings
    success = success && PROCESS_INT(root, &mail->Queue, MaxQueueSize, "MailRelay.Queue.MaxQueueSize", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, RetryAttempts, "MailRelay.Queue.RetryAttempts", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, RetryDelaySeconds, "MailRelay.Queue.RetryDelaySeconds", "MailRelay");

    // Process server configurations
    json_t* servers = json_object_get(root, "MailRelay.Servers");
    if (servers && json_is_array(servers)) {
        size_t index;
        json_t* server;
        mail->OutboundServerCount = 0;

        json_array_foreach(servers, index, server) {
            if (index >= MAX_OUTBOUND_SERVERS) {
                log_this("Config", "Too many outbound servers configured", LOG_LEVEL_ERROR);
                break;
            }

            char path_base[64];
            snprintf(path_base, sizeof(path_base), "MailRelay.Servers[%zu]", index);

            char path[128];
            snprintf(path, sizeof(path), "%s.Host", path_base);
            success = success && PROCESS_STRING(root, &mail->Servers[index], Host, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.Port", path_base);
            success = success && PROCESS_STRING(root, &mail->Servers[index], Port, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.Username", path_base);
            success = success && PROCESS_STRING(root, &mail->Servers[index], Username, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.Password", path_base);
            success = success && PROCESS_SENSITIVE(root, &mail->Servers[index], Password, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.UseTLS", path_base);
            success = success && PROCESS_BOOL(root, &mail->Servers[index], UseTLS, path, "MailRelay");

            if (success) {
                mail->OutboundServerCount++;
            }
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_mailrelay_validate(&config->mail_relay) == 0);
    }

    if (!success) {
        config_mailrelay_cleanup(&config->mail_relay);
    }

    return success;
}