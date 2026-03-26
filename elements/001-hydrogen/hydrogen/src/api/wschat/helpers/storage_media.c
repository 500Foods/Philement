/*
 * Chat Storage Media Functions
 *
 * Provides media asset storage and retrieval functions.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <jansson.h>

#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

#include "storage_media.h"
#include "storage_hex.h"
#include "storage_compress.h"

static const char* SR_CHAT_STORAGE_MEDIA = "CHAT_STORAGE";

extern DatabaseQueueManager* global_queue_manager;

/* Query execution helper - declared as extern since it's in chat_storage.c */
extern bool chat_storage_execute_query(DatabaseQueue* db_queue, int query_ref,
                                        const char* params_json, char** result_json);

bool chat_storage_store_media(const char* database, const char* media_hash,
                              const unsigned char* media_data, size_t media_size,
                              const char* mime_type) {
    if (!database || !media_hash || !media_data || media_size == 0) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Invalid parameters for store_media", LOG_LEVEL_ERROR, 0);
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return false;
    }

    /* Convert binary data to hex string */
    char* media_data_hex = chat_storage_binary_to_hex(media_data, media_size);
    if (!media_data_hex) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Failed to allocate hex buffer for media data", LOG_LEVEL_ERROR, 0);
        return false;
    }

    char params_json[4096];
    snprintf(params_json, sizeof(params_json),
        "{"
        "\"MEDIA_HASH\": \"%s\","
        "\"MEDIA_DATA\": \"%s\","
        "\"MEDIA_SIZE\": %zu,"
        "\"MIME_TYPE\": \"%s\""
        "}",
        media_hash,
        media_data_hex,
        media_size,
        mime_type ? mime_type : ""
    );

    free(media_data_hex);

    log_this(SR_CHAT_STORAGE_MEDIA, "Storing media hash: %s (size: %zu)", LOG_LEVEL_DEBUG, 2, media_hash, media_size);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 71, params_json, &result_json);

    if (result_json) {
        free(result_json);
    }

    if (!query_success) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Failed to execute QueryRef #071 for media hash: %s", LOG_LEVEL_ERROR, 1, media_hash);
    }

    return query_success;
}

bool chat_storage_retrieve_media(const char* database, const char* media_hash,
                                 unsigned char** media_data, size_t* media_size,
                                 char** mime_type) {
    if (!database || !media_hash || !media_data || !media_size) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Invalid parameters for retrieve_media", LOG_LEVEL_ERROR, 0);
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return false;
    }

    char params_json[1024];
    snprintf(params_json, sizeof(params_json),
        "{\"MEDIA_HASH\": \"%s\"}",
        media_hash
    );

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 72, params_json, &result_json);

    if (!query_success || !result_json) {
        log_this(SR_CHAT_STORAGE_MEDIA, "Failed to execute QueryRef #072 for media hash: %s", LOG_LEVEL_ERROR, 1, media_hash);
        if (result_json) {
            free(result_json);
        }
        return false;
    }

    /* Parse result JSON */
    json_error_t error;
    json_t* root = json_loads(result_json, 0, &error);
    free(result_json);

    if (!root || !json_is_array(root) || json_array_size(root) == 0) {
        log_this(SR_CHAT_STORAGE_MEDIA, "No media asset found for hash: %s", LOG_LEVEL_DEBUG, 1, media_hash);
        if (root) {
            json_decref(root);
        }
        return false;
    }

    json_t* row = json_array_get(root, 0);
    if (!json_is_object(row)) {
        json_decref(root);
        return false;
    }

    /* Extract media_data hex string */
    json_t* media_data_hex_json = json_object_get(row, "media_data");
    if (!media_data_hex_json || !json_is_string(media_data_hex_json)) {
        json_decref(root);
        return false;
    }
    const char* media_data_hex = json_string_value(media_data_hex_json);

    /* Convert hex to binary */
    uint8_t* binary_data = NULL;
    size_t binary_len = 0;
    if (!chat_storage_hex_to_binary(media_data_hex, &binary_data, &binary_len)) {
        json_decref(root);
        return false;
    }

    /* Extract mime_type */
    json_t* mime_json = json_object_get(row, "mime_type");
    char* mime = NULL;
    if (mime_json && json_is_string(mime_json)) {
        mime = strdup(json_string_value(mime_json));
    }

    /* Extract media_size */
    json_t* size_json = json_object_get(row, "media_size");
    size_t size = 0;
    if (size_json && json_is_integer(size_json)) {
        size = (size_t)json_integer_value(size_json);
    }

    json_decref(root);

    *media_data = binary_data;
    *media_size = size;
    if (mime_type) {
        *mime_type = mime;
    } else {
        free(mime);
    }

    log_this(SR_CHAT_STORAGE_MEDIA, "Retrieved media hash: %s (size: %zu)", LOG_LEVEL_DEBUG, 2, media_hash, size);
    return true;
}

bool chat_storage_resolve_media_in_content(const char* database,
                                           const char* content_json,
                                           char** resolved_json,
                                           char** error_message) {
    if (!database || !content_json || !resolved_json) {
        if (error_message) {
            *error_message = strdup("Invalid parameters");
        }
        return false;
    }

    json_error_t json_error;
    json_t* content_array = json_loads(content_json, 0, &json_error);
    if (!content_array || !json_is_array(content_array)) {
        if (content_array) {
            json_decref(content_array);
        }
        if (error_message) {
            *error_message = strdup("Invalid JSON array in content");
        }
        return false;
    }

    json_t* new_array = json_array();
    if (!new_array) {
        json_decref(content_array);
        if (error_message) {
            *error_message = strdup("Failed to create JSON array");
        }
        return false;
    }

    size_t index;
    json_t* item;
    bool success = true;

    json_array_foreach(content_array, index, item) {
        if (!json_is_object(item)) {
            json_array_append(new_array, item);
            continue;
        }

        json_t* type = json_object_get(item, "type");
        if (!type || !json_is_string(type)) {
            json_array_append(new_array, item);
            continue;
        }

        const char* type_str = json_string_value(type);
        if (strcmp(type_str, "image_url") != 0) {
            /* Not an image_url, keep as is */
            json_array_append(new_array, item);
            continue;
        }

        json_t* image_url = json_object_get(item, "image_url");
        if (!image_url || !json_is_object(image_url)) {
            json_array_append(new_array, item);
            continue;
        }

        json_t* url = json_object_get(image_url, "url");
        if (!url || !json_is_string(url)) {
            json_array_append(new_array, item);
            continue;
        }

        const char* url_str = json_string_value(url);
        if (strncmp(url_str, "media:", 6) != 0) {
            /* Not a media reference, keep as is */
            json_array_append(new_array, item);
            continue;
        }

        /* Extract hash after "media:" */
        const char* media_hash = url_str + 6;

        /* Retrieve media from storage */
        unsigned char* media_data = NULL;
        size_t media_size = 0;
        char* mime_type = NULL;

        if (!chat_storage_retrieve_media(database, media_hash, &media_data, &media_size, &mime_type)) {
            /* Media not found - keep original reference */
            json_array_append(new_array, item);
            continue;
        }

        /* Encode directly to standard base64 */
        char* base64 = utils_base64_encode(media_data, media_size);
        free(media_data);
        if (!base64) {
            free(mime_type);
            success = false;
            break;
        }

        /* Build data URL */
        const char* mime = mime_type ? mime_type : "application/octet-stream";
        char* data_url = malloc(strlen(mime) + strlen(base64) + 32);
        if (!data_url) {
            free(base64);
            free(mime_type);
            success = false;
            break;
        }
        sprintf(data_url, "data:%s;base64,%s", mime, base64);
        free(base64);
        free(mime_type);

        /* Create new image_url object with data URL */
        json_t* new_item = json_object();
        json_object_set(new_item, "type", json_string("image_url"));
        json_t* new_image_url = json_object();
        json_object_set(new_image_url, "url", json_string(data_url));
        json_object_set(new_item, "image_url", new_image_url);
        json_array_append(new_array, new_item);

        free(data_url);
        json_decref(new_item);
    }

    json_decref(content_array);

    if (!success) {
        json_decref(new_array);
        if (error_message) {
            *error_message = strdup("Failed to resolve media reference");
        }
        return false;
    }

    /* Convert new array to string */
    *resolved_json = json_dumps(new_array, JSON_COMPACT);
    json_decref(new_array);

    if (!*resolved_json) {
        if (error_message) {
            *error_message = strdup("Failed to serialize resolved content");
        }
        return false;
    }

    return true;
}
