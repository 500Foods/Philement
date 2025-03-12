/*
 * mDNS Server configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_mdns_server.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// mDNS Server Configuration
json_t* mdns_server = json_object_get(root, "mDNSServer");
if (json_is_object(mdns_server)) {
    log_config_section_header("mDNSServer");
    
    json_t* enabled = json_object_get(mdns_server, "Enabled");
    config->mdns_server.enabled = get_config_bool(enabled, 1);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
             config->mdns_server.enabled ? "true" : "false");

    json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
    config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);
    log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL,
             config->mdns_server.enable_ipv6 ? "true" : "false");

    json_t* device_id = json_object_get(mdns_server, "DeviceId");
    config->mdns_server.device_id = get_config_string_with_env("DeviceId", device_id, "hydrogen-printer");
    log_config_section_item("DeviceId", "%s", LOG_LEVEL_STATE, !device_id, 0, NULL, NULL, config->mdns_server.device_id);

    json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
    config->mdns_server.friendly_name = get_config_string_with_env("FriendlyName", friendly_name, "Hydrogen 3D Printer");
    log_config_section_item("FriendlyName", "%s", LOG_LEVEL_STATE, !friendly_name, 0, NULL, NULL, config->mdns_server.friendly_name);

    json_t* model = json_object_get(mdns_server, "Model");
    config->mdns_server.model = get_config_string_with_env("Model", model, "Hydrogen");
    log_config_section_item("Model", "%s", LOG_LEVEL_STATE, !model, 0, NULL, NULL, config->mdns_server.model);

    json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
    config->mdns_server.manufacturer = get_config_string_with_env("Manufacturer", manufacturer, "Philement");
    log_config_section_item("Manufacturer", "%s", LOG_LEVEL_STATE, !manufacturer, 0, NULL, NULL, config->mdns_server.manufacturer);

    json_t* version = json_object_get(mdns_server, "Version");
    config->mdns_server.version = get_config_string_with_env("Version", version, VERSION);
    log_config_section_item("Version", "%s", LOG_LEVEL_STATE, !version, 0, NULL, NULL, config->mdns_server.version);
    
    json_t* services = json_object_get(mdns_server, "Services");
    if (json_is_array(services)) {
        config->mdns_server.num_services = json_array_size(services);
        log_config_section_item("Services", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->mdns_server.num_services);
        config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));
        for (size_t i = 0; i < config->mdns_server.num_services; i++) {
            json_t* service = json_array_get(services, i);
            if (!json_is_object(service)) continue;
            // Get service properties
            json_t* name = json_object_get(service, "Name");
            config->mdns_server.services[i].name = get_config_string_with_env("Name", name, "hydrogen");
            
            json_t* type = json_object_get(service, "Type");
            config->mdns_server.services[i].type = get_config_string_with_env("Type", type, "_http._tcp.local");

            json_t* port = json_object_get(service, "Port");
            config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);
            
            // Log service details after all properties are populated
            log_config_section_item("Service", "%s: %s on port %d", LOG_LEVEL_STATE, 0, 1, NULL, NULL,
                                   config->mdns_server.services[i].name,
                                   config->mdns_server.services[i].type,
                                   config->mdns_server.services[i].port);

            json_t* txt_records = json_object_get(service, "TxtRecords");
            if (json_is_string(txt_records)) {
                config->mdns_server.services[i].num_txt_records = 1;
                config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                config->mdns_server.services[i].txt_records[0] = get_config_string_with_env("TxtRecord", txt_records, "");
            } else if (json_is_array(txt_records)) {
                config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                config->mdns_server.services[i].txt_records = malloc(config->mdns_server.services[i].num_txt_records * sizeof(char*));
                for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                    json_t* record = json_array_get(txt_records, j);
                    config->mdns_server.services[i].txt_records[j] = get_config_string_with_env("TxtRecord", record, "");
                }
            } else {
                config->mdns_server.services[i].num_txt_records = 0;
                config->mdns_server.services[i].txt_records = NULL;
            }
        }
    }
} else {
    log_config_section_header("mDNSServer");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}