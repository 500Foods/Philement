/*
 * Implementation of the Hydrogen server configuration system.
 * 
 * Handles loading and parsing of JSON configuration files, providing default values
 * when needed. Includes utilities for file operations and priority level management.
 * The configuration system supports web server, WebSocket, and mDNS settings with
 * flexible service discovery options.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "configuration.h"
#include "logging.h"

int MAX_PRIORITY_LABEL_WIDTH = 9;
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;

const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "INFO"},
    {1, "WARN"},
    {2, "DEBUG"},
    {3, "ERROR"},
    {4, "CRITICAL"}
};

char* get_executable_path() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return strdup(path);  // Return a dynamically allocated copy
    }
    return NULL;
}

long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

char* get_file_modification_time(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return NULL;
    }

    struct tm* tm_info = localtime(&st.st_mtime);
    if (tm_info == NULL) {
        return NULL;
    }

    char* time_str = malloc(20);  // YYYY-MM-DD HH:MM:SS\0
    if (time_str == NULL) {
        return NULL;
    }

    if (strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        free(time_str);
        return NULL;
    }

    return time_str;
}


void create_default_config(const char* config_path) {
    json_t* root = json_object();

    // Server Name
    json_object_set_new(root, "ServerName", json_string("Philement/hydrogen"));

    // Log File
    json_object_set_new(root, "LogFile", json_string("/var/log/hydrogen.log"));

    // Web Configuration
    json_t* web = json_object();
    json_object_set_new(web, "Port", json_integer(5000));
    json_object_set_new(web, "WebRoot", json_string("/home/asimard/lithium"));
    json_object_set_new(web, "UploadPath", json_string("/api/upload"));
    json_object_set_new(web, "UploadDir", json_string("/tmp/hydrogen_uploads"));
    json_object_set_new(web, "MaxUploadSize", json_integer(2147483648));
    json_object_set_new(web, "LogLevel", json_string("ALL"));
    json_object_set_new(root, "WebServer", web);

    // WebSocket Configuration
    json_t* websocket = json_object();
    json_object_set_new(websocket, "Port", json_integer(5001));
    json_object_set_new(websocket, "Key", json_string("default_key_change_me"));
    json_object_set_new(websocket, "Protocol", json_string("hydrogen-protocol"));
    json_object_set_new(websocket, "LogLevel", json_string("ALL"));
    json_object_set_new(root, "WebSocket", websocket);

    // mDNS Configuration
    json_t* mdns = json_object();
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string("0.1.0"));
    json_object_set_new(mdns, "LogLevel", json_string("ALL"));

    json_t* services = json_array();

    json_t* http_service = json_object();
    json_object_set_new(http_service, "Name", json_string("hydrogen"));
    json_object_set_new(http_service, "Type", json_string("_http._tcp.local"));
    json_object_set_new(http_service, "Port", json_integer(5000));
    json_object_set_new(http_service, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, http_service);

    json_t* octoprint_service = json_object();
    json_object_set_new(octoprint_service, "Name", json_string("hydrogen"));
    json_object_set_new(octoprint_service, "Type", json_string("_octoprint._tcp.local"));
    json_object_set_new(octoprint_service, "Port", json_integer(5000));
    json_object_set_new(octoprint_service, "TxtRecords", json_string("path=/api,version=1.1.0"));
    json_array_append_new(services, octoprint_service);

    json_t* websocket_service = json_object();
    json_object_set_new(websocket_service, "Name", json_string("Hydrogen"));
    json_object_set_new(websocket_service, "Type", json_string("_websocket._tcp.local"));
    json_object_set_new(websocket_service, "Port", json_integer(5001));
    json_object_set_new(websocket_service, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, websocket_service);

    json_object_set_new(mdns, "Services", services);
    json_object_set_new(root, "mDNS", mdns);

    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        log_this("Configuration", "Error: Unable to create default config at %s", 3, true, true, true, config_path);
    } else {
        log_this("Configuration", "Created default config at %s", 0, true, true, true, config_path);
    }

    json_decref(root);
}

AppConfig* load_config(const char* config_path) {
    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        log_this("Configuration", "Failed to load config file: %s", 3, true, true, true, error.text);
        return NULL;
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Configuration", "Failed to allocate memory for config", 3, true, true, true);
        json_decref(root);
        return NULL;
    }

    // Server Name
    json_t* server_name = json_object_get(root, "ServerName");
    if (json_is_string(server_name)) {
        config->server_name = strdup(json_string_value(server_name));
    }

    // Log File
    json_t* log_file = json_object_get(root, "LogFile");
    if (json_is_string(log_file)) {
        config->log_file_path = strdup(json_string_value(log_file));
    }

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        json_t* port = json_object_get(web, "Port");
        config->web.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;

        json_t* web_root = json_object_get(web, "WebRoot");
        const char* web_root_str = json_is_string(web_root) ? json_string_value(web_root) : "/var/www/html";
        config->web.web_root = strdup(web_root_str);

        json_t* upload_path = json_object_get(web, "UploadPath");
        const char* upload_path_str = json_is_string(upload_path) ? json_string_value(upload_path) : DEFAULT_UPLOAD_PATH;
        config->web.upload_path = strdup(upload_path_str);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        const char* upload_dir_str = json_is_string(upload_dir) ? json_string_value(upload_dir) : DEFAULT_UPLOAD_DIR;
        config->web.upload_dir = strdup(upload_dir_str);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = json_is_integer(max_upload_size) ? 
            (size_t)json_integer_value(max_upload_size) : DEFAULT_MAX_UPLOAD_SIZE;

        json_t* log_level = json_object_get(web, "LogLevel");
        const char* log_level_str = json_is_string(log_level) ? json_string_value(log_level) : "ALL";
        config->web.log_level = strdup(log_level_str);
    } else {
        // Use defaults if web section is missing
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.log_level = strdup("ALL");
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEBSOCKET_PORT;

        json_t* key = json_object_get(websocket, "Key");
        const char* key_str = json_is_string(key) ? json_string_value(key) : "default_key";
        config->websocket.key = strdup(key_str);

        json_t* protocol = json_object_get(websocket, "Protocol");
        const char* protocol_str = json_is_string(protocol) ? json_string_value(protocol) : "hydrogen-protocol";
        config->websocket.protocol = strdup(protocol_str);

        json_t* max_message_mb = json_object_get(websocket, "MaxMessageMB");
        config->websocket.max_message_size = json_is_integer(max_message_mb) ? 
            json_integer_value(max_message_mb) * 1024 * 1024 : 10 * 1024 * 1024;  // Default to 10 MB

        json_t* log_level = json_object_get(websocket, "LogLevel");
        const char* log_level_str = json_is_string(log_level) ? json_string_value(log_level) : "ALL";
        config->websocket.log_level = strdup(log_level_str);
    } else {
        // Use defaults if websocket section is missing
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;  // Default to 10 MB
    }

    // mDNS Configuration
    json_t* mdns = json_object_get(root, "mDNS");
    if (json_is_object(mdns)) {
        json_t* device_id = json_object_get(mdns, "DeviceId");
        const char* device_id_str = json_is_string(device_id) ? json_string_value(device_id) : "hydrogen-printer";
        config->mdns.device_id = strdup(device_id_str);

        json_t* log_level = json_object_get(mdns, "LogLevel");
        const char* log_level_str = json_is_string(log_level) ? json_string_value(log_level) : "ALL";
        config->mdns.log_level = strdup(log_level_str);

        json_t* friendly_name = json_object_get(mdns, "FriendlyName");
        const char* friendly_name_str = json_is_string(friendly_name) ? json_string_value(friendly_name) : "Hydrogen 3D Printer";
        config->mdns.friendly_name = strdup(friendly_name_str);

        json_t* model = json_object_get(mdns, "Model");
        const char* model_str = json_is_string(model) ? json_string_value(model) : "Hydrogen";
        config->mdns.model = strdup(model_str);

        json_t* manufacturer = json_object_get(mdns, "Manufacturer");
        const char* manufacturer_str = json_is_string(manufacturer) ? json_string_value(manufacturer) : "Philement";
        config->mdns.manufacturer = strdup(manufacturer_str);

        json_t* version = json_object_get(mdns, "Version");
        const char* version_str = json_is_string(version) ? json_string_value(version) : VERSION;
        config->mdns.version = strdup(version_str);

        json_t* services = json_object_get(mdns, "Services");
	if (json_is_array(services)) {
            config->mdns.num_services = json_array_size(services);
            config->mdns.services = calloc(config->mdns.num_services, sizeof(mdns_service_t));

            for (size_t i = 0; i < config->mdns.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;

                json_t* name = json_object_get(service, "Name");
                const char* name_str = json_is_string(name) ? json_string_value(name) : "hydrogen";
                config->mdns.services[i].name = strdup(name_str);

                json_t* type = json_object_get(service, "Type");
                const char* type_str = json_is_string(type) ? json_string_value(type) : "_http._tcp.local";
                config->mdns.services[i].type = strdup(type_str);

                json_t* port = json_object_get(service, "Port");
                config->mdns.services[i].port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;
        
                // Handle TXT records
                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    // If TxtRecords is a single string, treat it as one record
                    config->mdns.services[i].num_txt_records = 1;
                    config->mdns.services[i].txt_records = malloc(sizeof(char*));
                    config->mdns.services[i].txt_records[0] = strdup(json_string_value(txt_records));
                } else if (json_is_array(txt_records)) {
                    // If TxtRecords is an array, handle multiple records
                    config->mdns.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns.services[i].txt_records = malloc(config->mdns.services[i].num_txt_records * sizeof(char*));
                    for (size_t j = 0; j < config->mdns.services[i].num_txt_records; j++) {
                        config->mdns.services[i].txt_records[j] = strdup(json_string_value(json_array_get(txt_records, j)));
                    }
                } else {
                    // If TxtRecords is not present or invalid, set to NULL
                    config->mdns.services[i].num_txt_records = 0;
                    config->mdns.services[i].txt_records = NULL;
                }
            }
        }
    }

    json_decref(root);
    return config;
}


const char* get_priority_label(int priority) {
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        if (DEFAULT_PRIORITY_LEVELS[i].value == priority) {
            return DEFAULT_PRIORITY_LEVELS[i].label;
        }
    }
    return "UNKNOWN";
}

void calculate_max_priority_label_width() {
    MAX_PRIORITY_LABEL_WIDTH = 0;
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        int label_width = strlen(DEFAULT_PRIORITY_LEVELS[i].label);
        if (label_width > MAX_PRIORITY_LABEL_WIDTH) {
            MAX_PRIORITY_LABEL_WIDTH = label_width;
        }
    }
}
