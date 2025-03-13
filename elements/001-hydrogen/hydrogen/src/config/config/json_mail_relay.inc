/*
 * Mail Relay configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c, with all necessary variables already in scope.
 */

json_t* mail_relay = json_object_get(root, "MailRelay");
// Also check for common typo with space
if (!json_is_object(mail_relay)) {
    mail_relay = json_object_get(root, " MailRelay");
}

if (json_is_object(mail_relay)) {
    log_config_section_header("MailRelay");
    
    json_t* enabled = json_object_get(mail_relay, "Enabled");
    bool mail_relay_enabled = get_config_bool(enabled, true);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
            mail_relay_enabled ? "true" : "false");
    
    json_t* listen_port = json_object_get(mail_relay, "ListenPort");
    if (json_is_integer(listen_port)) {
        log_config_section_item("ListenPort", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, 
            json_integer_value(listen_port));
    }
    
    json_t* workers = json_object_get(mail_relay, "Workers");
    if (json_is_integer(workers)) {
        log_config_section_item("Workers", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, 
            json_integer_value(workers));
    }
    
    json_t* outbound_servers = json_object_get(mail_relay, "OutboundServers");
    if (json_is_array(outbound_servers)) {
        size_t server_count = json_array_size(outbound_servers);
        log_config_section_item("OutboundServers", "%zu Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, server_count);
        
        for (size_t i = 0; i < server_count && i < 3; i++) {  // Limit to 3 for display
            json_t* server = json_array_get(outbound_servers, i);
            if (!json_is_object(server)) continue;
            
            log_config_section_item("Server", "%d", LOG_LEVEL_STATE, 0, 1, NULL, NULL, (int)i + 1);
            
            // Create a server-specific key for clearer context
            char server_key[32];
            snprintf(server_key, sizeof(server_key), "SMTP_SERVER%zu", i+1);
            
            // Host with environment variable support
            json_t* host = json_object_get(server, "Host");
            char* host_value = get_config_string_with_env("Host", host, "smtp.example.com");
            log_config_section_item("Host", "%s", LOG_LEVEL_STATE, !host, 2, NULL, NULL, host_value);
            free(host_value);
            
            // Port with environment variable support
            json_t* port = json_object_get(server, "Port");
            if (json_is_integer(port)) {
                log_config_section_item("Port", "%d", LOG_LEVEL_STATE, 0, 2, NULL, NULL, json_integer_value(port));
            } else {
                char* port_value = get_config_string_with_env("Port", port, "25"); // Default SMTP port
                log_config_section_item("Port", "%s", LOG_LEVEL_STATE, !port, 2, NULL, NULL, port_value);
                free(port_value);
            }
            
            // Username with environment variable support
            json_t* username = json_object_get(server, "Username");
            char* username_value = get_config_string_with_env("Username", username, "");
            if (username_value && strlen(username_value) > 0) {
                log_config_section_item("Username", "configured", LOG_LEVEL_STATE, 0, 2, NULL, NULL);
            }
            free(username_value);
            
            // Password with environment variable support (not logged)
            json_t* password = json_object_get(server, "Password");
            char* password_value = get_config_string_with_env("Password", password, "");
            free(password_value); // Just process and free, don't log
            
            json_t* useTLS = json_object_get(server, "UseTLS");
            bool tls_enabled = get_config_bool(useTLS, true);
            log_config_section_item("UseTLS", "%s", LOG_LEVEL_STATE, !useTLS, 2, NULL, NULL,
                tls_enabled ? "true" : "false");
        }
        
        if (server_count > 3) {
            log_config_section_item("Note", "%zu additional servers not shown", LOG_LEVEL_STATE, 0, 1, NULL, NULL, 
                server_count - 3);
        }
    }
} else {
    log_config_section_header("MailRelay");
    log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}