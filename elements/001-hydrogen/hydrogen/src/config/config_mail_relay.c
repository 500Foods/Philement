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
    server->MinTLS = MAIL_TLS_DEFAULT_MIN_VERSION;
    server->AuthMode = MAIL_AUTH_MODE_NONE;
    server->TimeoutSeconds = 0;
}

// Helper function to cleanup mail relay security configuration
void cleanup_mail_relay_security(MailRelaySecurity* security) {
    if (!security) return;

    for (int i = 0; i < security->AllowSenderCount; i++) {
        free(security->AllowSenders[i]);
        security->AllowSenders[i] = NULL;
    }
    security->AllowSenderCount = 0;

    for (int i = 0; i < security->BlockSenderCount; i++) {
        free(security->BlockSenders[i]);
        security->BlockSenders[i] = NULL;
    }
    security->BlockSenderCount = 0;
}

// Helper function to cleanup mail relay test configuration
void cleanup_mail_relay_test(MailRelayTest* test) {
    if (!test) return;

    free(test->TestFrom);
    test->TestFrom = NULL;

    free(test->TestTo);
    test->TestTo = NULL;

    free(test->TestSubject);
    test->TestSubject = NULL;

    free(test->TestBody);
    test->TestBody = NULL;

    test->SendRawOnLaunch = false;
    test->SendOtpOnLaunch = false;
    test->FailNextSendOnLaunch = false;
    test->MailRepoProbeOnLaunch = false;
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
    mail->InboundRequireAuth = false;
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
    mail->Queue.StaleTimeoutSeconds = 300;

    // Template defaults
    mail->Templates.ReloadIntervalSeconds = 60;

    // OTP defaults
    mail->Otp.Digits = 6;
    mail->Otp.ExpirySeconds = 300;
    mail->Otp.MaxAttempts = 5;

    // Security defaults
    mail->Security.SenderPolicy = MAIL_SENDER_POLICY_ALLOW_ALL;
    mail->Security.AllowSenderCount = 0;
    mail->Security.BlockSenderCount = 0;
    for (int i = 0; i < MAX_MAIL_RELAY_ALLOW_SENDERS; i++) {
        mail->Security.AllowSenders[i] = NULL;
    }
    for (int i = 0; i < MAX_MAIL_RELAY_BLOCK_SENDERS; i++) {
        mail->Security.BlockSenders[i] = NULL;
    }

    // Rate-limit defaults (Phase 14.3)
    mail->RateLimit.Enabled = false;
    mail->RateLimit.Scope = MAIL_RL_SCOPE_GLOBAL;
    mail->RateLimit.MaxRequestsPerInterval = 60;
    mail->RateLimit.IntervalSeconds = 60;

    // Events defaults
    mail->Events.Enabled = false;
    mail->Events.MaxEventsPerInterval = 10;
    mail->Events.EventIntervalSeconds = 60;
    mail->Events.RuleCount = 0;
    for (int i = 0; i < MAX_MAIL_RELAY_EVENT_RULES; i++) {
        mail->Events.Rules[i].event_key = NULL;
        mail->Events.Rules[i].script_name = NULL;
    }

    // Test defaults
    mail->Test.SendRawOnLaunch = false;
    mail->Test.SendOtpOnLaunch = false;
    mail->Test.FailNextSendOnLaunch = false;
    mail->Test.MailRepoProbeOnLaunch = false;
    mail->Test.TestFrom = NULL;
    mail->Test.TestTo = NULL;
    mail->Test.TestSubject = NULL;
    mail->Test.TestBody = NULL;

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
    success = success && PROCESS_BOOL(root, mail, InboundRequireAuth, "MailRelay.InboundRequireAuth", "MailRelay");
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
    success = success && PROCESS_INT(root, &mail->Queue, StaleTimeoutSeconds, "MailRelay.Queue.StaleTimeoutSeconds", "MailRelay");

    // Process template settings
    success = success && PROCESS_INT(root, &mail->Templates, ReloadIntervalSeconds, "MailRelay.Templates.ReloadIntervalSeconds", "MailRelay");

    // Process OTP settings
    success = success && PROCESS_INT(root, &mail->Otp, Digits, "MailRelay.Otp.Digits", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Otp, ExpirySeconds, "MailRelay.Otp.ExpirySeconds", "MailRelay");
    success = success && PROCESS_INT(root, &mail->Otp, MaxAttempts, "MailRelay.Otp.MaxAttempts", "MailRelay");

    // Process events section
    json_t* mail_relay_section = json_object_get(root, "MailRelay");
    json_t* events_section = mail_relay_section ? json_object_get(mail_relay_section, "Events") : NULL;
    if (events_section && json_is_object(events_section)) {
        json_t* enabled = json_object_get(events_section, "Enabled");
        if (enabled && json_is_boolean(enabled)) {
            mail->Events.Enabled = json_boolean_value(enabled);
        }

        json_t* max_events = json_object_get(events_section, "MaxEventsPerInterval");
        if (max_events && json_is_integer(max_events)) {
            mail->Events.MaxEventsPerInterval = (int)json_integer_value(max_events);
        }

        json_t* interval_seconds = json_object_get(events_section, "EventIntervalSeconds");
        if (interval_seconds && json_is_integer(interval_seconds)) {
            mail->Events.EventIntervalSeconds = (int)json_integer_value(interval_seconds);
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
 
    // Process rate-limit section (Phase 14.3)
    json_t* rate_limit_section = mail_relay_section ? json_object_get(mail_relay_section, "RateLimit") : NULL;
    if (rate_limit_section && json_is_object(rate_limit_section)) {
        json_t* rl_enabled = json_object_get(rate_limit_section, "Enabled");
        if (rl_enabled && json_is_boolean(rl_enabled)) {
            mail->RateLimit.Enabled = json_boolean_value(rl_enabled);
        }

        json_t* rl_scope = json_object_get(rate_limit_section, "Scope");
        if (rl_scope && json_is_integer(rl_scope)) {
            int val = (int)json_integer_value(rl_scope);
            if (val >= MAIL_RL_SCOPE_GLOBAL && val <= MAIL_RL_SCOPE_TEMPLATE) {
                mail->RateLimit.Scope = val;
            } else {
                log_this(SR_CONFIG, "Invalid MailRelay.RateLimit.Scope value, using default", LOG_LEVEL_ERROR, 0);
            }
        }

        json_t* rl_max = json_object_get(rate_limit_section, "MaxRequestsPerInterval");
        if (rl_max && json_is_integer(rl_max)) {
            mail->RateLimit.MaxRequestsPerInterval = (int)json_integer_value(rl_max);
        }

        json_t* rl_interval = json_object_get(rate_limit_section, "IntervalSeconds");
        if (rl_interval && json_is_integer(rl_interval)) {
            mail->RateLimit.IntervalSeconds = (int)json_integer_value(rl_interval);
        }

        if (mail->RateLimit.MaxRequestsPerInterval < 0 ||
            mail->RateLimit.MaxRequestsPerInterval > MAX_MAIL_RL_REQUESTS) {
            log_this(SR_CONFIG,
                     "MailRelay.RateLimit.MaxRequestsPerInterval out of range [0, %d]",
                     LOG_LEVEL_ERROR, 1, MAX_MAIL_RL_REQUESTS);
            success = false;
        }
        if (mail->RateLimit.IntervalSeconds < MIN_MAIL_RL_INTERVAL ||
            mail->RateLimit.IntervalSeconds > MAX_MAIL_RL_INTERVAL) {
            log_this(SR_CONFIG,
                     "MailRelay.RateLimit.IntervalSeconds out of range [%d, %d]",
                     LOG_LEVEL_ERROR, 2, MIN_MAIL_RL_INTERVAL, MAX_MAIL_RL_INTERVAL);
            success = false;
        }
    }

    // Process security settings
    json_t* security_section = mail_relay_section ? json_object_get(mail_relay_section, "Security") : NULL;
    if (security_section && json_is_object(security_section)) {
        json_t* sender_policy = json_object_get(security_section, "SenderPolicy");
        if (sender_policy && json_is_integer(sender_policy)) {
            int val = (int)json_integer_value(sender_policy);
            if (val >= MAIL_SENDER_POLICY_ALLOW_ALL && val <= MAIL_SENDER_POLICY_BLOCKLIST) {
                mail->Security.SenderPolicy = val;
            } else {
                log_this(SR_CONFIG, "Invalid SenderPolicy value, using default", LOG_LEVEL_ERROR, 0);
            }
        }

        json_t* allow_arr = json_object_get(security_section, "AllowSenders");
        if (allow_arr && json_is_array(allow_arr)) {
            size_t idx;
            json_t* val;
            json_array_foreach(allow_arr, idx, val) {
                if (mail->Security.AllowSenderCount >= MAX_MAIL_RELAY_ALLOW_SENDERS) {
                    log_this(SR_CONFIG, "Too many allowed senders configured", LOG_LEVEL_ERROR, 0);
                    break;
                }
                if (json_is_string(val)) {
                    const char* v = json_string_value(val);
                    if (v && *v) {
                        mail->Security.AllowSenders[mail->Security.AllowSenderCount] = strdup(v);
                        mail->Security.AllowSenderCount++;
                    }
                }
            }
        }

        json_t* block_arr = json_object_get(security_section, "BlockSenders");
        if (block_arr && json_is_array(block_arr)) {
            size_t idx;
            json_t* val;
            json_array_foreach(block_arr, idx, val) {
                if (mail->Security.BlockSenderCount >= MAX_MAIL_RELAY_BLOCK_SENDERS) {
                    log_this(SR_CONFIG, "Too many blocked senders configured", LOG_LEVEL_ERROR, 0);
                    break;
                }
                if (json_is_string(val)) {
                    const char* v = json_string_value(val);
                    if (v && *v) {
                        mail->Security.BlockSenders[mail->Security.BlockSenderCount] = strdup(v);
                        mail->Security.BlockSenderCount++;
                    }
                }
            }
        }
    }

    // Process test configuration
    json_t* test_section = mail_relay_section ? json_object_get(mail_relay_section, "Test") : NULL;
    if (test_section && json_is_object(test_section)) {
        json_t* send_raw = json_object_get(test_section, "SendRawOnLaunch");
        if (send_raw && json_is_boolean(send_raw)) {
            mail->Test.SendRawOnLaunch = json_boolean_value(send_raw);
        }

        json_t* send_otp = json_object_get(test_section, "SendOtpOnLaunch");
        if (send_otp && json_is_boolean(send_otp)) {
            mail->Test.SendOtpOnLaunch = json_boolean_value(send_otp);
        }

        json_t* fail_next = json_object_get(test_section, "FailNextSendOnLaunch");
        if (fail_next && json_is_boolean(fail_next)) {
            mail->Test.FailNextSendOnLaunch = json_boolean_value(fail_next);
        }

        json_t* repo_probe = json_object_get(test_section, "MailRepoProbeOnLaunch");
        if (repo_probe && json_is_boolean(repo_probe)) {
            mail->Test.MailRepoProbeOnLaunch = json_boolean_value(repo_probe);
        }

        json_t* test_from = json_object_get(test_section, "TestFrom");
        if (test_from && json_is_string(test_from)) {
            const char* v = json_string_value(test_from);
            if (v && *v) {
                free(mail->Test.TestFrom);
                mail->Test.TestFrom = strdup(v);
            }
        }

        json_t* test_to = json_object_get(test_section, "TestTo");
        if (test_to && json_is_string(test_to)) {
            const char* v = json_string_value(test_to);
            if (v && *v) {
                free(mail->Test.TestTo);
                mail->Test.TestTo = strdup(v);
            }
        }

        json_t* test_subject = json_object_get(test_section, "TestSubject");
        if (test_subject && json_is_string(test_subject)) {
            const char* v = json_string_value(test_subject);
            if (v && *v) {
                free(mail->Test.TestSubject);
                mail->Test.TestSubject = strdup(v);
            }
        }

        json_t* test_body = json_object_get(test_section, "TestBody");
        if (test_body && json_is_string(test_body)) {
            const char* v = json_string_value(test_body);
            if (v && *v) {
                free(mail->Test.TestBody);
                mail->Test.TestBody = strdup(v);
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

    // Process server configurations.
    // NOTE: process_config_value() resolves paths by splitting on '.' only, so
    // array-index syntax like "MailRelay.Servers[0].Host" is not supported. Pass
    // the per-element JSON object as the root and use relative field paths.
    json_t* servers = mail_relay_section ? json_object_get(mail_relay_section, "Servers") : NULL;
    if (servers && json_is_array(servers)) {
        size_t index;
        json_t* server;
        json_array_foreach(servers, index, server) {
            if (index >= MAX_OUTBOUND_SERVERS) {
                log_this(SR_CONFIG, "Too many outbound servers configured", LOG_LEVEL_ERROR, 0);
                break;
            }

            success = success && PROCESS_STRING(server, &mail->Servers[index], Host, "Host", "MailRelay");
            success = success && PROCESS_STRING(server, &mail->Servers[index], Port, "Port", "MailRelay");
            success = success && PROCESS_STRING(server, &mail->Servers[index], Username, "Username", "MailRelay");
            success = success && PROCESS_SENSITIVE(server, &mail->Servers[index], Password, "Password", "MailRelay");
            success = success && PROCESS_BOOL(server, &mail->Servers[index], UseTLS, "UseTLS", "MailRelay");
            success = success && PROCESS_INT(server, &mail->Servers[index], TLSMode, "TLSMode", "MailRelay");
            // MinTLS defaults to MAIL_TLS_DEFAULT_MIN_VERSION if not set; validate range
            mail->Servers[index].MinTLS = MAIL_TLS_DEFAULT_MIN_VERSION;
            success = success && PROCESS_INT(server, &mail->Servers[index], MinTLS, "MinTLS", "MailRelay");
            if (mail->Servers[index].MinTLS < MIN_MAIL_TLS_VERSION ||
                mail->Servers[index].MinTLS > MAX_MAIL_TLS_VERSION) {
                log_this(SR_CONFIG,
                         "MailRelay.Servers[%d].MinTLS out of range [%d, %d], using default %d",
                         LOG_LEVEL_ERROR, 5, index, MIN_MAIL_TLS_VERSION, MAX_MAIL_TLS_VERSION,
                         MAIL_TLS_DEFAULT_MIN_VERSION);
                mail->Servers[index].MinTLS = MAIL_TLS_DEFAULT_MIN_VERSION;
            }
            success = success && PROCESS_STRING(server, &mail->Servers[index], CAPath, "CAPath", "MailRelay");
            success = success && PROCESS_INT(server, &mail->Servers[index], AuthMode, "AuthMode", "MailRelay");
            success = success && PROCESS_INT(server, &mail->Servers[index], TimeoutSeconds, "TimeoutSeconds", "MailRelay");

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

    cleanup_mail_relay_security(&config->Security);

    for (int i = 0; i < config->AdminRecipientCount; i++) {
        free(config->AdminRecipients[i]);
        config->AdminRecipients[i] = NULL;
    }
    config->AdminRecipientCount = 0;

    cleanup_mail_relay_events(&config->Events);
    cleanup_mail_relay_test(&config->Test);

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
    DUMP_BOOL2("――", "InboundRequireAuth", config->InboundRequireAuth);

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
    snprintf(buffer, sizeof(buffer), "Stale Timeout: %d seconds", config->Queue.StaleTimeoutSeconds);
    DUMP_TEXT("――――", buffer);

    // Dump template settings
    DUMP_TEXT("――", "Template Settings");
    snprintf(buffer, sizeof(buffer), "Reload Interval: %d seconds", config->Templates.ReloadIntervalSeconds);
    DUMP_TEXT("――――", buffer);

    // Dump OTP settings
    DUMP_TEXT("――", "OTP Settings");
    snprintf(buffer, sizeof(buffer), "Digits: %d", config->Otp.Digits);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Expiry: %d seconds", config->Otp.ExpirySeconds);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Attempts: %d", config->Otp.MaxAttempts);
    DUMP_TEXT("――――", buffer);

    // Dump events settings
    DUMP_TEXT("――", "Events Settings");
    snprintf(buffer, sizeof(buffer), "Enabled: %s", config->Events.Enabled ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Events Per Interval: %d", config->Events.MaxEventsPerInterval);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Event Interval Seconds: %d", config->Events.EventIntervalSeconds);
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

    // Dump test settings
    DUMP_TEXT("――", "Test Settings");
    snprintf(buffer, sizeof(buffer), "SendRawOnLaunch: %s", config->Test.SendRawOnLaunch ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "SendOtpOnLaunch: %s", config->Test.SendOtpOnLaunch ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "FailNextSendOnLaunch: %s", config->Test.FailNextSendOnLaunch ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "MailRepoProbeOnLaunch: %s", config->Test.MailRepoProbeOnLaunch ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    if (config->Test.TestFrom) {
        snprintf(buffer, sizeof(buffer), "TestFrom: %s", config->Test.TestFrom);
        DUMP_TEXT("――――", buffer);
    }
    if (config->Test.TestTo) {
        snprintf(buffer, sizeof(buffer), "TestTo: %s", config->Test.TestTo);
        DUMP_TEXT("――――", buffer);
    }
    if (config->Test.TestSubject) {
        snprintf(buffer, sizeof(buffer), "TestSubject: %s", config->Test.TestSubject);
        DUMP_TEXT("――――", buffer);
    }
    if (config->Test.TestBody) {
        snprintf(buffer, sizeof(buffer), "TestBody: %s", config->Test.TestBody);
        DUMP_TEXT("――――", buffer);
    }

    // Dump security settings
    DUMP_TEXT("――", "Security Settings");
    if (config->Security.SenderPolicy == MAIL_SENDER_POLICY_ALLOWLIST) {
        DUMP_TEXT("――――", "Sender Policy: allowlist");
    } else if (config->Security.SenderPolicy == MAIL_SENDER_POLICY_BLOCKLIST) {
        DUMP_TEXT("――――", "Sender Policy: blocklist");
    } else {
        DUMP_TEXT("――――", "Sender Policy: allow all");
    }
    snprintf(buffer, sizeof(buffer), "Allowed Senders: %d", config->Security.AllowSenderCount);
    DUMP_TEXT("――――", buffer);
    for (int i = 0; i < config->Security.AllowSenderCount; i++) {
        snprintf(buffer, sizeof(buffer), "  Allow %d: %s", i + 1, config->Security.AllowSenders[i]);
        DUMP_TEXT("――――――", buffer);
    }
    snprintf(buffer, sizeof(buffer), "Blocked Senders: %d", config->Security.BlockSenderCount);
    DUMP_TEXT("――――", buffer);
    for (int i = 0; i < config->Security.BlockSenderCount; i++) {
        snprintf(buffer, sizeof(buffer), "  Block %d: %s", i + 1, config->Security.BlockSenders[i]);
        DUMP_TEXT("――――――", buffer);
    }

    // Dump rate-limit settings (Phase 14.3)
    DUMP_TEXT("――", "Rate Limit Settings");
    snprintf(buffer, sizeof(buffer), "Enabled: %s", config->RateLimit.Enabled ? "true" : "false");
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Scope: %d", config->RateLimit.Scope);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Max Requests Per Interval: %d", config->RateLimit.MaxRequestsPerInterval);
    DUMP_TEXT("――――", buffer);
    snprintf(buffer, sizeof(buffer), "Interval Seconds: %d", config->RateLimit.IntervalSeconds);
    DUMP_TEXT("――――", buffer);

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

        snprintf(buffer, sizeof(buffer), "MinTLS: %d", server->MinTLS);
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "CAPath: %s", server->CAPath ? server->CAPath : "(not set)");
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "AuthMode: %d", server->AuthMode);
        DUMP_TEXT("――――――", buffer);

        snprintf(buffer, sizeof(buffer), "Timeout: %d seconds", server->TimeoutSeconds);
        DUMP_TEXT("――――――", buffer);
    }
}
