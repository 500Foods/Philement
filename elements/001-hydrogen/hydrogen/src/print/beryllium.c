/*
 * G-code Analysis
 */
// Global includes
#include <src/hydrogen.h>

// Local includes
#include "beryllium.h"

// Function prototypes for non-static functions
double calculate_layer_height(const double *z_values, int z_values_count);
double parse_parameter(const char *line, const char *parameter);
int parse_current_layer(const char *line);
bool parse_object_commands(const char *line, ObjectInfo **object_infos, int *num_objects,
                           int *current_object);
double process_movement_command(const char *line, BerylliumConfig *config,
                                double *current_x, double *current_y, double *current_z,
                                double *extrusion, double *current_extrusion_pos,
                                bool *relative_mode, bool *relative_extrusion,
                                double *current_feedrate, double *z_values, int *z_values_count,
                                int *z_values_capacity, int current_layer, int current_object,
                                int num_objects, double ***object_times);
double accelerated_move(double length, double acceleration, double max_velocity);

/**
 * Create a BerylliumConfig from AppConfig
 * @param app_config The application configuration
 * @return BerylliumConfig initialized with values from app_config
 */
BerylliumConfig beryllium_create_config(void) {
    BerylliumConfig config = {
        .acceleration = app_config ? app_config->print.motion.acceleration : 500.0,      // Default acceleration
        .z_acceleration = app_config ? app_config->print.motion.z_acceleration : 100.0,  // Default Z acceleration
        .extruder_acceleration = app_config ? app_config->print.motion.e_acceleration : 250.0, // Default extruder acceleration
        .max_speed_xy = app_config ? app_config->print.motion.max_speed_xy : 100.0,     // Default XY speed
        .max_speed_travel = app_config ? app_config->print.motion.max_speed_travel : 150.0, // Default travel speed
        .max_speed_z = app_config ? app_config->print.motion.max_speed_z : 20.0,        // Default Z speed
        .default_feedrate = 3000.0,  // Not configurable - G-code standard
        .filament_diameter = 1.75,   // Could be made configurable in future
        .filament_density = 1.24     // Could be made configurable in future
    };
    return config;
}

// Generate ISO8601 timestamps for print metadata
// Uses static buffer for efficiency
char* get_iso8601_timestamp(void) {
    static char buffer[sizeof "2011-10-08T07:07:09Z"];
    time_t now = time(NULL);
    strftime(buffer, sizeof buffer, "%FT%TZ", gmtime(&now));
    return buffer;
}

void format_time(double seconds, char *buffer) {
    int days = (int)(seconds / 86400);
    seconds -= days * 86400;
    int hours = (int)(seconds / 3600);
    seconds -= hours * 3600;
    int minutes = (int)(seconds / 60);
    seconds -= minutes * 60;
    sprintf(buffer, "%02d:%02d:%02d:%02.0f", days, hours, minutes, seconds);
}

// Format numbers with thousands separators for better readability
// Returns a static buffer for efficiency - not thread safe
char* format_number_with_separators(double value, int decimals) {
    static char buffer[64];
    char temp[64];

    // Format the number with specified decimal places
    if (decimals >= 0) sprintf(temp, "%.*f", decimals, value);
    else sprintf(temp, "%.0f", value);

    char *result = buffer;
    const char *input = temp;
    size_t len = strlen(temp);
    int comma_count = 0;

    // Handle negative numbers
    if (*input == '-') { *result++ = *input++; len--; }

    // Find decimal point position
    const char *decimal_point = strchr(input, '.');
    size_t integer_part_len = decimal_point ? (size_t)(decimal_point - input) : len;

    // Add commas to integer part
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '.') {
            // Copy decimal part as-is
            strcpy(result, input + i);
            result += strlen(input + i);
            break;
        }

        *result++ = input[i];

        // Add comma every 3 digits from the right (before decimal)
        if (++comma_count % 3 == 0 && i < integer_part_len - 1) {
            *result++ = ',';
        }
    }

    *result = '\0';
    return buffer;
}

// Calculate layer height from Z values array
// Returns layer height in mm, or 0.0 if calculation fails
double calculate_layer_height(const double *z_values, int z_values_count) {
    if (z_values == NULL || z_values_count < 2) return 0.0;  // Need at least 2 Z values to calculate height

    // Create a sorted copy of Z values
    double *sorted_z = malloc((size_t)z_values_count * sizeof(double));
    if (sorted_z == NULL) return 0.0;

    memcpy(sorted_z, z_values, (size_t)z_values_count * sizeof(double));

    // Simple bubble sort for Z values
    for (int i = 0; i < z_values_count - 1; i++) {
        for (int j = 0; j < z_values_count - i - 1; j++) {
            if (sorted_z[j] > sorted_z[j + 1]) {
                double temp = sorted_z[j];
                sorted_z[j] = sorted_z[j + 1];
                sorted_z[j + 1] = temp;
            }
        }
    }

    // Calculate differences and find most common layer height
    double *differences = malloc((size_t)(z_values_count - 1) * sizeof(double));
    if (differences == NULL) { free(sorted_z); return 0.0; }

    int diff_count = 0;
    for (int i = 1; i < z_values_count; i++) {
        double diff = sorted_z[i] - sorted_z[i - 1];
        if (diff > 0.001) {  // Only consider differences > 0.001mm
            differences[diff_count++] = diff;
        }
    }

    double layer_height = 0.0;
    if (diff_count > 0) {
        // Use the most common difference (simple mode calculation)
        // For simplicity, we'll use the median of the first few differences
        if (diff_count == 1) {
            layer_height = differences[0];
        } else {
            // Sort differences and take median
            for (int i = 0; i < diff_count - 1; i++) {
                for (int j = 0; j < diff_count - i - 1; j++) {
                    if (differences[j] > differences[j + 1]) {
                        double temp = differences[j];
                        differences[j] = differences[j + 1];
                        differences[j + 1] = temp;
                    }
                }
            }
            layer_height = differences[diff_count / 2];
        }
    }

    free(sorted_z);
    free(differences);
    return layer_height;
}

// Extract numeric parameters from G-code commands
// Returns NaN for missing parameters, prioritizes performance
double parse_parameter(const char *line, const char *parameter) {
    if (line == NULL || parameter == NULL) return NAN;

    const char *current = line;
    size_t param_len = strlen(parameter);

    while (*current != '\0') {
        // Check if we found the parameter
        if (strncmp(current, parameter, param_len) == 0) {
            // Check if this is the start of the parameter (not part of another word)
            if (current == line || *(current - 1) == ' ' || *(current - 1) == '\t' || *(current - 1) == '\n' || *(current - 1) == '\r') {
                // Move past the parameter name
                const char *value_start = current + param_len;

                // Skip any whitespace after the parameter name
                while (*value_start == ' ' || *value_start == '\t') {
                    value_start++;
                }

                // Parse the numeric value
                char *end_ptr;
                double value = strtod(value_start, &end_ptr);

                // Check if we actually parsed a number
                if (end_ptr != value_start) {
                    return value;
                }
            }
        }
        current++;
    }

    return NAN;  // Return NaN for missing parameters
}

char* parse_parameter_string(const char *line, const char *parameter) {
    if (line == NULL || parameter == NULL) return strdup("undefined");

    // Handle empty parameter string
    if (strlen(parameter) == 0) {
        return strdup("undefined");
    }

    const char *current = line;
    size_t param_len = strlen(parameter);

    // Search for the parameter in the line
    while (*current != '\0') {
        // Check if we found the parameter
        if (strncmp(current, parameter, param_len) == 0) {
            // Check if this is the start of the parameter (not part of another word)
            // Look at the character before the parameter
            if (current == line || *(current - 1) == ' ' || *(current - 1) == '\t' || *(current - 1) == '\n' || *(current - 1) == '\r') {
                // Move past the parameter name
                const char *value_start = current + param_len;

                // Skip any whitespace after the parameter name
                while (*value_start == ' ' || *value_start == '\t') {
                    value_start++;
                }

                // Check if there's an equals sign
                if (*value_start == '=') {
                    value_start++; // Move past '='
                    // Don't skip whitespace after '=' for NAME=value style parameters
                    // The whitespace should be preserved as part of the value
                }

                // Determine the extraction mode based on the parameter type and following content
                const char *end = value_start;

                // For message-style commands (M117, SET_PRINT_STATS_INFO, etc.), extract everything to end of line
                if (strcmp(parameter, "M117") == 0 ||
                    strcmp(parameter, "SET_PRINT_STATS_INFO") == 0 ||
                    strncmp(parameter, "EXCLUDE_OBJECT", 14) == 0) {
                    // Extract everything to end of line, preserving whitespace in the message
                    while (*end != '\0' && *end != '\n' && *end != '\r') {
                        end++;
                    }
                    // Trim trailing whitespace only at the very end of the line
                    while (end > value_start && (*(end - 1) == ' ' || *(end - 1) == '\t')) {
                        end--;
                    }
                } else {
                    // Check if we had an equals sign (NAME=value style parameter)
                    bool had_equals = (value_start > current && *(value_start - 1) == '=');

                    if (had_equals) {
                        // This is a NAME=value style parameter - preserve all whitespace in the value
                        while (*end != '\0' && *end != '\n' && *end != '\r') {
                            end++;
                        }
                        // Don't trim trailing whitespace for NAME=value style parameters
                    } else if (*value_start != '\0' && *value_start != '\n' && *value_start != '\r') {
                        // For other parameters (like X, Y, Z), extract the value until space or end of line
                        while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\n' && *end != '\r') {
                            end++;
                        }
                    }
                }

                // Calculate the length and allocate memory
                size_t length = (size_t)(end - value_start);
                char *value = (char *)malloc(length + 1);
                if (value != NULL) {
                    memcpy(value, value_start, length);
                    value[length] = '\0';
                    return value;
                }
                return NULL;
            }
        }
        current++;
    }
    return strdup("undefined");
}

char* parse_name_parameter(const char *line) {
    if (line == NULL) return strdup(""); // Return an empty string if line is NULL

    // Look for "NAME" followed by optional whitespace and then "="
    const char *current = line;
    while (*current != '\0') {
        if (strncmp(current, "NAME", 4) == 0) {
            current += 4; // Skip "NAME"

            // Skip any whitespace between "NAME" and "="
            while (*current == ' ' || *current == '\t') {
                current++;
            }

            // Check if we have an "=" after the whitespace
            if (*current == '=') {
                current++; // Skip the "="

                // Skip leading whitespace after "="
                while (*current == ' ' || *current == '\t') {
                    current++;
                }

                // Find the end of the value (first space, tab, newline, or end of string)
                const char *value_end = current;
                while (*value_end != '\0' && *value_end != ' ' && *value_end != '\t' && *value_end != '\n' && *value_end != '\r') {
                    value_end++;
                }

                size_t value_length = (size_t)(value_end - current);
                if (value_length > 0) {
                    char *name = (char *)malloc((value_length + 1) * sizeof(char));
                    if (name != NULL) {
                        strncpy(name, current, value_length);
                        name[value_length] = '\0';
                        return name;
                    }
                }
            }
        }
        current++;
    }
    return strdup(""); // Return an empty string if NAME= is not found or if value is empty
}

// Parse object definitions and state changes from G-code
// Returns: true if object state changed, false otherwise
bool parse_object_commands(const char *line, ObjectInfo **object_infos, int *num_objects,
                           int *current_object) {
    if (line == NULL) return false;

    if (strstr(line, "EXCLUDE_OBJECT_DEFINE") != NULL) {
        char *name_pos = strstr(line, "NAME=");
        if (name_pos != NULL) {
            char *name_start = name_pos + 5;
            const char *name_end = strchr(name_start, ' ');
            if (name_end == NULL) {
                name_end = strchr(name_start, '\n');
            }
            if (name_end == NULL) {
                name_end = strchr(name_start, '\r');
            }
            if (name_end == NULL) {
                name_end = name_start + strlen(name_start);
            }

            if (name_end > name_start) {
                size_t name_length = (size_t)(name_end - name_start);
                ObjectInfo *temp = realloc(*object_infos, ((size_t)*num_objects + 1) * sizeof(ObjectInfo));
                if (temp == NULL) {
                    log_this(SR_BERYLLIUM, "Memory reallocation failed for object_infos", LOG_LEVEL_ERROR, 0);
                    return false;
                }
                *object_infos = temp;
                (*object_infos)[*num_objects].name = strndup(name_start, name_length);
                if ((*object_infos)[*num_objects].name == NULL) {
                    log_this(SR_BERYLLIUM, "Memory allocation failed for object name", LOG_LEVEL_ERROR, 0);
                    return false;
                }
                (*object_infos)[*num_objects].index = *num_objects;
                (*num_objects)++;
            }
            return true;
        }
        return false;  // Malformed EXCLUDE_OBJECT_DEFINE without NAME=
    }

    if (strstr(line, "EXCLUDE_OBJECT_START") != NULL) {
        char *name_pos = strstr(line, "NAME=");
        if (name_pos != NULL) {
            char *name_start = name_pos + 5;
            const char *name_end = strchr(name_start, ' ');
            if (name_end == NULL) {
                name_end = strchr(name_start, '\n');
            }
            if (name_end == NULL) {
                name_end = strchr(name_start, '\r');
            }
            if (name_end == NULL) {
                name_end = name_start + strlen(name_start);
            }

            if (name_end > name_start) {
                size_t name_length = (size_t)(name_end - name_start);
                char object_name[name_length + 1];
                strncpy(object_name, name_start, name_length);
                object_name[name_length] = '\0';

                // Find the index of the current object based on its name
                for (int i = 0; i < *num_objects; i++) {
                    if (*object_infos != NULL && (*object_infos)[i].name != NULL &&
                        strcmp((*object_infos)[i].name, object_name) == 0) {
                        *current_object = i;
                        break;
                    }
                }
            }
            return true;
        }
        return false;  // Malformed EXCLUDE_OBJECT_START without NAME=
    }

    if (strstr(line, "EXCLUDE_OBJECT_END") != NULL) {
        *current_object = -1;
        return true;
    }
    return false;
}

// Parse layer changes from G-code metadata
// Returns -1 for non-layer lines to distinguish from layer 0
int parse_current_layer(const char *line) {
    if (line == NULL) return -1;

    // Check for SET_PRINT_STATS_INFO format (Klipper/PrusaSlicer)
    char *pos = strstr(line, "SET_PRINT_STATS_INFO CURRENT_LAYER=");
    if (pos != NULL) {
        return atoi(pos + strlen("SET_PRINT_STATS_INFO CURRENT_LAYER="));
    }

    // Check for LAYER_CHANGE format (Cura/Simplify3D/Marlin)
    pos = strstr(line, "LAYER_CHANGE");
    if (pos != NULL) {
        // Look for layer number after LAYER_CHANGE
        // Format: ;LAYER_CHANGE\n;Z:0.2\n or ;LAYER:5\n
        const char *current = pos + strlen("LAYER_CHANGE");
        while (*current != '\0') {
            if (strncmp(current, ";Z:", 3) == 0) {
                // Z height format - not a direct layer number, return -1
                return -1;
            } else if (strncmp(current, ";LAYER:", 7) == 0) {
                // Direct layer number format
                return atoi(current + 7);
            }
            current++;
        }
    }

    // Check for ;LAYER: format (standalone layer comments)
    pos = strstr(line, ";LAYER:");
    if (pos != NULL) {
        return atoi(pos + 7);
    }

    return -1;
}

// Process G-code movement commands (G0, G1, G4, G92)
// Returns: movement time added to print duration
double process_movement_command(const char *line, BerylliumConfig *config,
                                double *current_x, double *current_y, double *current_z,
                                double *extrusion, double *current_extrusion_pos,
                                bool *relative_mode, bool *relative_extrusion,
                                double *current_feedrate, double *z_values, int *z_values_count,
                                int *z_values_capacity, int current_layer, int current_object,
                                int num_objects, double ***object_times) {
    if (line == NULL) return 0.0;
    double move_time = 0.0;

    if (strstr(line, "G91") != NULL) {
        *relative_mode = true;
        *relative_extrusion = true;
    } else if (strstr(line, "G90") != NULL) {
        *relative_mode = false;
        *relative_extrusion = false;
    } else if (strstr(line, "M83") != NULL) {
        *relative_extrusion = true;
    } else if (strstr(line, "M82") != NULL) {
        *relative_extrusion = false;
    } else if (strncmp(line, "G1 ", 3) == 0 || strncmp(line, "G0 ", 3) == 0) {
        double x = parse_parameter(line, "X");
        double y = parse_parameter(line, "Y");
        double z = parse_parameter(line, "Z");
        double e = parse_parameter(line, "E");
        double f = parse_parameter(line, "F");

        // Update current feedrate if F parameter is specified
        if (!isnan(f) && f > 0.0) {
            *current_feedrate = f;
        }
        double max_speed_xy = isnan(e) || e <= 0.0 ? config->max_speed_travel : config->max_speed_xy;

        double next_x = *relative_mode ? (isnan(x) ? *current_x : *current_x + x) : (isnan(x) ? *current_x : x);
        double next_y = *relative_mode ? (isnan(y) ? *current_y : *current_y + y) : (isnan(y) ? *current_y : y);
        double next_z = *relative_mode ? (isnan(z) ? *current_z : *current_z + z) : (isnan(z) ? *current_z : z);

        double distance_xy = sqrt(pow(next_x - *current_x, 2) + pow(next_y - *current_y, 2));
        double distance_z = fabs(next_z - *current_z);

        // Only calculate movement time if there's actually movement
        if (distance_xy > 0.0 || distance_z > 0.0 || !isnan(e)) {
            double max_velocity_xy = fmin(*current_feedrate / 60.0, max_speed_xy);
            double max_velocity_z = fmin(*current_feedrate / 60.0, config->max_speed_z);
            double max_velocity_e = fmin(*current_feedrate / 60.0, config->max_speed_xy);

            double time_xy = distance_xy > 0.0 ? accelerated_move(distance_xy, config->acceleration, max_velocity_xy) : 0.0;
            double time_z = distance_z > 0.0 ? accelerated_move(distance_z, config->z_acceleration, max_velocity_z) : 0.0;
            double time_e = !isnan(e) && fabs(e) > 0.0 ? accelerated_move(fabs(e), config->extruder_acceleration, max_velocity_e) : 0.0;

            // Assume XY and E move concurrently, Z moves separately
            move_time = fmax(time_xy, time_e) + time_z;
        }

        // Track object time if we have valid layer and object
        if (object_times && *object_times && current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS &&
            current_object < num_objects && (*object_times)[current_layer] != NULL) {
            (*object_times)[current_layer][current_object] += move_time;
        }

        *current_x = next_x;
        *current_y = next_y;

        // Track Z values for layer height calculation
        if (*current_z != next_z) {
            bool z_exists = false;
            for (int i = 0; i < *z_values_count; i++) {
                if (fabs(z_values[i] - next_z) < 1e-6) {
                    z_exists = true;
                    break;
                }
            }

            if (!z_exists) {
                if (*z_values_count == *z_values_capacity) {
                    *z_values_capacity += Z_VALUES_CHUNK_SIZE;
                    double *new_z_values = realloc(z_values, (size_t)*z_values_capacity * sizeof(double));
                    if (new_z_values == NULL) {
                        log_this(SR_BERYLLIUM, "Memory reallocation failed for z_values", LOG_LEVEL_ERROR, 0);
                        return 0.0;
                    }
                    z_values = new_z_values;
                }
                z_values[(*z_values_count)++] = next_z;
            }

            *current_z = next_z;
        }

        // Handle extrusion
        if (!isnan(e)) {
            if (*relative_extrusion) {
                *extrusion += e;
                *current_extrusion_pos += e;
            } else {
                // In absolute mode, calculate the difference from current position
                double extrusion_diff = e - *current_extrusion_pos;
                if (extrusion_diff > 0.0) {  // Only count positive extrusion
                    *extrusion += extrusion_diff;
                }
                *current_extrusion_pos = e;
            }
        }
    } else if (strncmp(line, "G4 ", 3) == 0) {
        // Handle G4 (dwell) command
        double p = parse_parameter(line, "P") / 1000.0;  // P is in milliseconds
        double s = parse_parameter(line, "S");  // S is in seconds
        double time = 0.0;
        if (!isnan(p) && p > 0.0) {
            time = p;
        } else if (!isnan(s) && s > 0.0) {
            time = s;
        }

        if (time > 0.0) {
            move_time = time;
            if (object_times && *object_times && current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS &&
                current_object < num_objects && (*object_times)[current_layer] != NULL) {
                (*object_times)[current_layer][current_object] += time;
            }
        }
    } else if (strncmp(line, "G92", 3) == 0) {
        // Handle G92 (set position) command - might reset extrusion
        double e = parse_parameter(line, "E");
        if (!isnan(e)) {
            *current_extrusion_pos = e;
            // Don't modify total extrusion count when resetting position
        }
    }

    return move_time;
}

// Calculate move duration using real-world motion profiles
// Uses trapezoidal velocity profiles for accuracy and performance
double accelerated_move(double length, double acceleration, double max_velocity) {
    if (length == 0.0) return 0.0;
    // Handle edge cases that would cause division by zero or invalid math
    if (acceleration <= 0.0 || max_velocity <= 0.0) return 0.0;  // Can't move with zero or negative acceleration/velocity

    double accel_distance = max_velocity * max_velocity / (2.0 * acceleration);

    if (length <= 2.0 * accel_distance) {
        // Triangle profile (no constant velocity phase)
        double peak_velocity = sqrt(acceleration * length);
        return 2.0 * peak_velocity / acceleration;
    } else {
        // Trapezoidal profile
        double accel_time = max_velocity / acceleration;
        double const_time = (length - 2.0 * accel_distance) / max_velocity;
        return 2.0 * accel_time + const_time;
    }
}

BerylliumStats beryllium_analyze_gcode(FILE *file, const BerylliumConfig *config) {
    BerylliumStats stats = {0};
    stats.success = false;  // Initialize success flag to false
    stats.object_times = NULL;  // Explicitly initialize pointer to NULL
    stats.object_infos = NULL;  // Explicitly initialize pointer to NULL

    // Check for NULL parameters
    if (file == NULL || config == NULL) return stats;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0, extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    char line[MAX_LINE_LENGTH];
    double layer_start_time = 0.0;
    int current_layer = -1;
    int current_object = -1;
    double current_feedrate = config->default_feedrate;  // Persist feedrate between commands

    ObjectInfo *object_infos = NULL;
    int num_objects = 0;

    // Allocate memory for stats.object_times
    stats.object_times = calloc(MAX_LAYERS, sizeof(double *));
    if (stats.object_times == NULL) {
        log_this(SR_BERYLLIUM, "Memory allocation failed for object_times", LOG_LEVEL_ERROR, 0);
        return stats;
    }

    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    stats.file_size = (file_size_long >= 0 && file_size_long <= INT_MAX) ? (int)file_size_long : 0;
    rewind(file); // Reset the file pointer to the beginning

    double *z_values = calloc(Z_VALUES_CHUNK_SIZE, sizeof(double));  // Start with configured chunk size
    if (z_values == NULL) {
        log_this(SR_BERYLLIUM, "Memory allocation failed for z_values", LOG_LEVEL_ERROR, 0);
        free(stats.object_times);
        stats.object_times = NULL;
        stats.success = false;
        return stats;
    }
    int z_values_count = 0, z_values_capacity = Z_VALUES_CHUNK_SIZE;


    while (fgets(line, sizeof(line), file) != NULL) {
        stats.total_lines++;
        if (line[0] == 'G' || line[0] == 'M') {
            stats.gcode_lines++;
        }

        if (strstr(line, "EXCLUDE_OBJECT_DEFINE") != NULL) {
            char *name_pos = strstr(line, "NAME=");
            if (name_pos != NULL) {
                char *name_start = name_pos + 5;
                const char *name_end = strchr(name_start, ' ');
                if (name_end == NULL) {
                    name_end = strchr(name_start, '\n');
                }
                if (name_end == NULL) {
                    name_end = strchr(name_start, '\r');
                }
                if (name_end == NULL) {
                    name_end = name_start + strlen(name_start);
                }

                if (name_end > name_start) {
                    size_t name_length = (size_t)(name_end - name_start);
                    ObjectInfo *temp = realloc(object_infos, ((size_t)num_objects + 1) * sizeof(ObjectInfo));
                    if (temp == NULL) {
                        log_this(SR_BERYLLIUM, "Memory reallocation failed for object_infos", LOG_LEVEL_ERROR, 0);
                        free(stats.object_times);
                        stats.object_times = NULL;
                        free(z_values);
                        for (int i = 0; i < num_objects; i++) {
                            free(object_infos[i].name);
                        }
                        free(object_infos);
                        stats.success = false;
                        return stats;
                    }
                    object_infos = temp;
                    object_infos[num_objects].name = strndup(name_start, name_length);
                    //printf("Object defined: %s\n",object_infos[num_objects].name);
                    if (object_infos[num_objects].name == NULL) {
                        log_this(SR_BERYLLIUM, "Memory allocation failed for object name", LOG_LEVEL_ERROR, 0);
                        free(stats.object_times);
                        stats.object_times = NULL;
                        free(z_values);
                        for (int i = 0; i < num_objects; i++) {
                            free(object_infos[i].name);
                        }
                        free(object_infos);
                        stats.success = false;
                        return stats;
                    }
                    object_infos[num_objects].index = num_objects;
                    num_objects++;
                }
            }
        }

       if (strstr(line, "EXCLUDE_OBJECT_START") != NULL) {
            char *name_pos = strstr(line, "NAME=");
            if (name_pos != NULL) {
                char *name_start = name_pos + 5;
                const char *name_end = strchr(name_start, ' ');
                if (name_end == NULL) {
                    name_end = strchr(name_start, '\n');
                }
                if (name_end == NULL) {
                    name_end = strchr(name_start, '\r');
                }
                if (name_end == NULL) {
                    name_end = name_start + strlen(name_start);
                }

                if (name_end > name_start) {
                    size_t name_length = (size_t)(name_end - name_start);
                    char object_name[name_length + 1];
                    strncpy(object_name, name_start, name_length);
                    object_name[name_length] = '\0';

                    // printf("Looking for object: [%s]\n", object_name);
                    // Find the index of the current object based on its name
                    for (int i = 0; i < num_objects; i++) {
                        if (object_infos != NULL && object_infos[i].name != NULL && strcmp(object_infos[i].name, object_name) == 0) {
                            // printf("Match found: [%s] at index %d\n", object_infos[i].name, i);
                            current_object = i;
                            break;
                        }
                    }
                }
            }
	    // printf("Object started: %d\n", current_object);
        }
       if (strstr(line, "EXCLUDE_OBJECT_END") != NULL) {
	    // printf("Object ended: %d\n", current_object);
            current_object = -1;
        }

        int layer = parse_current_layer(line);
        if (layer >= 0) {
            if (current_layer >= 0 && current_layer < MAX_LAYERS) {
                stats.layer_times[current_layer] = stats.print_time - layer_start_time;
            }
            current_layer = layer;
//	    current_object = -1;
            layer_start_time = stats.print_time;
            stats.layer_count_slicer = (int)fmax(stats.layer_count_slicer, current_layer + 1);

                if (current_layer < MAX_LAYERS) {
                if (stats.object_times[current_layer] == NULL) {
                    stats.object_times[current_layer] = calloc((size_t)num_objects, sizeof(double));
                    if (stats.object_times[current_layer] == NULL) {
                        log_this(SR_BERYLLIUM, "Memory allocation failed for layer object times", LOG_LEVEL_ERROR, 0);
                        // Free previously allocated layer arrays
                        for (int i = 0; i < current_layer; i++) {
                            free(stats.object_times[i]);
                        }
                        free(stats.object_times);
                        stats.object_times = NULL;
                        free(z_values);
                        for (int i = 0; i < num_objects; i++) {
                            free(object_infos[i].name);
                        }
                        free(object_infos);
                        stats.success = false;
                        return stats;
                    }
                }
            }
        }

        if (strstr(line, "G91") != NULL) {
            relative_mode = true;
            relative_extrusion = true;
        } else if (strstr(line, "G90") != NULL) {
            relative_mode = false;
            relative_extrusion = false;
        } else if (strstr(line, "M83") != NULL) {
            relative_extrusion = true;
        } else if (strstr(line, "M82") != NULL) {
            relative_extrusion = false;
        } else if (strncmp(line, "G1 ", 3) == 0 || strncmp(line, "G0 ", 3) == 0) {
            double x = parse_parameter(line, "X");
            double y = parse_parameter(line, "Y");
            double z = parse_parameter(line, "Z");
            double e = parse_parameter(line, "E");
            double f = parse_parameter(line, "F");

            // Update current feedrate if F parameter is specified
            if (!isnan(f) && f > 0.0) {
                current_feedrate = f;
            }
            double max_speed_xy = isnan(e) || e <= 0.0 ? config->max_speed_travel : config->max_speed_xy;

            double next_x = relative_mode ? (isnan(x) ? current_x : current_x + x) : (isnan(x) ? current_x : x);
            double next_y = relative_mode ? (isnan(y) ? current_y : current_y + y) : (isnan(y) ? current_y : y);
            double next_z = relative_mode ? (isnan(z) ? current_z : current_z + z) : (isnan(z) ? current_z : z);

            double distance_xy = sqrt(pow(next_x - current_x, 2) + pow(next_y - current_y, 2));
            double distance_z = fabs(next_z - current_z);

            // Only calculate movement time if there's actually movement
            double move_time = 0.0;
            if (distance_xy > 0.0 || distance_z > 0.0 || !isnan(e)) {
                double max_velocity_xy = fmin(current_feedrate / 60.0, max_speed_xy);
                double max_velocity_z = fmin(current_feedrate / 60.0, config->max_speed_z);
                double max_velocity_e = fmin(current_feedrate / 60.0, config->max_speed_xy);

                double time_xy = distance_xy > 0.0 ? accelerated_move(distance_xy, config->acceleration, max_velocity_xy) : 0.0;
                double time_z = distance_z > 0.0 ? accelerated_move(distance_z, config->z_acceleration, max_velocity_z) : 0.0;
                double time_e = !isnan(e) && fabs(e) > 0.0 ? accelerated_move(fabs(e), config->extruder_acceleration, max_velocity_e) : 0.0;

                // Assume XY and E move concurrently, Z moves separately
                move_time = fmax(time_xy, time_e) + time_z;
            }
            stats.print_time += move_time;

	        // printf("Current layer: %d, Current object: %d, Move time: %.2f\n", current_layer, current_object, move_time);

    if (stats.object_times && current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS && current_object < num_objects && stats.object_times[current_layer] != NULL) {
        stats.object_times[current_layer][current_object] += move_time;
    }

            current_x = next_x;
            current_y = next_y;

            if (current_z != next_z) {
                bool z_exists = false;
                for (int i = 0; i < z_values_count; i++) {
                    if (fabs(z_values[i] - next_z) < 1e-6) {
                        z_exists = true;
                        break;
                    }
                }

                if (!z_exists) {
                    if (z_values_count == z_values_capacity) {
                        z_values_capacity += Z_VALUES_CHUNK_SIZE;
                        double *new_z_values = realloc(z_values, (size_t)z_values_capacity * sizeof(double));
                        if (new_z_values == NULL) {
                            log_this(SR_BERYLLIUM, "Memory reallocation failed for z_values", LOG_LEVEL_ERROR, 0);
                            free(z_values);
                            for (int i = 0; i < stats.layer_count_slicer; i++) {
                                free(stats.object_times[i]);
                            }
                            free(stats.object_times);
                            stats.object_times = NULL;
                            for (int i = 0; i < num_objects; i++) {
                                free(object_infos[i].name);
                            }
                            free(object_infos);
                            stats.success = false;
                            return stats;
                        }
                        z_values = new_z_values;
                    }
                    z_values[z_values_count++] = next_z;
                }

                current_z = next_z;
            }

            if (!isnan(e)) {
                if (relative_extrusion) {
                    extrusion += e;
                    current_extrusion_pos += e;
                } else {
                    // In absolute mode, calculate the difference from current position
                    double extrusion_diff = e - current_extrusion_pos;
                    if (extrusion_diff > 0.0) {  // Only count positive extrusion
                        extrusion += extrusion_diff;
                    }
                    current_extrusion_pos = e;
                }
            }
            // Note: G1 moves without E parameters are travel moves and should not extrude
        } else if (strncmp(line, "G4 ", 3) == 0) {
            // Handle G4 (dwell) command
            double p = parse_parameter(line, "P") / 1000.0;  // P is in milliseconds
            double s = parse_parameter(line, "S");  // S is in seconds
            double time = 0.0;
            if (!isnan(p) && p > 0.0) {
                time = p;
            } else if (!isnan(s) && s > 0.0) {
                time = s;
            }

            if (time > 0.0) {
                stats.print_time += time;
                if (current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS && current_object < num_objects && stats.object_times[current_layer] != NULL) {
                    stats.object_times[current_layer][current_object] += time;
                }
            }
        } else if (strncmp(line, "G92", 3) == 0) {
            // Handle G92 (set position) command - might reset extrusion
            double e = parse_parameter(line, "E");
            if (!isnan(e)) {
                current_extrusion_pos = e;
                // Don't modify total extrusion count when resetting position
            }
        }
    }

    // Handle the last layer
    if (current_layer >= 0 && current_layer < MAX_LAYERS) {
        stats.layer_times[current_layer] = stats.print_time - layer_start_time;
    }

    stats.layer_count_height = z_values_count;

    // Calculate layer height from Z values
    stats.layer_height = calculate_layer_height(z_values, z_values_count);

    double filament_radius = config->filament_diameter / 2.0;
    stats.extrusion = extrusion;
    stats.filament_volume = M_PI * pow(filament_radius, 2) * extrusion / 1000.0;  // Volume in cm^3
    stats.filament_weight = stats.filament_volume * config->filament_density;  // Weight in grams

    // Set final stats values
    stats.object_infos = object_infos;
    stats.num_objects = num_objects;
    stats.success = true;  // Confirm success before returning

    // Note: We don't free object_times or its arrays here
    // The caller is responsible for freeing memory after using the stats
    free(z_values);

    return stats;
}

void beryllium_format_stats(const BerylliumStats *stats) {
    if (!stats || !stats->success) return;

    // Format time as Hh Mm Ss
    int hours = (int)(stats->print_time / 3600);
    int minutes = (int)((stats->print_time - hours * 3600) / 60);
    int seconds = (int)(stats->print_time - hours * 3600 - minutes * 60);

    // Calculate total height from layer count * layer height
    double total_height = stats->layer_count_height * stats->layer_height;

    printf("=== G-code Analysis Results ===\n");
    printf("- File: %s bytes\n", format_number_with_separators((double)stats->file_size, -1));
    printf("- Lines: %s total / %s gcode\n",
           format_number_with_separators((double)stats->total_lines, -1),
           format_number_with_separators((double)stats->gcode_lines, -1));
    printf("- Layers: %s layers, %.1f mm height\n",
           format_number_with_separators((double)stats->layer_count_height, -1),
           total_height);
    printf("- Print Time: %dh %02dm %02ds\n", hours, minutes, seconds);
    printf("- Filament: %s mm (%.1f meters)\n",
           format_number_with_separators(stats->extrusion, 1),
           stats->extrusion / 1000.0);
    printf("- Material: %.1f cm³ / %.1f g\n", stats->filament_volume, stats->filament_weight);
    printf("- Objects: %s\n", format_number_with_separators((double)stats->num_objects, -1));
    printf("Beryllium analysis completed successfully\n");
}

// Clean up analysis results with complete resource release
// Preserves computed values while freeing allocated memory
void beryllium_free_stats(BerylliumStats *stats) {
    if (!stats) return;

    // Free object_times array and its layer arrays
    if (stats->object_times) {
        for (int i = 0; i < stats->layer_count_slicer; i++) {
            free(stats->object_times[i]);
        }
        free(stats->object_times);
        stats->object_times = NULL;
    }

    // Free object_infos array and object names
    if (stats->object_infos) {
        for (int i = 0; i < stats->num_objects; i++) {
            free(stats->object_infos[i].name);
        }
        free(stats->object_infos);
        stats->object_infos = NULL;
    }

    // Reset counters - these are safe to reset as they're not core metrics
    stats->num_objects = 0;
    stats->layer_count_slicer = 0;

    // DO NOT reset the success flag or any computed metrics!
    // The caller needs to access stats.success, stats.print_time, etc. after calling this function
    // Only reset success to false in the analysis function when there's an actual error
}
