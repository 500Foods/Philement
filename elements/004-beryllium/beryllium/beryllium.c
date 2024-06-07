// beryllium.c
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

static int parse_current_layer(const char *line) {
    char *pos = strstr(line, "SET_PRINT_STATS_INFO CURRENT_LAYER=");
    if (pos != NULL) {
        return atoi(pos + strlen("SET_PRINT_STATS_INFO CURRENT_LAYER="));
    }
    return -1;
}

static double accelerated_move(double length, double acceleration, double max_velocity) {
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

    fseek(file, 0, SEEK_END);
    stats.file_size = ftell(file);
    rewind(file); // Reset the file pointer to the beginning

    double *z_values = calloc(100, sizeof(double));  // Start with 100 elements
    if (z_values == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return stats;
    }
    int z_values_count = 0, z_values_capacity = 100;

    while (fgets(line, sizeof(line), file) != NULL) {
        stats.total_lines++;
        if (line[0] == 'G' || line[0] == 'M') {
            stats.gcode_lines++;
        }

        int layer = parse_current_layer(line);
        if (layer >= 0) {
            if (current_layer >= 0 && current_layer < MAX_LAYERS) {
                stats.layer_times[current_layer] = stats.print_time - layer_start_time;
            }
            current_layer = layer;
            layer_start_time = stats.print_time;
            stats.layer_count_slicer = fmax(stats.layer_count_slicer, current_layer + 1);
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
            stats.print_time += (p > 0.0) ? p : s;
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

    free(z_values);
    return stats;
}
