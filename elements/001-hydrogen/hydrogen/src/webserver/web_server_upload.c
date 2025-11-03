// Global includes
#include <src/hydrogen.h>

// Local includes
#include "web_server_upload.h"
#include "web_server_core.h"
#include <src/print/beryllium.h>
#include <src/utils/utils.h>

// Global configuration
extern AppConfig* app_config;

// Generate collision-resistant file identifiers
void generate_uuid(char *uuid_str) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long int time_in_usec = ((unsigned long long int)tv.tv_sec * 1000000ULL) + (unsigned long long int)tv.tv_usec;

    snprintf(uuid_str, UUID_STR_LEN, "%08llx-%04x-%04x-%04x-%08llx%04x",
             time_in_usec & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff),
             (unsigned int)((rand() & 0xfff) | 0x4000),
             (unsigned int)((rand() & 0x3fff) | 0x8000),
             ((unsigned long long int)(unsigned int)rand() * (unsigned int)rand()) & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff));
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

                char temp_path[DEFAULT_LINE_BUFFER_SIZE];
                snprintf(temp_path, sizeof(temp_path), "%s/%s.gcode", server_web_config->upload_dir, uuid_str);
                con_info->fp = fopen(temp_path, "wb");
                if (!con_info->fp) {
                    log_this(SR_WEBSERVER, "Failed to open file for writing", LOG_LEVEL_DEBUG, 0);
                    return MHD_NO;
                }
                free(con_info->original_filename);  // Free previous allocation if any
                free(con_info->new_filename);       // Free previous allocation if any
                con_info->original_filename = strdup(filename);
                con_info->new_filename = strdup(temp_path);

                log_this(SR_WEBSERVER, "Starting file upload", LOG_LEVEL_DEBUG, 0);
                log_this(SR_WEBSERVER, filename, LOG_LEVEL_DEBUG, 0);
            }
        }

        if (size > 0) {
            if (con_info->total_size + size > server_web_config->max_upload_size) {
                log_this(SR_WEBSERVER, "File upload exceeds maximum allowed size", LOG_LEVEL_ERROR, 0);

                // Close and delete the partially uploaded file
                if (con_info->fp) {
                    fclose(con_info->fp);
                    con_info->fp = NULL;
                    if (remove(con_info->new_filename) != 0) {
                        log_this(SR_WEBSERVER, "Failed to remove partial upload file", LOG_LEVEL_DEBUG, 0);
                    }
                    free(con_info->new_filename);
                    con_info->new_filename = NULL;
                }

                // Mark upload as failed
                con_info->upload_failed = true;
                con_info->error_code = MHD_HTTP_CONTENT_TOO_LARGE;
                return MHD_NO;
            }

            if (fwrite(data, 1, size, con_info->fp) != size) {
                log_this(SR_WEBSERVER, "Failed to write to file", LOG_LEVEL_DEBUG, 0);
                return MHD_NO;
            }
            con_info->total_size += size;

            // Log progress every 100MB
            if (con_info->total_size / (100 * 1024 * 1024) > con_info->last_logged_mb) {
                con_info->last_logged_mb = con_info->total_size / (100 * 1024 * 1024);
                log_this(SR_WEBSERVER, "Upload progress", LOG_LEVEL_ALERT, 0);
                char progress_str[32];
                snprintf(progress_str, sizeof(progress_str), "%zu MB", con_info->last_logged_mb * 100);
                log_this(SR_WEBSERVER, progress_str, LOG_LEVEL_ALERT, 0);
            }
        }
    } else if (0 == strcmp(key, "print")) {
        // Handle the 'print' field
        con_info->print_after_upload = (0 == strcmp(data, "true"));
        log_this(SR_WEBSERVER, con_info->print_after_upload ? "Print after upload: enabled" : "Print after upload: disabled", LOG_LEVEL_DEBUG, 0);
    } else {
        // Log unknown keys
        char log_buffer[DEFAULT_LOG_BUFFER_SIZE];  // Use configured log buffer size
        snprintf(log_buffer, sizeof(log_buffer), "Received unknown key in form data: %s", key);
        log_this(SR_WEBSERVER, log_buffer, LOG_LEVEL_ALERT, 0);
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
        // Check if upload failed due to size limit
        if (con_info->upload_failed) {
            log_this(SR_WEBSERVER, "File upload rejected due to size limit", LOG_LEVEL_ERROR, 0);

            const char *error_response = "{\"error\": \"File too large\", \"done\": false}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_response),
                                            (void*)error_response, MHD_RESPMEM_PERSISTENT);
            add_cors_headers(response);
            MHD_add_response_header(response, "Content-Type", "application/json");
            enum MHD_Result ret = MHD_queue_response(connection, con_info->error_code, response);
            MHD_destroy_response(response);
            con_info->response_sent = true;
            return ret;
        }

        if (con_info->fp) {
            fclose(con_info->fp);
            con_info->fp = NULL;

            // Process print job here
            json_t* print_job = json_object();

            if (con_info->original_filename) {
                json_object_set_new(print_job, "original_filename", json_string(con_info->original_filename));
            } else {
                json_object_set_new(print_job, "original_filename", json_string("unknown"));
            }
            json_object_set_new(print_job, "new_filename", json_string(con_info->new_filename));
            json_object_set_new(print_job, "file_size", json_integer((json_int_t)con_info->total_size));
            json_object_set_new(print_job, "print_after_upload", json_boolean(con_info->print_after_upload));

            // Run beryllium analysis for .gcode files
            if (con_info->original_filename && strstr(con_info->original_filename, ".gcode")) {
                log_this(SR_WEBSERVER, "Running beryllium analysis on uploaded G-code file", LOG_LEVEL_DEBUG, 0);
                json_t* gcode_info = extract_gcode_info(con_info->new_filename);
                if (gcode_info) {
                    json_object_set_new(print_job, "gcode_info", gcode_info);

                    // Extract BerylliumStats from JSON for proper formatting
                    BerylliumStats stats;
                    memset(&stats, 0, sizeof(BerylliumStats));
                    stats.success = true; // Assume success since we have gcode_info

                    json_t* file_size = json_object_get(gcode_info, "file_size");
                    if (file_size) stats.file_size = (int)json_integer_value(file_size);

                    json_t* total_lines = json_object_get(gcode_info, "total_lines");
                    if (total_lines) stats.total_lines = (int)json_integer_value(total_lines);

                    json_t* gcode_lines = json_object_get(gcode_info, "gcode_lines");
                    if (gcode_lines) stats.gcode_lines = (int)json_integer_value(gcode_lines);

                    json_t* layer_count_height = json_object_get(gcode_info, "layer_count_height");
                    if (layer_count_height) stats.layer_count_height = (int)json_integer_value(layer_count_height);

                    json_t* layer_count_slicer = json_object_get(gcode_info, "layer_count_slicer");
                    if (layer_count_slicer) stats.layer_count_slicer = (int)json_integer_value(layer_count_slicer);

                    // Extract filament and timing data
                    json_t* filament_used = json_object_get(gcode_info, "filament_used_mm");
                    if (filament_used) stats.extrusion = json_real_value(filament_used);

                    json_t* filament_volume = json_object_get(gcode_info, "filament_used_cm3");
                    if (filament_volume) stats.filament_volume = json_real_value(filament_volume);

                    json_t* filament_weight = json_object_get(gcode_info, "filament_weight_g");
                    if (filament_weight) stats.filament_weight = json_real_value(filament_weight);

                    json_t* objects = json_object_get(gcode_info, "objects");
                    if (objects) stats.num_objects = (int)json_array_size(objects);

                    // Calculate print time from the formatted time string
                    json_t* estimated_time = json_object_get(gcode_info, "estimated_print_time");
                    if (estimated_time) {
                        // Parse time string like "01:30:45" to seconds
                        const char* time_str = json_string_value(estimated_time);
                        int days = 0, hours = 0, minutes = 0, seconds = 0;
                        sscanf(time_str, "%d:%d:%d:%d", &days, &hours, &minutes, &seconds);
                        stats.print_time = days * 86400 + hours * 3600 + minutes * 60 + seconds;
                    }

                    // Calculate layer height from total height / layer count
                    // Note: We need to estimate this since it's not directly available in JSON
                    if (stats.layer_count_height > 0) {
                        // This is a simplified calculation - in practice you'd want to track Z values
                        stats.layer_height = 0.2; // Default layer height assumption
                    }

                    // Use the improved beryllium formatting logic with proper server logging
                    // Format time as Hh Mm Ss
                    int hours = (int)(stats.print_time / 3600);
                    int minutes = (int)((stats.print_time - hours * 3600) / 60);
                    int seconds = (int)(stats.print_time - hours * 3600 - minutes * 60);

                    // Format each line using the improved format_double_with_commas function
                    {
                        char formatted_file_size[32];
                        format_number_with_commas((size_t)stats.file_size, formatted_file_size, sizeof(formatted_file_size));
                        char file_info[80];
                        snprintf(file_info, sizeof(file_info), "- File: %s bytes", formatted_file_size);
                        log_this(SR_WEBSERVER, file_info, LOG_LEVEL_DEBUG, 0);
                    }

                    {
                        char formatted_gcode_lines[32];
                        format_number_with_commas((size_t)stats.gcode_lines, formatted_gcode_lines, sizeof(formatted_gcode_lines));
                        char lines_info[96];
                        snprintf(lines_info, sizeof(lines_info), "- Lines: %s", formatted_gcode_lines);
                        log_this(SR_WEBSERVER, lines_info, LOG_LEVEL_DEBUG, 0);
                    }

                    if (stats.layer_count_height > 0) {
                        double total_height = stats.layer_count_height * stats.layer_height;
                        char formatted_layer_count[32];
                        char layers_info[96];
                        if (total_height > 0.0) {
                            format_number_with_commas((size_t)stats.layer_count_height, formatted_layer_count, sizeof(formatted_layer_count));
                            snprintf(layers_info, sizeof(layers_info), "- Layers: %s layers, %.1f mm height",
                                    formatted_layer_count, total_height);
                        } else {
                            // Fallback if layer height calculation failed
                            format_number_with_commas((size_t)stats.layer_count_height, formatted_layer_count, sizeof(formatted_layer_count));
                            snprintf(layers_info, sizeof(layers_info), "- Layers: %s", formatted_layer_count);
                        }
                        log_this(SR_WEBSERVER, layers_info, LOG_LEVEL_DEBUG, 0);
                    }

                    if (hours > 0 || minutes > 0 || seconds > 0) {
                        char time_info[80];
                        if (hours > 0) {
                            snprintf(time_info, sizeof(time_info), "- Print Time: %dh %02dm %02ds", hours, minutes, seconds);
                        } else if (minutes > 0) {
                            snprintf(time_info, sizeof(time_info), "- Print Time: %dm %02ds", minutes, seconds);
                        } else {
                            snprintf(time_info, sizeof(time_info), "- Print Time: %ds", seconds);
                        }
                        log_this(SR_WEBSERVER, time_info, LOG_LEVEL_DEBUG, 0);
                    }

                    if (stats.extrusion > 0.0) {
                        char formatted_extrusion[32];
                        format_double_with_commas(stats.extrusion, 1, formatted_extrusion, sizeof(formatted_extrusion));
                        char filament_info[96];
                        snprintf(filament_info, sizeof(filament_info), "- Filament: %s mm", formatted_extrusion);
                        log_this(SR_WEBSERVER, filament_info, LOG_LEVEL_DEBUG, 0);
                    }

                    if (stats.filament_volume > 0.0 && stats.filament_weight > 0.0) {
                        char formatted_volume[32], formatted_weight[32];
                        format_double_with_commas(stats.filament_volume, 3, formatted_volume, sizeof(formatted_volume));
                        format_double_with_commas(stats.filament_weight, 1, formatted_weight, sizeof(formatted_weight));
                        char material_info[96];
                        snprintf(material_info, sizeof(material_info), "- Material: %s cm³ / %s g",
                                formatted_volume, formatted_weight);
                        log_this(SR_WEBSERVER, material_info, LOG_LEVEL_DEBUG, 0);
                    }

                    if (stats.num_objects > 0) {
                        char formatted_objects[32];
                        format_number_with_commas((size_t)stats.num_objects, formatted_objects, sizeof(formatted_objects));
                        char objects_info[80];
                        snprintf(objects_info, sizeof(objects_info), "- Objects: %s", formatted_objects);
                        log_this(SR_WEBSERVER, objects_info, LOG_LEVEL_DEBUG, 0);
                    }

                    log_this(SR_WEBSERVER, "Beryllium analysis completed successfully", LOG_LEVEL_DEBUG, 0);
                } else {
                    log_this(SR_WEBSERVER, "Beryllium analysis failed", LOG_LEVEL_ERROR, 0);
                }
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
                    log_this(SR_WEBSERVER, "Added print job to queue", LOG_LEVEL_DEBUG, 0);
                } else {
                    log_this(SR_WEBSERVER, "Failed to find PrintQueue", LOG_LEVEL_DEBUG, 0);
                }

                free(print_job_str);
            } else {
                log_this(SR_WEBSERVER, "Failed to create JSON string", LOG_LEVEL_DEBUG, 0);
            }

            json_decref(print_job);

            log_this(SR_WEBSERVER, "File upload completed:", LOG_LEVEL_DEBUG, 0);

            char info_str[DEFAULT_LINE_BUFFER_SIZE];
            if (con_info->original_filename) {
                snprintf(info_str, sizeof(info_str), "Source: %s", con_info->original_filename);
            } else {
                snprintf(info_str, sizeof(info_str), "Source: (no filename)");
            }
            log_this(SR_WEBSERVER, info_str, LOG_LEVEL_DEBUG, 0);
            snprintf(info_str, sizeof(info_str), "Local: %s", con_info->new_filename);
            log_this(SR_WEBSERVER, info_str, LOG_LEVEL_DEBUG, 0);
            snprintf(info_str, sizeof(info_str), "Size: %zu bytes", con_info->total_size);
            log_this(SR_WEBSERVER, info_str, LOG_LEVEL_DEBUG, 0);
            snprintf(info_str, sizeof(info_str), "Print: %s", con_info->print_after_upload ? "true" : "false");
            log_this(SR_WEBSERVER, info_str, LOG_LEVEL_DEBUG, 0);

            // Send response
            const char* filename_for_response = con_info->original_filename ? con_info->original_filename : "unknown";
            size_t filename_len = strlen(filename_for_response);
            char *json_response = malloc(filename_len + 100);  // Extra space for JSON structure
            sprintf(json_response, "{\"files\": {\"local\": {\"name\": \"%s\", \"origin\": \"local\"}}, \"done\": true}", filename_for_response);

            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_response),
                                            (void*)json_response, MHD_RESPMEM_MUST_FREE);
            add_cors_headers(response);
            MHD_add_response_header(response, "Content-Type", "application/json");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            con_info->response_sent = true;
            return ret;
        } else {
            log_this(SR_WEBSERVER, "File upload failed or no file was uploaded", LOG_LEVEL_ALERT, 0);
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
    if (!filename) {
        log_this(SR_WEBSERVER, "NULL filename passed to extract_gcode_info", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this(SR_WEBSERVER, "Failed to open G-code file for analysis", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Use configuration-based defaults
    BerylliumConfig config = beryllium_create_config();

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
    if (!filename) {
        log_this(SR_WEBSERVER, "NULL filename passed to extract_preview_image", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this(SR_WEBSERVER, "Failed to open G-code file for image extraction", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    char* image_data = NULL;
    size_t image_size = 0;
    bool in_thumbnail = false;
    char line_buffer[DEFAULT_LINE_BUFFER_SIZE];

    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        if (strstr(line_buffer, "; thumbnail begin")) {
            in_thumbnail = true;
            continue;
        }
        if (strstr(line_buffer, "; thumbnail end")) {
            break;
        }
        if (in_thumbnail && line_buffer[0] == ';') {
            size_t len = strlen(line_buffer);
            if (len > 2) {
                char *temp = realloc(image_data, image_size + len - 2);
                if (temp != NULL) {
                    image_data = temp;
                    memcpy(image_data + image_size, line_buffer + 2, len - 3);  // -3 to remove newline
                    image_size += len - 3;
                } else {
                    // Realloc failed, free original memory to avoid leak
                    free(image_data);
                    image_data = NULL;
                    log_this(SR_WEBSERVER, "Memory allocation failed during image extraction", LOG_LEVEL_DEBUG, 0);
                    break;
                }
            }
        }
    }

    fclose(file);

    if (image_data) {
        char *temp = realloc(image_data, image_size + 1);
        if (temp != NULL) {
            image_data = temp;
            image_data[image_size] = '\0';

            // Create a data URL
        } else {
            // Realloc failed, free original memory to avoid leak
            free(image_data);
            image_data = NULL;
            log_this(SR_WEBSERVER, "Memory allocation failed during image finalization", LOG_LEVEL_DEBUG, 0);
            return NULL;
        }
        char *data_url = malloc(image_size * 4 / 3 + 50); // Allocate enough space for the prefix and the base64 data
        sprintf(data_url, "data:image/png;base64,%s", image_data);
        free(image_data);
        return data_url;
    }

    return image_data;
}
