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
    json_object_set_new(root, "ServerName", json_string(DEFAULT_SERVER_NAME));
    json_object_set_new(root, "WebPort", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(root, "WebsocketPort", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(root, "UploadPath", json_string(DEFAULT_UPLOAD_PATH));
    json_object_set_new(root, "UploadDir", json_string(DEFAULT_UPLOAD_DIR));
    json_object_set_new(root, "MaxUploadSize", json_integer(DEFAULT_MAX_UPLOAD_SIZE));
    json_object_set_new(root, "LogFile", json_string(DEFAULT_LOG_FILE));

    json_t* mdns = json_object();
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string(VERSION));

    json_t* services = json_array();

    json_t* web_service = json_object();
    json_object_set_new(web_service, "Name", json_string("Hydrogen Web Interface"));
    json_object_set_new(web_service, "Type", json_string("_http._tcp"));
    json_object_set_new(web_service, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(web_service, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, web_service);

    json_t* octoprint_service = json_object();
    json_object_set_new(octoprint_service, "Name", json_string("Hydrogen OctoPrint Emulation"));
    json_object_set_new(octoprint_service, "Type", json_string("_octoprint._tcp"));
    json_object_set_new(octoprint_service, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(octoprint_service, "TxtRecords", json_string("path=/api,version=1.1.0"));
    json_array_append_new(services, octoprint_service);

    json_t* websocket_service = json_object();
    json_object_set_new(websocket_service, "Name", json_string("Hydrogen WebSocket"));
    json_object_set_new(websocket_service, "Type", json_string("_websocket._tcp"));
    json_object_set_new(websocket_service, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(websocket_service, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, websocket_service);

    json_object_set_new(mdns, "Services", services);
    json_object_set_new(root, "mDNS", mdns);

    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        fprintf(stderr, "Error: Unable to create default config at %s\n", config_path);
    } else {
        printf("Created default config at %s\n", config_path);
    }

    json_decref(root);
}

AppConfig* load_config(const char* config_path) {
    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        fprintf(stderr, "Error: Failed to load config file: %s\n", error.text);
        create_default_config(config_path);
        root = json_load_file(config_path, 0, &error);
        if (!root) {
            fprintf(stderr, "Error: Failed to load default config file: %s\n", error.text);
            return NULL;
        }
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        fprintf(stderr, "Error: Failed to allocate memory for config\n");
        json_decref(root);
        return NULL;
    }

    config->server_name = strdup(json_string_value(json_object_get(root, "ServerName")));
    config->web_port = json_integer_value(json_object_get(root, "WebPort"));
    config->websocket_port = json_integer_value(json_object_get(root, "WebsocketPort"));
    config->upload_path = strdup(json_string_value(json_object_get(root, "UploadPath")));
    config->upload_dir = strdup(json_string_value(json_object_get(root, "UploadDir")));
    config->max_upload_size = json_integer_value(json_object_get(root, "MaxUploadSize"));
    config->log_file_path = strdup(json_string_value(json_object_get(root, "LogFile")));

    config->executable_path = get_executable_path();

    json_t* mdns = json_object_get(root, "mDNS");
    if (mdns) {
        config->mdns.device_id = strdup(json_string_value(json_object_get(mdns, "DeviceId")));
        config->mdns.friendly_name = strdup(json_string_value(json_object_get(mdns, "FriendlyName")));
        config->mdns.model = strdup(json_string_value(json_object_get(mdns, "Model")));
        config->mdns.manufacturer = strdup(json_string_value(json_object_get(mdns, "Manufacturer")));
        config->mdns.version = strdup(json_string_value(json_object_get(mdns, "Version")));

        json_t* services = json_object_get(mdns, "Services");
        if (services && json_is_array(services)) {
            config->mdns.num_services = json_array_size(services);
            config->mdns.services = calloc(config->mdns.num_services, sizeof(mdns_service_t));

            for (int i = 0; i < config->mdns.num_services; i++) {
                json_t* service = json_array_get(services, i);
                config->mdns.services[i].name = strdup(json_string_value(json_object_get(service, "Name")));
                config->mdns.services[i].type = strdup(json_string_value(json_object_get(service, "Type")));
                config->mdns.services[i].port = json_integer_value(json_object_get(service, "Port"));

                // Parse TXT records
                const char* txt_records_str = json_string_value(json_object_get(service, "TxtRecords"));
                char* txt_records_copy = strdup(txt_records_str);
                char* saveptr;
                char* token = strtok_r(txt_records_copy, ",", &saveptr);
                int num_txt_records = 0;
                char** txt_records = NULL;

                while (token != NULL) {
                    // Trim leading and trailing whitespace
                    while (*token == ' ') token++;
                    char* end = token + strlen(token) - 1;
                    while (end > token && *end == ' ') end--;
                    *(end + 1) = '\0';

                    txt_records = realloc(txt_records, (num_txt_records + 1) * sizeof(char*));
                    txt_records[num_txt_records] = strdup(token);
                    num_txt_records++;
                    token = strtok_r(NULL, ",", &saveptr);
                }

                config->mdns.services[i].txt_records = txt_records;
                config->mdns.services[i].num_txt_records = num_txt_records;
                free(txt_records_copy);
            }
        }
    }

    json_decref(root);

    // Calculate max priority label width
    calculate_max_priority_label_width();

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
