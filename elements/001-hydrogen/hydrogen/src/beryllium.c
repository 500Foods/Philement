#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "beryllium.h"

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

static double parse_parameter(const char *line, const char *parameter) {
    char *pos = strstr(line, parameter);
    if (pos != NULL) {
        return strtod(pos + strlen(parameter), NULL);
    }
    return 0.0;
}

char* parse_parameter_string(const char *line, const char *parameter) {
    char *pos = strstr(line, parameter);
    if (pos != NULL) {
        // Find the start of the value (after '=')
        pos = strchr(pos, '=');
        if (pos != NULL) {
            pos++; // Move past '='
            // Find the end of the value (space or end of string)
            char *end = strchr(pos, ' ');
            if (end == NULL) {
                end = pos + strlen(pos);
            }
            // Allocate memory and copy the value
            int length = end - pos;
            char *value = malloc(length + 1);
            if (value != NULL) {
                strncpy(value, pos, length);
                value[length] = '\0';
                return value;
            }
        }
    }
    return strdup("undefined");
}

char* parse_name_parameter(const char *line) {
    char *name_start = strstr(line, "NAME=");
    if (name_start != NULL) {
        name_start += 5; // Skip "NAME="
        char *name_end = strchr(name_start, ' ');
        if (name_end != NULL) {
            int name_length = (int)(name_end - name_start);
            char *name = (char *)malloc((name_length + 1) * sizeof(char));
            if (name != NULL) {
                strncpy(name, name_start, name_length);
                name[name_length] = '\0';
                return name;
            }
        }
    }
    return strdup(""); // Return an empty string if NAME= is not found
}

static int parse_current_layer(const char *line) {
    char *pos = strstr(line, "SET_PRINT_STATS_INFO CURRENT_LAYER=");
    if (pos != NULL) {
        return atoi(pos + strlen("SET_PRINT_STATS_INFO CURRENT_LAYER="));
    }
    return -1;
}


static int accelerated_move(double length, double acceleration, double max_velocity) {
    if (length == 0.0) {
        return 0.0;
    }

    double accel_distance = max_velocity * max_velocity / (2.0 * acceleration);

    if (length <= 2.0 * accel_distance) {
        // Triangle profile (no constant velocity phase)
        double peak_velocity = sqrt(acceleration * length);
        return length / peak_velocity;
    } else {
        // Trapezoidal profile
        double accel_time = max_velocity / acceleration;
        double const_time = (length - 2.0 * accel_distance) / max_velocity;
        return 2.0 * accel_time + const_time;
    }
}

BerylliumStats beryllium_analyze_gcode(FILE *file, const BerylliumConfig *config) {
    BerylliumStats stats = {0};
    double current_x = 0.0, current_y = 0.0, current_z = 0.0, extrusion = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    char line[MAX_LINE_LENGTH];
    double layer_start_time = 0.0;
    int current_layer = -1;
    int current_object = -1;

    ObjectInfo *object_infos = NULL;
    int num_objects = 0;

    // Allocate memory for stats.object_times
    stats.object_times = calloc(MAX_LAYERS, sizeof(double *));
    if (stats.object_times == NULL) {
        fprintf(stderr, "Memory allocation failed for object_times\n");
        return stats;
    }

    fseek(file, 0, SEEK_END);
    stats.file_size = ftell(file);
    rewind(file); // Reset the file pointer to the beginning

    double *z_values = calloc(100, sizeof(double));  // Start with 100 elements
    if (z_values == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(stats.object_times);
        return stats;
    }
    int z_values_count = 0, z_values_capacity = 100;


    while (fgets(line, sizeof(line), file) != NULL) {
        stats.total_lines++;
        if (line[0] == 'G' || line[0] == 'M') {
            stats.gcode_lines++;
        }

        if (strstr(line, "EXCLUDE_OBJECT_DEFINE") != NULL) {
            char *name_start = strstr(line, "NAME=") + 5;
            char *name_end = strchr(name_start, ' ');
            if (name_end) {
                int name_length = name_end - name_start;
                object_infos = realloc(object_infos, (num_objects + 1) * sizeof(ObjectInfo));
                if (object_infos == NULL) {
                    fprintf(stderr, "Memory reallocation failed for object_infos\n");
                    free(stats.object_times);
                    free(z_values);
                    for (int i = 0; i < num_objects; i++) {
                        free(object_infos[i].name);
                    }
                    free(object_infos);
                    return stats;
                }
                object_infos[num_objects].name = strndup(name_start, name_length);
	    //printf("Object defined: %s\n",object_infos[num_objects].name);
                if (object_infos[num_objects].name == NULL) {
                    fprintf(stderr, "Memory allocation failed for object name\n");
                    free(stats.object_times);
                    free(z_values);
                    for (int i = 0; i < num_objects; i++) {
                        free(object_infos[i].name);
                    }
                    free(object_infos);
                    return stats;
                }
                object_infos[num_objects].index = num_objects;
                num_objects++;
            }
        }

       if (strstr(line, "EXCLUDE_OBJECT_START") != NULL) {
            char *name_start = strstr(line, "NAME=") + 5;
            char *name_end = strchr(name_start, ' ');
                int name_length = name_end - name_start;
	    //printf("%d\n", name_length);
            if (name_end) {
                char object_name[name_length + 1];
                strncpy(object_name, name_start, name_length);
                object_name[name_length] = '\0';

		// printf("[%s]\n",object_name);
                // Find the index of the current object based on its name
                for (int i = 0; i < num_objects; i++) {
                    if (strcmp(object_infos[i].name, object_name) == 0) {
		// printf("Match: [%s]\n",object_infos[i].name);
                        current_object = i;
                        break;
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
            stats.layer_count_slicer = fmax(stats.layer_count_slicer, current_layer + 1);

            if (current_layer >= 0 && current_layer < MAX_LAYERS) {
                if (stats.object_times[current_layer] == NULL) {
                    stats.object_times[current_layer] = calloc(num_objects, sizeof(double));
                    if (stats.object_times[current_layer] == NULL) {
                        fprintf(stderr, "Memory allocation failed for object_times[%d]\n", current_layer);
                        free(stats.object_times);
                        free(z_values);
                        for (int i = 0; i < num_objects; i++) {
                            free(object_infos[i].name);
                        }
                        free(object_infos);
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

            double feedrate = (f > 0.0) ? f : config->default_feedrate;
            double max_speed_xy = (e > 0.0) ? config->max_speed_xy : config->max_speed_travel;

            double next_x = relative_mode ? current_x + x : (x != 0.0 ? x : current_x);
            double next_y = relative_mode ? current_y + y : (y != 0.0 ? y : current_y);
            double next_z = relative_mode ? current_z + z : (z != 0.0 ? z : current_z);

            double distance_xy = sqrt(pow(next_x - current_x, 2) + pow(next_y - current_y, 2));
            double distance_z = fabs(next_z - current_z);

            double max_velocity_xy = fmin(feedrate / 60.0, max_speed_xy);
            double max_velocity_z = fmin(feedrate / 60.0, config->max_speed_z);
            double max_velocity_e = fmin(feedrate / 60.0, config->max_speed_xy);

            double time_xy = accelerated_move(distance_xy, config->acceleration, max_velocity_xy);
            double time_z = accelerated_move(distance_z, config->z_acceleration, max_velocity_z);
            double time_e = accelerated_move(fabs(e), config->extruder_acceleration, max_velocity_e);

            // Assume XY and E move concurrently, Z moves separately
            double move_time = fmax(time_xy, time_e) + time_z;
            stats.print_time += move_time;

	        // printf("Current layer: %d, Current object: %d, Move time: %.2f\n", current_layer, current_object, move_time);

    if (current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS && current_object < num_objects && stats.object_times[current_layer] != NULL) {
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
                        z_values_capacity += 100;
                        double *new_z_values = realloc(z_values, z_values_capacity * sizeof(double));
                        if (new_z_values == NULL) {
                            fprintf(stderr, "Memory reallocation failed\n");
                            free(z_values);
                            for (int i = 0; i < stats.layer_count_slicer; i++) {
                                free(stats.object_times[i]);
                            }
                            free(stats.object_times);
                            for (int i = 0; i < num_objects; i++) {
                                free(object_infos[i].name);
                            }
                            free(object_infos);
                            return stats;
                        }
                        z_values = new_z_values;
                    }
                    z_values[z_values_count++] = next_z;
                }

                current_z = next_z;
            }

            if (relative_extrusion) {
                extrusion += e;
            } else {
                extrusion = e;
            }
        } else if (strncmp(line, "G4 ", 3) == 0) {
            // Handle G4 (dwell) command
            double p = parse_parameter(line, "P") / 1000.0;  // P is in milliseconds
            double s = parse_parameter(line, "S");  // S is in seconds
            double time = (p > 0.0) ? p : s;
            stats.print_time += time;

            if (current_layer >= 0 && current_object >= 0 && current_layer < MAX_LAYERS && current_object < num_objects && stats.object_times[current_layer] != NULL) {
                stats.object_times[current_layer][current_object] += time;
            }
        }
    }

    // Handle the last layer
    if (current_layer >= 0 && current_layer < MAX_LAYERS) {
        stats.layer_times[current_layer] = stats.print_time - layer_start_time;
    }

    stats.layer_count_height = z_values_count;

    double filament_radius = config->filament_diameter / 2.0;
    stats.extrusion = extrusion;
    stats.filament_volume = M_PI * pow(filament_radius, 2) * extrusion / 1000.0;  // Volume in cm^3
    stats.filament_weight = stats.filament_volume * config->filament_density;  // Weight in grams

    stats.object_infos = object_infos;
    stats.num_objects = num_objects;

        free(z_values);
	    // Debugging statements
    // printf("Debugging stats:\n");
    // printf("Number of layers (height): %d\n", stats.layer_count_height);
    // printf("Number of layers (slicer): %d\n", stats.layer_count_slicer);
    // printf("Number of objects: %d\n", stats.num_objects);

    // printf("Layer times:\n");
    // for (int i = 0; i < stats.layer_count_slicer; i++) {
        // printf("Layer %d: %.2f\n", i + 1, stats.layer_times[i]);
    // }

    // printf("Object times:\n");
    // for (int i = 0; i < stats.layer_count_slicer; i++) {
        // if (stats.object_times[i] != NULL) {
            // for (int j = 0; j < stats.num_objects; j++) {
                // printf("Layer %d, Object %d: %.2f\n", i + 1, j + 1, stats.object_times[i][j]);
            // }
        // }
    // }

    return stats;
}
