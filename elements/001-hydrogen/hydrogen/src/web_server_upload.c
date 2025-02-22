// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "web_server_upload.h"
#include "web_server_core.h"
#include "beryllium.h"
#include "queue.h"
#include "logging.h"
#include "utils_time.h"
#include "utils_logging.h"
#include "configuration.h"

// Global configuration
extern AppConfig* app_config;

// System headers
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Generate collision-resistant file identifiers
void generate_uuid(char *uuid_str) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long int time_in_usec = ((unsigned long long int)tv.tv_sec * 1000000ULL) + tv.tv_usec;

    snprintf(uuid_str, UUID_STR_LEN, "%08llx-%04x-%04x-%04x-%012llx",
             time_in_usec & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff),
             (unsigned int)((rand() & 0xfff) | 0x4000),
             (unsigned int)((rand() & 0x3fff) | 0x8000),
             (unsigned long long int)rand() * rand());
}

enum MHD_Result handle_upload_data(void *coninfo_cls, enum MHD_ValueKind kind,
                                 const char *key, const char *filename,
                                 const char *content_type, const char *transfer_encoding,
                                 const char *data, uint64_t off, size_t size) {
    struct ConnectionInfo *con_info = coninfo_cls;
    (void)kind; (void)content_type; (void)transfer_encoding; (void)off;  // Unused parameters

    if (0 == strcmp(key, "file")) {
        if (!con_info->fp) {
            if (filename) {
                char uuid_str[37];
                generate_uuid(uuid_str);

                char file_path[DEFAULT_LINE_BUFFER_SIZE];  // Use configured line buffer size
                snprintf(file_path, sizeof(file_path), "%s/%s.gcode", server_web_config->upload_dir, uuid_str);
                con_info->fp = fopen(file_path, "wb");
                if (!con_info->fp) {
                    log_this("WebServer", "Failed to open file for writing", 3, true, false, true);
                    return MHD_NO;
                }
                free(con_info->original_filename);  // Free previous allocation if any
                free(con_info->new_filename);       // Free previous allocation if any
                con_info->original_filename = strdup(filename);
                con_info->new_filename = strdup(file_path);

                char log_buffer[DEFAULT_LOG_BUFFER_SIZE];  // Use configured log buffer size
                snprintf(log_buffer, sizeof(log_buffer), "Starting file upload: %s", filename);
                log_this("WebServer", log_buffer, 0, true, false, true);
            }
        }

        if (size > 0) {
            if (con_info->total_size + size > server_web_config->max_upload_size) {
                log_this("WebServer", "File upload exceeds maximum allowed size", 3, true, false, true);
                return MHD_NO;
            }

            if (fwrite(data, 1, size, con_info->fp) != size) {
                log_this("WebServer", "Failed to write to file", 3, true, false, true);
                return MHD_NO;
            }
            con_info->total_size += size;

            // Log progress every 100MB
            if (con_info->total_size / (100 * 1024 * 1024) > con_info->last_logged_mb) {
                con_info->last_logged_mb = con_info->total_size / (100 * 1024 * 1024);
                char progress_log[DEFAULT_LOG_BUFFER_SIZE];  // Use configured log buffer size
                snprintf(progress_log, sizeof(progress_log), "Upload progress: %zu MB", con_info->last_logged_mb * 100);
                log_this("WebServer", progress_log, 2, true, false, true);
            }
        }
    } else if (0 == strcmp(key, "print")) {
        // Handle the 'print' field
        con_info->print_after_upload = (0 == strcmp(data, "true"));
        log_this("WebServer", con_info->print_after_upload ? "Print after upload: enabled" : "Print after upload: disabled", 
                0, true, false, true);
    } else {
        // Log unknown keys
        char log_buffer[DEFAULT_LOG_BUFFER_SIZE];  // Use configured log buffer size
        snprintf(log_buffer, sizeof(log_buffer), "Received unknown key in form data: %s", key);
        log_this("WebServer", log_buffer, 2, true, false, true);
    }

    return MHD_YES;
}

enum MHD_Result handle_upload_request(struct MHD_Connection *connection,
                                    const char *upload_data,
                                    size_t *upload_data_size,
                                    void **con_cls) {
    struct ConnectionInfo *con_info = *con_cls;
    
    if (NULL == con_info) {
        con_info = calloc(1, sizeof(struct ConnectionInfo));
        if (NULL == con_info) return MHD_NO;
        con_info->postprocessor = MHD_create_post_processor(connection, 
            app_config ? app_config->resources.post_processor_buffer_size : DEFAULT_POST_PROCESSOR_BUFFER_SIZE,
            handle_upload_data, con_info);
        if (NULL == con_info->postprocessor) {
            free(con_info);
            return MHD_NO;
        }
        *con_cls = (void*)con_info;
        return MHD_YES;
    }

    if (*upload_data_size != 0) {
        MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    } else if (!con_info->response_sent) {
        if (con_info->fp) {
            fclose(con_info->fp);
            con_info->fp = NULL;

            // Process print job here
            json_t* print_job = json_object();

            json_object_set_new(print_job, "original_filename", json_string(con_info->original_filename));
            json_object_set_new(print_job, "new_filename", json_string(con_info->new_filename));
            json_object_set_new(print_job, "file_size", json_integer(con_info->total_size));
            json_object_set_new(print_job, "print_after_upload", json_boolean(con_info->print_after_upload));

            json_t* gcode_info = extract_gcode_info(con_info->new_filename);
            if (gcode_info) {
                json_object_set_new(print_job, "gcode_info", gcode_info);
            }

            char* preview_image = extract_preview_image(con_info->new_filename);
            if (preview_image) {
                json_object_set_new(print_job, "preview_image", json_string(preview_image));
                free(preview_image);
            }

            char* print_job_str = json_dumps(print_job, JSON_COMPACT);
            if (print_job_str) {
                Queue* print_queue = queue_find("PrintQueue");
                if (print_queue) {
                    queue_enqueue(print_queue, print_job_str, strlen(print_job_str), 0);
                    log_this("WebServer", "Added print job to queue", 0, true, false, true);
                } else {
                    log_this("WebServer", "Failed to find PrintQueue", 3, true, false, true);
                }

                free(print_job_str);
            } else {
                log_this("WebServer", "Failed to create JSON string", 3, true, false, true);
            }

            json_decref(print_job);

            char complete_log[DEFAULT_LOG_BUFFER_SIZE];  // Use configured log buffer size
            log_this("WebServer", "File upload completed:", 0, true, true, true);
            snprintf(complete_log, sizeof(complete_log), " -> Source: %s", con_info->original_filename);
            log_this("WebServer", complete_log, 0, true, true, true);
            snprintf(complete_log, sizeof(complete_log), " ->  Local: %s", con_info->new_filename);
            log_this("WebServer", complete_log, 0, true, true, true);
            snprintf(complete_log, sizeof(complete_log), " ->   Size: %zu bytes", con_info->total_size);
            log_this("WebServer", complete_log, 0, true, true, true);
            snprintf(complete_log, sizeof(complete_log), " ->  Print: %s", con_info->print_after_upload ? "true" : "false");
            log_this("WebServer", complete_log, 0, true, true, true);

            // Send response
            const char *response_text = "{\"files\": {\"local\": {\"name\": \"%s\", \"origin\": \"local\"}}, \"done\": true}";
            char *json_response = malloc(strlen(response_text) + strlen(con_info->original_filename) + 1);
            sprintf(json_response, response_text, con_info->original_filename);

            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_response),
                                            (void*)json_response, MHD_RESPMEM_MUST_FREE);
            add_cors_headers(response);
            MHD_add_response_header(response, "Content-Type", "application/json");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            con_info->response_sent = true;
            return ret;
        } else {
            log_this("WebServer", "File upload failed or no file was uploaded", 2, true, false, true);
            const char *error_response = "{\"error\": \"File upload failed\", \"done\": false}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_response),
                                            (void*)error_response, MHD_RESPMEM_PERSISTENT);
            add_cors_headers(response);
            MHD_add_response_header(response, "Content-Type", "application/json");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            con_info->response_sent = true;
            return ret;
        }
    }

    return MHD_YES;
}

json_t* extract_gcode_info(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this("WebServer", "Failed to open G-code file for analysis", 3, true, false, true);
        return NULL;
    }

    // Use configuration-based defaults
    BerylliumConfig config = beryllium_create_config(app_config);

    char *start_time = get_iso8601_timestamp();
    clock_t start = clock();
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);
    clock_t end = clock();
    char *end_time = get_iso8601_timestamp();
    fclose(file);

    double elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    json_t* info = json_object();

    json_object_set_new(info, "analysis_start", json_string(start_time));
    json_object_set_new(info, "analysis_end", json_string(end_time));
    json_object_set_new(info, "analysis_duration_ms", json_real(elapsed_time));

    json_object_set_new(info, "file_size", json_integer(stats.file_size));
    json_object_set_new(info, "total_lines", json_integer(stats.total_lines));
    json_object_set_new(info, "gcode_lines", json_integer(stats.gcode_lines));
    json_object_set_new(info, "layer_count_height", json_integer(stats.layer_count_height));
    json_object_set_new(info, "layer_count_slicer", json_integer(stats.layer_count_slicer));

    json_t* objects = json_array();
    for (int i = 0; i < stats.num_objects; i++) {
        json_t* object = json_object();
        json_object_set_new(object, "index", json_integer(stats.object_infos[i].index + 1));
        json_object_set_new(object, "name", json_string(stats.object_infos[i].name));
        json_array_append_new(objects, object);
    }
    json_object_set_new(info, "objects", objects);

    json_object_set_new(info, "filament_used_mm", json_real(stats.extrusion));
    json_object_set_new(info, "filament_used_cm3", json_real(stats.filament_volume));
    json_object_set_new(info, "filament_weight_g", json_real(stats.filament_weight));

    char print_time_str[20];
    format_time(stats.print_time, print_time_str);
    json_object_set_new(info, "estimated_print_time", json_string(print_time_str));

    json_t* layers = json_array();
    double cumulative_time = 0.0;
    for (int i = 0; i < stats.layer_count_slicer; i++) {
        json_t* layer = json_object();
        char start_str[20], end_str[20], duration_str[20];
        format_time(cumulative_time, start_str);
        cumulative_time += stats.layer_times[i];
        format_time(cumulative_time, end_str);
        format_time(stats.layer_times[i], duration_str);

        json_object_set_new(layer, "layer", json_integer(i + 1));
        json_object_set_new(layer, "start_time", json_string(start_str));
        json_object_set_new(layer, "end_time", json_string(end_str));
        json_object_set_new(layer, "duration", json_string(duration_str));

        json_t* layer_objects = json_array();
        for (int j = 0; j < stats.num_objects; j++) {
            if (stats.object_times[i][j] > 0) {
                json_t* obj = json_object();
                char obj_start_str[20], obj_end_str[20], obj_duration_str[20];
                format_time(cumulative_time - stats.layer_times[i], obj_start_str);
                format_time(cumulative_time - stats.layer_times[i] + stats.object_times[i][j], obj_end_str);
                format_time(stats.object_times[i][j], obj_duration_str);

                json_object_set_new(obj, "object", json_integer(j + 1));
                json_object_set_new(obj, "start_time", json_string(obj_start_str));
                json_object_set_new(obj, "end_time", json_string(obj_end_str));
                json_object_set_new(obj, "duration", json_string(obj_duration_str));

                json_array_append_new(layer_objects, obj);
            }
        }
        json_object_set_new(layer, "objects", layer_objects);
        json_array_append_new(layers, layer);
    }
    json_object_set_new(info, "layers", layers);

    json_t* configuration = json_object();
    json_object_set_new(configuration, "acceleration", json_real(config.acceleration));
    json_object_set_new(configuration, "z_acceleration", json_real(config.z_acceleration));
    json_object_set_new(configuration, "extruder_acceleration", json_real(config.extruder_acceleration));
    json_object_set_new(configuration, "max_speed_xy", json_real(config.max_speed_xy));
    json_object_set_new(configuration, "max_speed_travel", json_real(config.max_speed_travel));
    json_object_set_new(configuration, "max_speed_z", json_real(config.max_speed_z));
    json_object_set_new(configuration, "default_feedrate", json_real(config.default_feedrate));
    json_object_set_new(configuration, "filament_diameter", json_real(config.filament_diameter));
    json_object_set_new(configuration, "filament_density", json_real(config.filament_density));
    json_object_set_new(info, "configuration", configuration);

    // Clean up the stats structure
    beryllium_free_stats(&stats);

    return info;
}

char* extract_preview_image(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this("WebServer", "Failed to open G-code file for image extraction", 3, true, false, true);
        return NULL;
    }

    char line[DEFAULT_LINE_BUFFER_SIZE];  // Use configured line buffer size
    char* image_data = NULL;
    size_t image_size = 0;
    bool in_thumbnail = false;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "; thumbnail begin")) {
            in_thumbnail = true;
            continue;
        }
        if (strstr(line, "; thumbnail end")) {
            break;
        }
        if (in_thumbnail && line[0] == ';') {
            size_t len = strlen(line);
            if (len > 2) {
                image_data = realloc(image_data, image_size + len - 2);
                memcpy(image_data + image_size, line + 2, len - 3);  // -3 to remove newline
                image_size += len - 3;
            }
        }
    }

    fclose(file);

    if (image_data) {
        image_data = realloc(image_data, image_size + 1);
        image_data[image_size] = '\0';

        // Create a data URL
        char *data_url = malloc(image_size * 4 / 3 + 50); // Allocate enough space for the prefix and the base64 data
        sprintf(data_url, "data:image/png;base64,%s", image_data);
        free(image_data);
        return data_url;
    }

    return image_data;
}