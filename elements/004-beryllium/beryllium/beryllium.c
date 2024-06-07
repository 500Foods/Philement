// beryllium.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "beryllium.h"

static double acceleration = 1000.0;  // Default acceleration (mm/s^2)
static double z_acceleration = 250.0;  // Z-axis acceleration (mm/s^2)
static double extruder_acceleration = 2000.0;  // Extruder acceleration (mm/s^2)

char* get_iso8601_timestamp(void) {
    static char buffer[sizeof "2011-10-08T07:07:09Z"];
    time_t now = time(NULL);
    strftime(buffer, sizeof buffer, "%FT%TZ", gmtime(&now));
    return buffer;
}

void format_time(double seconds, char *buffer) {
    int hours = (int)(seconds / 3600);
    seconds -= hours * 3600;
    int minutes = (int)(seconds / 60);
    seconds -= minutes * 60;
    sprintf(buffer, "%dh %dm %.0fs", hours, minutes, seconds);
}

static double parse_parameter(const char *line, const char *parameter) {
    char *pos = strstr(line, parameter);
    if (pos != NULL) {
        return strtod(pos + strlen(parameter), NULL);
    }
    return 0.0;
}

static double accelerated_move(double length, double acceleration, double velocity, double feedrate) {
    if (length == 0.0) {
        return 0.0;
    }
    double max_velocity = fmax(velocity, feedrate / 60.0);
    double half_length = length / 2.0;
    double t_init = max_velocity / acceleration;
    double dx_init = 0.5 * acceleration * pow(t_init, 2);
    double t = 0.0;

    if (half_length >= dx_init) {
        half_length -= dx_init;
        t += t_init;
    }

    t += half_length / max_velocity;
    return t * 2.0;
}

BerylliumStats beryllium_analyze_gcode(FILE *file) {
    BerylliumStats stats = {0};
    double current_x = 0.0, current_y = 0.0, current_z = 0.0, extrusion = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double velocity = 5000.0 / 60.0;  // Default velocity (mm/s)
    char line[MAX_LINE_LENGTH];

    double *z_values = NULL;
    int z_values_count = 0, z_values_capacity = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        stats.total_lines++;
        if (line[0] == 'G' || line[0] == 'M') {
            stats.gcode_lines++;
        }

        // Parse G-code commands (G91, G90, M83, M82, G92, M204, G1, G0, G28)
        // Update variables and stats accordingly
        // This part remains largely the same as in your original code

    }

    stats.layer_count = z_values_count;

    fseek(file, 0, SEEK_END);
    stats.file_size = ftell(file);

    double filament_radius = FILAMENT_DIAMETER / 2.0;
    stats.extrusion = extrusion;
    stats.filament_volume = M_PI * pow(filament_radius, 2) * extrusion / 1000.0;  // Volume in cm^3
    stats.filament_weight = stats.filament_volume * FILAMENT_DENSITY;  // Weight in grams
    stats.print_time = print_time;

    free(z_values);
    return stats;
}
