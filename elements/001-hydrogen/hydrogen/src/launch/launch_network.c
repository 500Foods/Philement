/*
 * Network Subsystem Launch Readiness Check
 * 
 * This file implements the launch readiness check for the network subsystem.
 * It verifies that network interfaces are available and properly configured.
 */

 // Global includes
 #include <src/hydrogen.h>
 #include <string.h>
 #include <arpa/inet.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <unistd.h>
 #include <netinet/ip_icmp.h>
 #include <netinet/ip.h>
 
 // Local includes
 #include "launch.h"
 #include <src/network/network.h>

// Network subsystem shutdown flag
volatile sig_atomic_t network_system_shutdown = 0;

// Use the proper interface timing function from network_linux.c

// Registry ID and cached readiness state
int network_subsystem_id = -1;

// Register the network subsystem with the registry (for readiness)
static void register_network(void) {
    if (network_subsystem_id < 0) {
        network_subsystem_id = register_subsystem(SR_NETWORK, NULL, NULL, NULL, NULL, NULL);
    }
}

/*
 * Check if the network subsystem is ready to launch
 * 
 * This function performs readiness checks for the network subsystem by:
 * - Verifying system state and dependencies
 * - Checking network interface availability
 * - Validating interface configuration
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Returns:
 * LaunchReadiness structure containing:
 * - ready: true if system is ready to launch
 * - messages: NULL-terminated array of status messages (must be freed by caller on success path)
 * - subsystem: Name of the subsystem ("Network")
 */

// Check if the network subsystem is ready to launch
LaunchReadiness check_network_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_NETWORK));

    // Register with registry if not already registered
    register_network();
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      " SR_NETWORK " subsystem registered"));

    // Register Thread dependency - we only need to verify it's ready for launch
    if (!add_dependency_from_launch(network_subsystem_id, SR_THREADS)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register " SR_THREADS " dependency"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      " SR_THREADS " dependency registered"));

    // Early return cases
    if (server_stopping || web_server_shutdown) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System shutdown in progress"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (system shutdown)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    if (!server_starting && !server_running) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System not in startup or running state"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid system state)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (no configuration)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    // Get network limits for validation
    const NetworkLimits* limits = get_network_limits();

    // Validate interface and IP limits
    if (app_config->network.max_interfaces < limits->min_interfaces ||
        app_config->network.max_interfaces > limits->max_interfaces) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid max_interfaces: %zu (must be between %zu and %zu)",
                    app_config->network.max_interfaces, limits->min_interfaces, limits->max_interfaces);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid max_interfaces)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    if (app_config->network.max_ips_per_interface < limits->min_ips_per_interface ||
        app_config->network.max_ips_per_interface > limits->max_ips_per_interface) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid max_ips_per_interface: %zu (must be between %zu and %zu)",
                    app_config->network.max_ips_per_interface, limits->min_ips_per_interface, limits->max_ips_per_interface);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid max_ips_per_interface)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    // Validate name and address length limits
    if (app_config->network.max_interface_name_length < limits->min_interface_name_length ||
        app_config->network.max_interface_name_length > limits->max_interface_name_length) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid interface name length: %zu (must be between %zu and %zu)",
                    app_config->network.max_interface_name_length, limits->min_interface_name_length, limits->max_interface_name_length);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid interface name length)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    if (app_config->network.max_ip_address_length < limits->min_ip_address_length ||
        app_config->network.max_ip_address_length > limits->max_ip_address_length) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid IP address length: %zu (must be between %zu and %zu)",
                    app_config->network.max_ip_address_length, limits->min_ip_address_length, limits->max_ip_address_length);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid IP address length)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    // Validate port range
    if (app_config->network.start_port < limits->min_port ||
        app_config->network.start_port > limits->max_port ||
        app_config->network.end_port < limits->min_port ||
        app_config->network.end_port > limits->max_port ||
        app_config->network.start_port >= app_config->network.end_port) {
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "  No-Go:   Invalid port range: %d-%d (must be between %d and %d, start < end)",
                    app_config->network.start_port, app_config->network.end_port, limits->min_port, limits->max_port);
            add_launch_message(&messages, &count, &capacity, msg);
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (invalid port range)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    // Validate reserved ports array
    if (app_config->network.reserved_ports_count > 0 && !app_config->network.reserved_ports) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Reserved ports array is NULL but count > 0"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (reserved ports array issue)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    add_launch_message(&messages, &count, &capacity, strdup("  Go:      " SR_NETWORK " configuration validated"));

    network_info_t* network_info = get_network_info();
    if (!network_info) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to get " SR_NETWORK " configuration"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (no network information)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    if (network_info->count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No " SR_NETWORK " interfaces available"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (no interfaces)"));
        free_network_info(network_info);
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_NETWORK, .ready = false, .messages = messages };
    }

    char* count_msg = malloc(256);
    if (count_msg) {
        snprintf(count_msg, 256, "  Go:      %d " SR_NETWORK " interfaces available", network_info->count);
        add_launch_message(&messages, &count, &capacity, count_msg);
    }

    // Check for "all" interfaces configuration
    bool all_interfaces_enabled = false;
    if (app_config && app_config->network.available_interfaces &&
        app_config->network.available_interfaces_count == 1 &&
        app_config->network.available_interfaces[0].interface_name &&
        strcmp(app_config->network.available_interfaces[0].interface_name, "all") == 0 &&
        app_config->network.available_interfaces[0].available) {
        all_interfaces_enabled = true;
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      All " SR_NETWORK " interfaces enabled via config"));
    }

    // Check for specifically configured interfaces if not using "all"
    if (!all_interfaces_enabled && app_config->network.available_interfaces &&
        app_config->network.available_interfaces_count > 0) {
        int json_interfaces_count = (int)app_config->network.available_interfaces_count;
        char* config_count_msg = malloc(256);
        if (config_count_msg) {
            snprintf(config_count_msg, 256, "  Go:      %d " SR_NETWORK " interfaces configured:", json_interfaces_count);
            add_launch_message(&messages, &count, &capacity, config_count_msg);
        }

        // List specifically configured interfaces
        for (size_t i = 0; i < app_config->network.available_interfaces_count; i++) {
            if (app_config->network.available_interfaces[i].interface_name) {
                char* interface_msg = malloc(256);
                if (interface_msg) {
                    const char* interface_name = app_config->network.available_interfaces[i].interface_name;
                    bool is_available = app_config->network.available_interfaces[i].available;
                    if (is_available) {
                        snprintf(interface_msg, 256, "  Go:      Available: %s is enabled", interface_name);
                    } else {
                        snprintf(interface_msg, 256, "  No-Go:   Available: %s is disabled", interface_name);
                    }
                    add_launch_message(&messages, &count, &capacity, interface_msg);
                }
            }
        }
    } else if (!all_interfaces_enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      No specific interfaces configured - using defaults"));
    }

    // List interfaces sorted by name without checking up/down status
    // Create a sorted copy of interfaces for consistent display
    typedef struct {
        char name[IF_NAMESIZE];
        bool is_available;
        bool is_configured;
        char config_status[128];
        char ipv4_ips[MAX_IPS][INET_ADDRSTRLEN];
        char ipv6_ips[MAX_IPS][INET6_ADDRSTRLEN];
        int ipv4_count;
        int ipv6_count;
    } sorted_interface_t;

    sorted_interface_t sorted_interfaces[MAX_INTERFACES];
    int sorted_count = 0;

    // Collect and categorize interfaces
    for (int i = 0; i < network_info->count && sorted_count < MAX_INTERFACES; i++) {
        const interface_t* interface = &network_info->interfaces[i];
        bool is_available = all_interfaces_enabled;

        // If not all interfaces enabled, check specific configuration
        if (!all_interfaces_enabled) {
            bool is_configured = is_interface_configured(interface->name, &is_available);
            if (!is_configured) {
                is_available = true; // Not configured means enabled by default
            }
        }

        const char* config_status = all_interfaces_enabled ? "enabled via all:true" :
            (is_interface_configured(interface->name, NULL) ?
                (is_available ? "enabled in config" : "disabled in config") :
                "not in config - enabled by default");

        // Copy interface data
        strncpy(sorted_interfaces[sorted_count].name, interface->name, IF_NAMESIZE - 1);
        sorted_interfaces[sorted_count].name[IF_NAMESIZE - 1] = '\0';
        sorted_interfaces[sorted_count].is_available = is_available;
        sorted_interfaces[sorted_count].is_configured = is_interface_configured(interface->name, NULL);
        strncpy(sorted_interfaces[sorted_count].config_status, config_status, 127);
        sorted_interfaces[sorted_count].config_status[127] = '\0';

        // Separate IPv4 and IPv6 addresses
        sorted_interfaces[sorted_count].ipv4_count = 0;
        sorted_interfaces[sorted_count].ipv6_count = 0;

        for (int j = 0; j < interface->ip_count && j < MAX_IPS; j++) {
            if (interface->is_ipv6[j]) {
                strncpy(sorted_interfaces[sorted_count].ipv6_ips[sorted_interfaces[sorted_count].ipv6_count],
                       interface->ips[j], INET6_ADDRSTRLEN - 1);
                sorted_interfaces[sorted_count].ipv6_ips[sorted_interfaces[sorted_count].ipv6_count][INET6_ADDRSTRLEN - 1] = '\0';
                sorted_interfaces[sorted_count].ipv6_count++;
            } else {
                strncpy(sorted_interfaces[sorted_count].ipv4_ips[sorted_interfaces[sorted_count].ipv4_count],
                       interface->ips[j], INET_ADDRSTRLEN - 1);
                sorted_interfaces[sorted_count].ipv4_ips[sorted_interfaces[sorted_count].ipv4_count][INET_ADDRSTRLEN - 1] = '\0';
                sorted_interfaces[sorted_count].ipv4_count++;
            }
        }

        sorted_count++;
    }

    // Sort interfaces by name
    for (int i = 0; i < sorted_count - 1; i++) {
        for (int j = i + 1; j < sorted_count; j++) {
            if (strcmp(sorted_interfaces[i].name, sorted_interfaces[j].name) > 0) {
                sorted_interface_t temp = sorted_interfaces[i];
                sorted_interfaces[i] = sorted_interfaces[j];
                sorted_interfaces[j] = temp;
            }
        }
    }

    // Display sorted interfaces with IPv4 and IPv6 separately
    // Note: Loopback interface (lo) is shown here for completeness but will be excluded during launch testing
    for (int i = 0; i < sorted_count; i++) {
        const sorted_interface_t* iface = &sorted_interfaces[i];

        // Show interface name and configuration status using separate messages
        // add_launch_message(&messages, &count, &capacity, strdup("  Info:    Interface listing:"));
        // add_launch_message(&messages, &count, &capacity, strdup("  Info:    ├─ Name: "));

        // char name_msg[256];
        // snprintf(name_msg, sizeof(name_msg), "  Go:      %.50s", iface->name);
        // add_launch_message(&messages, &count, &capacity, strdup(name_msg));

        // add_launch_message(&messages, &count, &capacity, strdup("  Info:    ├─ Configuration:"));
        // add_launch_message(&messages, &count, &capacity, strdup("  Info:    │  └─ Status: enabled"));
        // add_launch_message(&messages, &count, &capacity, strdup("  Info:    │  └─ Rationale: available for use"));

        // List IPv4 addresses if any
        // if (iface->ipv4_count > 0) {
        //     add_launch_message(&messages, &count, &capacity, strdup("  Info:    ├─ IPv4 addresses:"));

        //     for (int j = 0; j < iface->ipv4_count; j++) {
        //         char ipv4_addr_msg[256];
        //         snprintf(ipv4_addr_msg, sizeof(ipv4_addr_msg), "  Info:    │  └─ %s", iface->ipv4_ips[j]);
        //         add_launch_message(&messages, &count, &capacity, strdup(ipv4_addr_msg));
        //     }
        // }

        // List IPv6 addresses if any
        // if (iface->ipv6_count > 0) {
        //     add_launch_message(&messages, &count, &capacity, strdup("  Info:    ├─ IPv6 addresses:"));

        //     for (int j = 0; j < iface->ipv6_count; j++) {
        //         char ipv6_addr_msg[256];
        //         snprintf(ipv6_addr_msg, sizeof(ipv6_addr_msg), "  Info:    │  └─ %s", iface->ipv6_ips[j]);
        //         add_launch_message(&messages, &count, &capacity, strdup(ipv6_addr_msg));
        //     }
        // }

        // Show availability status
        if (iface->is_available) {
            char name_msg[256];
            snprintf(name_msg, sizeof(name_msg), "  Go:      %.50s", iface->name);
            add_launch_message(&messages, &count, &capacity, strdup(name_msg));
        //      add_launch_message(&messages, &count, &capacity, strdup("  Go:      %Available for use"));
         } else {
            char name_msg[256];
            snprintf(name_msg, sizeof(name_msg), "  No-Go:   %.50s", iface->name);
            add_launch_message(&messages, &count, &capacity, strdup(name_msg));
        //     add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   └─ Not available for use"));
        }
    }

    // Count available interfaces for readiness decision
    int available_interfaces = 0;
    for (int i = 0; i < sorted_count; i++) {
        if (sorted_interfaces[i].is_available) {
            available_interfaces++;
        }
    }

    if (available_interfaces > 0) {
        char* decision_msg = malloc(256);
        if (decision_msg) {
            snprintf(decision_msg, 256, "  Decide:  Go For Launch of " SR_NETWORK " Subsystem (%d interfaces available)", available_interfaces);
            add_launch_message(&messages, &count, &capacity, decision_msg);
        }
        // Mark as ready for launch - this enables dependent subsystems to detect network availability
        update_subsystem_state(network_subsystem_id, SUBSYSTEM_READY);
//        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem marked as ready"));
        ready = true;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_NETWORK " Subsystem (no interfaces available)"));
        ready = false;
    }

    free_network_info(network_info);
    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_NETWORK,
        .ready = ready,
        .messages = messages
    };
}

// Network subsystem launch function
int launch_network_subsystem(void) {

    // Log Launch
    log_this(SR_NETWORK, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK, LOG_LEVEL_DEBUG, 0);
    
    // Verify system state
    if (server_stopping || server_starting != 1) {
        log_this(SR_NETWORK, "Cannot initialize " SR_NETWORK " subsystem outside startup phase", LOG_LEVEL_DEBUG, 0);
        log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK " FAILED: Not in startup phase", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Verify config state
    if (!app_config) {
        log_this(SR_NETWORK, "Configuration not loaded", LOG_LEVEL_DEBUG, 0);
        log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK " FAILED: No configuration", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Register with launch handlers if not already registered
    if (network_subsystem_id < 0) {
        network_subsystem_id = register_subsystem_from_launch(SR_NETWORK, NULL, NULL, NULL,
                                                            (int (*)(void))launch_network_subsystem,
                                                            (void (*)(void))shutdown_network_subsystem);
        if (network_subsystem_id < 0) {
            log_this(SR_NETWORK, "Failed to register " SR_NETWORK " subsystem", LOG_LEVEL_DEBUG, 0);
            return 0;
        }
    }
    
    // Enumerate and test network interfaces in one unified display
    log_this(SR_NETWORK, "Enumerating " SR_NETWORK " interfaces", LOG_LEVEL_DEBUG, 0);
    network_info_t *info = get_network_info();
    if (!info) {
        log_this(SR_NETWORK, "Failed to get " SR_NETWORK " configuration", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Sort interfaces by name for consistent display
    for (int i = 0; i < info->count - 1; i++) {
        for (int j = i + 1; j < info->count; j++) {
            if (strcmp(info->interfaces[i].name, info->interfaces[j].name) > 0) {
                interface_t temp = info->interfaces[i];
                info->interfaces[i] = info->interfaces[j];
                info->interfaces[j] = temp;
            }
        }
    }

    // Test network interfaces and collect runtime information
    // log_this(SR_NETWORK, "Testing " SR_NETWORK " interfaces:", LOG_LEVEL_DEBUG, 0);
    bool ping_success = test_network_interfaces_quiet(info, true);

    // Create runtime interface information structure
    typedef struct {
        char name[IF_NAMESIZE];
        char ip_address[INET6_ADDRSTRLEN];
        bool is_ipv6;
        bool is_up;
        int mtu;
        bool is_linklocal;
        double ping_ms;
    } runtime_interface_t;

    runtime_interface_t runtime_interfaces[MAX_INTERFACES * MAX_IPS];
    int runtime_count = 0;

    // Collect runtime information for each interface (including loopback)
    for (int i = 0; i < info->count && runtime_count < MAX_INTERFACES * MAX_IPS; i++) {
        const interface_t* interface = &info->interfaces[i];

        // Check if interface is enabled in config
        bool is_available = true;
        bool is_configured = is_interface_configured(interface->name, &is_available);

        // Skip if explicitly disabled in config
        if (is_configured && !is_available) {
            continue;
        }

        // Test each IP address and collect runtime information
        for (int j = 0; j < interface->ip_count; j++) {
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, interface->name, IFNAMSIZ - 1);

            bool is_ipv6 = interface->is_ipv6[j];
            int mtu = 0;

            // Use the public test_interface function from network_linux.c
            // Since it's static there, we'll need to create a wrapper or use ioctl directly
            int sockfd;
            bool interface_up = false;

            // Create socket for ioctl operations
            if (is_ipv6) {
                sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
            } else {
                sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            }

            if (sockfd >= 0) {
                // Set timeout
                struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                // Get interface flags
                if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) >= 0) {
                    // Check if interface is up and running
                    if ((ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING)) {
                        interface_up = true;
                        ping_success = true;
                    }
                }

                // Get interface MTU
                if (ioctl(sockfd, SIOCGIFMTU, &ifr) >= 0) {
                    mtu = ifr.ifr_mtu;
                }

                close(sockfd);
            }

            // Detect if IPv6 address is linklocal
            bool is_linklocal = false;
            if (is_ipv6) {
                // Check if it's a linklocal address (fe80::/10 prefix)
                struct in6_addr addr6;
                if (inet_pton(AF_INET6, interface->ips[j], &addr6) == 1) {
                    const uint8_t* bytes = addr6.s6_addr;
                    // Linklocal addresses start with fe80::/10 (first 10 bits: 1111111010)
                    is_linklocal = (bytes[0] == 0xfe) && ((bytes[1] & 0xc0) == 0x80);
                }
            }

            // Get actual ping time using the proper interface timing function
            double actual_ping_ms = 0.0;
            if (interface_up) {
                // Use the interface_time function from network_linux.c
                // Since it's static there, we need to call it directly
                actual_ping_ms = interface_time(interface->ips[j]);
            }

            // Store runtime information
            strncpy(runtime_interfaces[runtime_count].name, interface->name, IF_NAMESIZE - 1);
            runtime_interfaces[runtime_count].name[IF_NAMESIZE - 1] = '\0';
            strncpy(runtime_interfaces[runtime_count].ip_address, interface->ips[j], INET6_ADDRSTRLEN - 1);
            runtime_interfaces[runtime_count].ip_address[INET6_ADDRSTRLEN - 1] = '\0';
            runtime_interfaces[runtime_count].is_ipv6 = is_ipv6;
            runtime_interfaces[runtime_count].is_up = interface_up;
            runtime_interfaces[runtime_count].mtu = mtu;
            runtime_interfaces[runtime_count].is_linklocal = is_linklocal;
            runtime_interfaces[runtime_count].ping_ms = actual_ping_ms;

            runtime_count++;
        }
    }

    // Sort runtime interfaces by name for consistent display
    for (int i = 0; i < runtime_count - 1; i++) {
        for (int j = i + 1; j < runtime_count; j++) {
            if (strcmp(runtime_interfaces[i].name, runtime_interfaces[j].name) > 0) {
                runtime_interface_t temp = runtime_interfaces[i];
                runtime_interfaces[i] = runtime_interfaces[j];
                runtime_interfaces[j] = temp;
            }
        }
    }

    // Display unified interface information
    // log_this(SR_NETWORK, "Available " SR_NETWORK " interfaces:", LOG_LEVEL_DEBUG, 0);
    int up_count = 0;
    for (int i = 0; i < runtime_count; i++) {
        const runtime_interface_t* runtime = &runtime_interfaces[i];
        const char* ip_type = runtime->is_ipv6 ? "IPv6" : "IPv4";

        // Interface line without state
        log_this(SR_NETWORK, "― %.10s", LOG_LEVEL_DEBUG, 1, runtime->name);

        // IP address
        log_this(SR_NETWORK, "――― %s:  %s", LOG_LEVEL_DEBUG, 2, ip_type, runtime->ip_address);

        // State
        const char* status = runtime->is_up ? "up" : "down";
        log_this(SR_NETWORK, "――― State: %s", LOG_LEVEL_DEBUG, 1, status);

        // MTU and Ping time (only for up interfaces)
        if (runtime->is_up) {
            log_this(SR_NETWORK, "――― MTU:   %d", LOG_LEVEL_DEBUG, 1, runtime->mtu);
            log_this(SR_NETWORK, "――― Ping:  %.6fms", LOG_LEVEL_DEBUG, 1, runtime->ping_ms);
            up_count++;
        }

        // LinkLocal status (only for IPv6)
        if (runtime->is_ipv6) {
            log_this(SR_NETWORK, "――― Link:  %s", LOG_LEVEL_DEBUG, 1,
                     runtime->is_linklocal ? "true" : "false");
        }
    }

    log_this(SR_NETWORK, "Availabe interfaces: %d", LOG_LEVEL_DEBUG, 1, up_count);

    free_network_info(info);

    if (!ping_success) {
        log_this(SR_NETWORK, "No " SR_NETWORK " interfaces responded to ping", LOG_LEVEL_DEBUG, 0);
        log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK " FAILED: No usable interfaces", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    
    // Update registry and verify state
    log_this(SR_NETWORK, "Updating " SR_NETWORK " in " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_NETWORK, true);
    
    SubsystemState final_state = get_subsystem_state(network_subsystem_id);
  
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK " COMPLETE", LOG_LEVEL_DEBUG, 0);
        return 1;
    } else {
        log_this(SR_NETWORK, "LAUNCH: " SR_NETWORK " WARNING: Unexpected final state: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}

