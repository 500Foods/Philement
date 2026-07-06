/*
 * Mail relay configuration implementation
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_mail_relay.h"
#include "config_databases.h"

// Helper function to cleanup a single server configuration
void cleanup_server(OutboundServer* server) {
    if (!server) return;

    free(server->Host);
    server->Host = NULL;

    free(server->Port);
    server->Port = NULL;

    free(server->Username);
    server->Username = NULL;

    free(server->Password);
    server->Password = NULL;

    free(server->CAPath);
    server->CAPath = NULL;

    server->UseTLS = false;
    server->TLSMode = MAIL_TLS_MODE_NONE;
    server->AuthMode = MAIL_AUTH_MODE_NONE;
    server->TimeoutSeconds = 0;
}

// Helper function to cleanup mail relay events configuration
void cleanup_mail_relay_events(MailRelayEvents* events) {
    if (!events) return;

    for (int i = 0; i < events->RuleCount; i++) {
        free(events->Rules[i].event_key);
        events->Rules[i].event_key = NULL;

        free(events->Rules[i].script_name);
        events->Rules[i].script_name = NULL;
    }

    events->RuleCount = 0;
    events->Enabled = false;
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
    mail->Enabled = false;
    mail->OutboundEnabled = false;
    mail->InboundEnabled = false;
    mail->ListenPort = 25;
    mail->Workers = 2;

    // Database defaults to NULL; resolved after loading if only one DB exists
    mail->Database = NULL;
    mail->DefaultFrom = NULL;
    mail->DefaultReplyTo = NULL;
    mail->AdminRecipientCount = 0;

    // Queue defaults
    mail->Queue.MaxQueueSize = 1000;
    mail->Queue.MaxInMemory = 1000;
    mail->Queue.Persist = false;
    mail->Queue.RetryAttempts = 3;
    mail->Queue.RetryDelaySeconds = 300;
    mail->Queue.InitialDelaySeconds = 10;
    mail->Queue.MaxDelaySeconds = 3600;
    mail->Queue.DebounceSeconds = 5;

    // Template defaults
    mail->Templates.ReloadIntervalSeconds = 60;

    // Events defaults
    mail->Events.Enabled = false;
    mail->Events.RuleCount = 0;
    for (int i = 0; i < MAX_MAIL_RELAY_EVENT_RULES; i++) {
        mail->Events.Rules[i].event_key = NULL;
        mail->Events.Rules[i].script_name = NULL;
    }

    // Initialize outbound servers array
    mail->OutboundServerCount = 0;
    for (int i = 0; i < MAX_OUTBOUND_SERVERS; i++) {
        memset(&mail->Servers[i], 0, sizeof(OutboundServer));
    }

    // Process all config items in sequence
    success = PROCESS_SECTION(root, "MailRelay");
    success = success && PROCESS_BOOL(root, mail, Enabled, "MailRelay.Enabled", "MailRelay");
    success = success && PROCESS_BOOL(root, mail, OutboundEnabled, "MailRelay.OutboundEnabled", "MailRelay");
    success = success && PROCESS_BOOL(root, mail, InboundEnabled, "MailRelay.InboundEnabled", "MailRelay");
    success = success && PROCESS_STRING(root, mail, Database, "MailRelay.Database", "MailRelay");
    success = success && PROCESS_STRING(root, mail, DefaultFrom, "MailRelay.DefaultFrom", "MailRelay");
    success = success && PROCESS_STRING(root, mail, DefaultReplyTo, "MailRelay.DefaultReplyTo", "MailRelay");
    success = success && PROCESS_INT(root, mail, ListenPort, "MailRelay.ListenPort", "MailRelay");
    success = success && PROCESS_INT(root, mail, Workers, "MailRelay.Workers", "MailRelay");

    // Process queue settings
    success = success && PROCESS_INT(root, &mail->Queue, MaxInMemory, "MailRelay.Queue.MaxInMemory", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, MaxQueueSize, "MailRelay.Queue.MaxQueueSize", "MailRelay");
    success = success && PROCESS_BOOL(root, &mail->Queue, Persist, "MailRelay.Queue.Persist", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, RetryAttempts, "MailRelay.Queue.RetryAttempts", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, RetryDelaySeconds, "MailRelay.Queue.RetryDelaySeconds", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, InitialDelaySeconds, "MailRelay.Queue.InitialDelaySeconds", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, MaxDelaySeconds, "MailRelay.Queue.MaxDelaySeconds", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Queue, DebounceSeconds, "MailRelay.Queue.DebounceSeconds", "MailRelay");

    // Process template settings
    success = success && PROCESS_INT(root, &mail->Templates, ReloadIntervalSeconds, "MailRelay.Templates.ReloadIntervalSeconds", "MailRelay");

    // Process events section
    json_t* mail_relay_section = json_object_get(root, "MailRelay");
    json_t* events_section = mail_relay_section ? json_object_get(mail_relay_section, "Events") : NULL;
    if (events_section && json_is_object(events_section)) {
        json_t* enabled = json_object_get(events_section, "Enabled");
        if (enabled && json_is_boolean(enabled)) {
            mail->Events.Enabled = json_boolean_value(enabled);
        }

        json_t* rules = json_object_get(events_section, "Rules");
        if (rules && json_is_object(rules)) {
            const char* key = NULL;
            json_t* value = NULL;
            json_object_foreach(rules, key, value) {
                if (mail->Events.RuleCount >= MAX_MAIL_RELAY_EVENT_RULES) {
                    log_this(SR_CONFIG, "Too many event rules configured", LOG_LEVEL_ERROR, 0);
                    break;
                }
                if (!key || !json_is_string(value)) {
                    continue;
                }
                const char* script = json_string_value(value);
                if (!script || !*script) {
                    continue;
                }
                mail->Events.Rules[mail->Events.RuleCount].event_key = strdup(key);
                mail->Events.Rules[mail->Events.RuleCount].script_name = strdup(script);
                mail->Events.RuleCount++;
            }
        }
    }

    // Process admin recipients
    json_t* admin_recipients = mail_relay_section ? json_object_get(mail_relay_section, "AdminRecipients") : NULL;
    if (admin_recipients && json_is_array(admin_recipients)) {
        size_t index;
        json_t* recipient;
        json_array_foreach(admin_recipients, index, recipient) {
            if (mail->AdminRecipientCount >= MAX_MAIL_RELAY_ADMIN_RECIPIENTS) {
                log_this(SR_CONFIG, "Too many admin recipients configured", LOG_LEVEL_ERROR, 0);
                break;
            }
            const char* addr = NULL;
            if (json_is_string(recipient)) {
                addr = json_string_value(recipient);
            }
            if (addr && *addr) {
                mail->AdminRecipients[mail->AdminRecipientCount] = strdup(addr);
                mail->AdminRecipientCount++;
            }
        }
    }

    // Process server configurations
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

            snprintf(path, sizeof(path), "%s.TLSMode", path_base);
            success = success && PROCESS_INT(root, &mail->Servers[index], TLSMode, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.CAPath", path_base);
            success = success && PROCESS_STRING(root, &mail->Servers[index], CAPath, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.AuthMode", path_base);
            success = success && PROCESS_INT(root, &mail->Servers[index], AuthMode, path, "MailRelay");

            snprintf(path, sizeof(path), "%s.TimeoutSeconds", path_base);
            success = success && PROCESS_INT(root, &mail->Servers[index], TimeoutSeconds, path, "MailRelay");

            if (success) {
                mail->OutboundServerCount++;
            }
        }
    }

    // Default Database to the only configured database if not explicitly set
    if (!mail->Database && config &&
        config->databases.connection_count == 1 &&
        config->databases.connections[0].connection_name &&
        config->databases.connections[0].connection_name[0] != '\0') {
        mail->Database = strdup(config->databases.connections[0].connection_name);
    }

    if (!success) {
        cleanup_mailrelay_config(mail);
    }

    return success;
}

// Clean up mail relay configuration
void cleanup_mailrelay_config(MailRelayConfig* config) {
    if (!config) return;

    free(config->Database);
    config->Database = NULL;

    free(config->DefaultFrom);
    config->DefaultFrom = NULL;

    free(config->DefaultReplyTo);
    config->DefaultReplyTo = NULL;

    for (int i = 0; i < config->AdminRecipientCount; i++) {
        free(config->AdminRecipients[i]);
        config->AdminRecipients[i] = NULL;
    }
    config->AdminRecipientCount = 0;

    cleanup_mail_relay_events(&config->Events);

    // Free all outbound server configurations
    for (int i = 0; i < config->OutboundServerCount; i++) {
        cleanup_server(&config->Servers[i]);
    }
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
    DUMP_BOOL2("――", "OutboundEnabled", config->OutboundEnabled);
    DUMP_BOOL2("――", "InboundEnabled", config->InboundEnabled);

    char buffer[256];
    if (config->Database) {
        snprintf(buffer, sizeof(buffer), "Database: %s", config->Database);
    } else {
        snprintf(buffer, sizeof(buffer), "Database: (not set)");
    }
    DUMP_TEXT("――", buffer);

    if (config->DefaultFrom) {
        snprintf(buffer, sizeof(buffer), "Default From: %s", config->DefaultFrom);
    } else {
        snprintf(buffer, sizeof(buffer), "Default From: (not set)");
    }
    DUMP_TEXT("――", buffer);

    if (config->DefaultReplyTo) {
        snprintf(buffer, sizeof(buffer), "Default Reply-To: %s", config->DefaultReplyTo);
    } else {
        snprintf(buffer, sizeof(buffer), "Default Reply-To: (not set)");
    }
    DUMP_TEXT("――", buffer);

    snprintf(buffer, sizeof(buffer), "Admin Recipients: %d", config->AdminRecipientCount);
    DUMP_TEXT("――", buffer);
    for (int i = 0; i < config->AdminRecipientCount; i++) {
        snprintf(buffer, sizeof(buffer), "  Admin %d: %s", i + 1, config->AdminRecipients[i]);
        DUMP_TEXT("――――", buffer);
    }

    snprintf(buffer, sizeof(buffer), "Listen Port: %d", config->ListenPort);
    DUMP_TEXT("――", buffer);
    snprintf(buffer, sizeof(buffer), "Workers: %d", config->Workers);
    DUMP_TEXT("――", buffer);

    // Dump queue settings
    DUMP_TEXT("――", "Queue Settings");
    snprintf(buffer, sizeof(buffer), "Max Queue Size: %d (Max In Memory: %d)",
             config->Queue.MaxQueueSize, config->Queue.MaxInMemory);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Persist: %s", config->Queue.Persist ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Attempts: %d", config->Queue.RetryAttempts);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Retry Delay: %d seconds", config->Queue.RetryDelaySeconds);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Initial Delay: %d seconds", config->Queue.InitialDelaySeconds);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Delay: %d seconds", config->Queue.MaxDelaySeconds);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Debounce: %d seconds", config->Queue.DebounceSeconds);
    DUMP_TEXT("――――", buffer);

    // Dump template settings
    DUMP_TEXT("――", "Template Settings");
    snprintf(buffer, sizeof(buffer), "Reload Interval: %d seconds", config->Templates.ReloadIntervalSeconds);
    DUMP_TEXT("――――", buffer);

    // Dump events settings
    DUMP_TEXT("――", "Events Settings");
    snprintf(buffer, sizeof(buffer), "Enabled: %s", config->Events.Enabled ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Rule Count: %d", config->Events.RuleCount);
    DUMP_TEXT("――――", buffer);
    for (int i = 0; i < config->Events.RuleCount; i++) {
        snprintf(buffer, sizeof(buffer), "  Event %d: %s -> %s",
                 i + 1,
                 config->Events.Rules[i].event_key,
                 config->Events.Rules[i].script_name);
        DUMP_TEXT("――――", buffer);
    }

    // Dump outbound servers
    snprintf(buffer, sizeof(buffer), "Outbound Servers (%d)", config->OutboundServerCount);
    DUMP_TEXT("――", buffer);

    for (int i = 0; i < config->OutboundServerCount; i++) {
        const OutboundServer* server = &config->Servers[i];

        snprintf(buffer, sizeof(buffer), "Server %d", i + 1);
        DUMP_TEXT("――――", buffer);

        snprintf(buffer, sizeof(buffer), "Host: %s", server->Host ? server->Host : "(null)");
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "Port: %s", server->Port ? server->Port : "(null)");
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "Username: %s", server->Username ? server->Username : "(null)");
        DUMP_TEXT("――――――", buffer);

        DUMP_TEXT("――――――", "Password: *****");

        snprintf(buffer, sizeof(buffer), "UseTLS: %s", server->UseTLS ? "true" : "false");
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "TLSMode: %d", server->TLSMode);
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "CAPath: %s", server->CAPath ? server->CAPath : "(not set)");
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "AuthMode: %d", server->AuthMode);
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "Timeout: %d seconds", server->TimeoutSeconds);
        DUMP_TEXT("――――――", buffer);
    }
}
