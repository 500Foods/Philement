#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "utils.h"
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

nitro_config_t *nitro_config_load(const char *app_name) {
    nitro_config_t *config = malloc(sizeof(nitro_config_t));
    if (!config) {
        nitro_log("ERROR", "Out of memory");
        return NULL;
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "%s.json", app_name);

    json_t *root;
    json_error_t error;
    root = json_load_file(filename, 0, &error);

    if (!root) {
        nitro_log("INFO", "No config file found, using defaults");
        char id_buf[NITRO_ID_LEN + 1];
        nitro_generate_id(id_buf, sizeof(id_buf));

        config->id = malloc(strlen(app_name) + NITRO_ID_LEN + 2);
        if (!config->id) {
            nitro_log("ERROR", "Out of memory");
            free(config);
            return NULL;
        }
        sprintf(config->id, "%s-%s", app_name, id_buf);

        config->name = strdup(app_name);
        config->port = NITRO_DEFAULT_PORT;
    } else {
        config->id = strdup(json_string_value(json_object_get(root, "id")));
        config->name = strdup(json_string_value(json_object_get(root, "name")));
        config->port = json_integer_value(json_object_get(root, "port"));

        if (!config->id) {
            char id_buf[NITRO_ID_LEN + 1];
            nitro_generate_id(id_buf, sizeof(id_buf));

            config->id = malloc(strlen(app_name) + NITRO_ID_LEN + 2);
            if (!config->id) {
                nitro_log("ERROR", "Out of memory");
                json_decref(root);
                nitro_config_free(config);
                return NULL;
            }
            sprintf(config->id, "%s-%s", app_name, id_buf);
        }

        if (!config->name) {
            config->name = strdup(app_name);
        }

        if (config->port == 0) {
            config->port = NITRO_DEFAULT_PORT;
        }

        json_decref(root);
    }

    return config;
}

   void nitro_config_save(const char *app_name, nitro_config_t *config) {
       json_t *root = json_object();
       json_object_set_new(root, "id", json_string(config->id));
       json_object_set_new(root, "name", json_string(config->name));
       json_object_set_new(root, "port", json_integer(config->port));

       char filename[256];
       snprintf(filename, sizeof(filename), "%s.json", app_name);
       json_dump_file(root, filename, JSON_INDENT(2));
       json_decref(root);
   }

   void nitro_config_free(nitro_config_t *config) {
       if (config) {
           free(config->id);
           free(config->name);
           free(config);
       }
   }
