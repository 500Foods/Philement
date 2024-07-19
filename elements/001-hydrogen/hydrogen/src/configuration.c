// System Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>

// Project Libraries
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
    json_object_set_new(root, "Web", web);

    // WebSocket Configuration
    json_t* websocket = json_object();
    json_object_set_new(websocket, "Port", json_integer(5001));
    json_object_set_new(websocket, "Key", json_string("default_key_change_me"));
    json_object_set_new(websocket, "Protocol", json_string("hydrogen-protocol"));
    json_object_set_new(root, "WebSocket", websocket);

    // mDNS Configuration
    json_t* mdns = json_object();
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string("0.1.0"));

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
    json_t* web = json_object_get(root, "Web");
    if (json_is_object(web)) {
        config->web.port = json_integer_value(json_object_get(web, "Port"));
        config->web.web_root = strdup(json_string_value(json_object_get(web, "WebRoot")));
        config->web.upload_path = strdup(json_string_value(json_object_get(web, "UploadPath")));
        config->web.upload_dir = strdup(json_string_value(json_object_get(web, "UploadDir")));
        config->web.max_upload_size = json_integer_value(json_object_get(web, "MaxUploadSize"));
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        config->websocket.port = json_integer_value(json_object_get(websocket, "Port"));
        config->websocket.key = strdup(json_string_value(json_object_get(websocket, "Key")));
        config->websocket.protocol = strdup(json_string_value(json_object_get(websocket, "Protocol")));

        json_t* max_message_mb = json_object_get(websocket, "MaxMessageMB");
        if (json_is_integer(max_message_mb)) {
            config->websocket.max_message_size = json_integer_value(max_message_mb) * 1024 * 1024;
        } else {
            config->websocket.max_message_size = 10 * 1024 * 1024;  // Default to 10 MB if not specified
        }

    }

    // mDNS Configuration
    json_t* mdns = json_object_get(root, "mDNS");
    if (json_is_object(mdns)) {
        config->mdns.device_id = strdup(json_string_value(json_object_get(mdns, "DeviceId")));
        config->mdns.friendly_name = strdup(json_string_value(json_object_get(mdns, "FriendlyName")));
        config->mdns.model = strdup(json_string_value(json_object_get(mdns, "Model")));
        config->mdns.manufacturer = strdup(json_string_value(json_object_get(mdns, "Manufacturer")));
        config->mdns.version = strdup(json_string_value(json_object_get(mdns, "Version")));

        json_t* services = json_object_get(mdns, "Services");
	if (json_is_array(services)) {
            config->mdns.num_services = json_array_size(services);
            config->mdns.services = calloc(config->mdns.num_services, sizeof(mdns_service_t));

            for (size_t i = 0; i < config->mdns.num_services; i++) {
                json_t* service = json_array_get(services, i);
                config->mdns.services[i].name = strdup(json_string_value(json_object_get(service, "Name")));
                config->mdns.services[i].type = strdup(json_string_value(json_object_get(service, "Type")));
                config->mdns.services[i].port = json_integer_value(json_object_get(service, "Port"));
        
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
