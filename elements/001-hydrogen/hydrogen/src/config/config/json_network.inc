/*
 * Network configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_network.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Network Configuration
json_t* network = json_object_get(root, "Network");
if (json_is_object(network)) {
    log_config_section_header("Network");
    
    json_t* interfaces = json_object_get(network, "Interfaces");
    if (json_is_object(interfaces)) {
        log_config_section_item("Interfaces", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(interfaces, "MaxInterfaces");
        config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
        log_config_section_item("MaxInterfaces", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_interfaces);
        
        val = json_object_get(interfaces, "MaxIPsPerInterface");
        config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
        log_config_section_item("MaxIPsPerInterface", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_ips_per_interface);
        
        val = json_object_get(interfaces, "MaxInterfaceNameLength");
        config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
        log_config_section_item("MaxInterfaceNameLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_interface_name_length);
        
        val = json_object_get(interfaces, "MaxIPAddressLength");
        config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
        log_config_section_item("MaxIPAddressLength", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.max_ip_address_length);
    }

    json_t* port_allocation = json_object_get(network, "PortAllocation");
    if (json_is_object(port_allocation)) {
        log_config_section_item("PortAllocation", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(port_allocation, "StartPort");
        config->network.start_port = get_config_int(val, DEFAULT_WEB_PORT);
        log_config_section_item("StartPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.start_port);
        
        val = json_object_get(port_allocation, "EndPort");
        config->network.end_port = get_config_int(val, 65535);
        log_config_section_item("EndPort", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->network.end_port);
        log_config_section_item("ReservedPorts", "%zu ports reserved", LOG_LEVEL_STATE, 0, 1, NULL, NULL, config->network.reserved_ports_count);
        
        json_t* reserved_ports = json_object_get(port_allocation, "ReservedPorts");
        if (json_is_array(reserved_ports)) {
            config->network.reserved_ports_count = json_array_size(reserved_ports);
            config->network.reserved_ports = malloc(sizeof(int) * config->network.reserved_ports_count);
            for (size_t i = 0; i < config->network.reserved_ports_count; i++) {
                config->network.reserved_ports[i] = json_integer_value(json_array_get(reserved_ports, i));
            }
        }
    }
} else {
    config->network.max_interfaces = DEFAULT_MAX_INTERFACES;
    config->network.max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
    config->network.max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
    config->network.max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;
    config->network.start_port = DEFAULT_WEB_PORT;
    config->network.end_port = 65535;
    config->network.reserved_ports = NULL;
    config->network.reserved_ports_count = 0;
    
    log_config_section_header("Network");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}