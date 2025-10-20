/*
 * Mail relay configuration implementation
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_mail_relay.h"

// Helper function to cleanup a single server configuration
void cleanup_server(OutboundServer* server) {
    if (!server) return;

    free(server->Host);
    free(server->Port);
    free(server->Username);
    free(server->Password);

    // Reset pointers
    server->Host = NULL;
    server->Port = NULL;
    server->Username = NULL;
    server->Password = NULL;
}

// Load mail relay configuration from JSON
bool load_mailrelay_config(json_t* root, AppConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Invalid parameters for mail relay configuration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool success = true;
    MailRelayConfig* mail = &config->mail_relay;

    // Initialize with defaults
    mail->Enabled = true;
    mail->ListenPort = 587;
    mail->Workers = 2;
    
    // Queue defaults
    mail->Queue.MaxQueueSize = 1000;
    mail->Queue.RetryAttempts = 3;
    mail->Queue.RetryDelaySeconds = 300;

    // Initialize outbound servers array
    mail->OutboundServerCount = 0;
    
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
    json_t* mail_relay_section = json_object_get(root, "MailRelay");
    json_t* servers = mail_relay_section ? json_object_get(mail_relay_section, "Servers") : NULL;
    if (servers && json_is_array(servers)) {
        size_t index;
        json_t* server;
        json_array_foreach(servers, index, server) {
            if (index >= MAX_OUTBOUND_SERVERS) {
                log_this(SR_CONFIG, "Too many outbound servers configured", LOG_LEVEL_ERROR, 0);
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

    // If no servers configured, set up default environment variable placeholders
    if (mail->OutboundServerCount == 0) {
        // Primary server defaults
        mail->Servers[0].Host = strdup("${env.SMTP_SERVER1_HOST}");
        mail->Servers[0].Port = strdup("${env.SMTP_SERVER1_PORT}");
        mail->Servers[0].Username = strdup("${env.SMTP_SERVER1_USER}");
        mail->Servers[0].Password = strdup("${env.SMTP_SERVER1_PASS}");
        mail->Servers[0].UseTLS = true;

        // Backup server defaults
        mail->Servers[1].Host = strdup("${env.SMTP_SERVER2_HOST}");
        mail->Servers[1].Port = strdup("${env.SMTP_SERVER2_PORT}");
        mail->Servers[1].Username = strdup("${env.SMTP_SERVER2_USER}");
        mail->Servers[1].Password = strdup("${env.SMTP_SERVER2_PASS}");
        mail->Servers[1].UseTLS = true;

        mail->OutboundServerCount = 2;

        // Log default server setup
        log_this(SR_CONFIG, "――― Using default environment variables for SMTP servers (*)", LOG_LEVEL_DEBUG, 0);
    }

    if (!success) {
        cleanup_mailrelay_config(mail);
    }

    return success;
}

// Clean up mail relay configuration
void cleanup_mailrelay_config(MailRelayConfig* config) {
    if (!config) return;

    // Free all outbound server configurations
    for (int i = 0; i < config->OutboundServerCount; i++) {
        cleanup_server(&config->Servers[i]);
    }

    // Reset counts and settings
    config->OutboundServerCount = 0;
}

// Dump mail relay configuration
void dump_mailrelay_config(const MailRelayConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL mail relay config");
        return;
    }

    // Dump basic configuration
    DUMP_BOOL2("――", "Enabled", config->Enabled);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Listen Port: %d", config->ListenPort);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Workers: %d", config->Workers);
    DUMP_TEXT("――", buffer);

    // Dump queue settings
    DUMP_TEXT("――", "Queue Settings");
    snprintf(buffer, sizeof(buffer), "Max Queue Size: %d", config->Queue.MaxQueueSize);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Attempts: %d", config->Queue.RetryAttempts);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Delay: %d seconds", config->Queue.RetryDelaySeconds);
    DUMP_TEXT("――――", buffer);

    // Dump outbound servers
    snprintf(buffer, sizeof(buffer), "Outbound Servers (%d)", config->OutboundServerCount);
    DUMP_TEXT("――", buffer);

    for (int i = 0; i < config->OutboundServerCount; i++) {
        const OutboundServer* server = &config->Servers[i];
        
        snprintf(buffer, sizeof(buffer), "Server %d", i + 1);
        DUMP_TEXT("――――", buffer);
        
        snprintf(buffer, sizeof(buffer), "Host: %s", server->Host);
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Port: %s", server->Port);
        DUMP_TEXT("――――――", buffer);
        snprintf(buffer, sizeof(buffer), "Username: %s", server->Username);
        DUMP_TEXT("――――――", buffer);
        DUMP_TEXT("――――――", "Password: *****");  // Don't dump actual password
        DUMP_BOOL2("――――――", "TLS Enabled", server->UseTLS);
    }
}
