#include "configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

int MAX_PRIORITY_LABEL_WIDTH = 0;
const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "INFO"},
    {1, "WARN"},
    {2, "DEBUG"},
    {3, "ERROR"},
    {4, "CRITICAL"}
};

char* get_executable_path() {
    char* path = malloc(PATH_MAX);
    if (path == NULL) {
        return NULL;
    }
    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);
    if (len == -1) {
        free(path);
        return NULL;
    }
    path[len] = '\0';
    return path;
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

    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        fprintf(stderr, "Error: Unable to create default config at %s\n", config_path);
    } else {
        printf("Created default config at %s\n", config_path);
    }

    json_decref(root);
}

json_t* load_config(const char* config_path) {
    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        fprintf(stderr, "Error: Failed to load config file: %s\n", error.text);
        create_default_config(config_path);
        root = json_load_file(config_path, 0, &error);
        if (!root) {
            fprintf(stderr, "Error: Failed to load default config file: %s\n", error.text);
        }
    }


    calculate_max_priority_label_width();

    return root;
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
