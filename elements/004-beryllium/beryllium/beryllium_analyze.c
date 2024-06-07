// beryllium_analyze.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "beryllium.h"

void print_usage(const char *program_name, const BerylliumConfig *defaults) {
    printf("Philement/Beryllium G-Code Analyzer\n");
    printf("Usage: %s [OPTIONS] <filename>\n", program_name);
    printf("Options:\n");
    printf("  -a, --acceleration       ACCEL  Set acceleration           (default: %.2f mm/s^2)\n", defaults->acceleration);
    printf("  -z, --z-acceleration     ACCEL  Set Z-axis acceleration    (default: %.2f mm/s^2)\n", defaults->z_acceleration);
    printf("  -e, --extruder-accel     ACCEL  Set extruder acceleration  (default: %.2f mm/s^2)\n", defaults->extruder_acceleration);
    printf("  -x, --max-speed-xy       SPEED  Set max XY speed           (default: %.2f mm/s)\n",   defaults->max_speed_xy);
    printf("  -t, --max-speed-travel   SPEED  Set max travel speed       (default: %.2f mm/s)\n",   defaults->max_speed_travel);
    printf("  -m, --max-speed-z        SPEED  Set max Z speed            (default: %.2f mm/s)\n",   defaults->max_speed_z);
    printf("  -f, --default-feedrate   RATE   Set default feedrate       (default: %.2f mm/min)\n", defaults->default_feedrate);
    printf("  -d, --filament-diameter  DIAM   Set filament diameter      (default: %.2f mm)\n",     defaults->filament_diameter);
    printf("  -g, --filament-density   DENS   Set filament density       (default: %.2f g/cm^3)\n", defaults->filament_density);
    printf("  -l, --layertimes                Output layer times         (default: not listed)\n");
}

int main(int argc, char *argv[]) {
    BerylliumConfig config = {
        .acceleration = ACCELERATION,
        .z_acceleration = Z_ACCELERATION,
        .extruder_acceleration = E_ACCELERATION,
        .max_speed_xy = MAX_SPEED_XY,
        .max_speed_travel = MAX_SPEED_TRAVEL,
        .max_speed_z = MAX_SPEED_Z,
        .default_feedrate = DEFAULT_FEEDRATE,
        .filament_diameter = DEFAULT_FILAMENT_DIAMETER,
        .filament_density = DEFAULT_FILAMENT_DENSITY
    };

    int output_layertimes = 0;  

    static struct option long_options[] = {
        {"acceleration", required_argument, 0, 'a'},
        {"z-acceleration", required_argument, 0, 'z'},
        {"extruder-accel", required_argument, 0, 'e'},
        {"max-speed-xy", required_argument, 0, 'x'},
        {"max-speed-travel", required_argument, 0, 't'},
        {"max-speed-z", required_argument, 0, 'm'},
        {"default-feedrate", required_argument, 0, 'f'},
        {"filament-diameter", required_argument, 0, 'd'},
        {"filament-density", required_argument, 0, 'g'},
        {"layertimes", no_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:z:e:x:t:m:f:d:g:l", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a': config.acceleration = atof(optarg); break;
            case 'z': config.z_acceleration = atof(optarg); break;
            case 'e': config.extruder_acceleration = atof(optarg); break;
            case 'x': config.max_speed_xy = atof(optarg); break;
            case 't': config.max_speed_travel = atof(optarg); break;
            case 'm': config.max_speed_z = atof(optarg); break;
            case 'f': config.default_feedrate = atof(optarg); break;
            case 'd': config.filament_diameter = atof(optarg); break;
            case 'g': config.filament_density = atof(optarg); break;
            case 'l': output_layertimes = 1; break;
            default:
                print_usage(argv[0], &config);
                return 1;
        }
    }

    if (optind >= argc) {
        print_usage(argv[0], &config);
        return 1;
    }

    char *filename = argv[optind];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return 1;
    }

    char *start_time = get_iso8601_timestamp();
    printf("Philement/Beryllium G-Code Analyzer\n");
    printf("Analysis start: %s\n", start_time);

    clock_t start = clock();
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);
    clock_t end = clock();

    fclose(file);

    char print_time_str[20];
    format_time(stats.print_time, print_time_str);

    char *end_time = get_iso8601_timestamp();
    printf("Analysis end: %s\n", end_time);

    double elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Analysis duration: %.6f ms\n", elapsed_time);

    printf("\nFile size: %ld bytes\n", stats.file_size);
    printf("Total lines: %d\n", stats.total_lines);
    printf("G-code lines: %d\n", stats.gcode_lines);
    printf("Number of layers (height): %d\n", stats.layer_count_height);
    printf("Number of layers (slicer): %d\n", stats.layer_count_slicer);
    printf("Filament used: %.2f mm (%.2f cm^3)\n", stats.extrusion, stats.filament_volume);
    printf("Filament weight: %.2f grams\n", stats.filament_weight);
    printf("Estimated print time: %s\n", print_time_str);

    if (output_layertimes && stats.layer_count_slicer > 0) {
        printf("\nLayer  Start Time   End Time     Duration\n");
        double cumulative_time = 0.0;
        for (int i = 0; i < stats.layer_count_slicer; i++) {
            char start_str[20], end_str[20], duration_str[20];
            format_time(cumulative_time, start_str);
            cumulative_time += stats.layer_times[i];
            format_time(cumulative_time, end_str);
            format_time(stats.layer_times[i], duration_str);
            printf("%05d  %s  %s  %s\n", i + 1, start_str, end_str, duration_str);
        }
    } else if (output_layertimes) {
        printf("\nLayer times require explicit layer changes\n");
    }

    printf("\nConfiguration: \n");
    printf("  Acceleration: %.2f mm/s^2\n", config.acceleration);
    printf("  Z-axis acceleration: %.2f mm/s^2\n", config.z_acceleration);
    printf("  Extruder acceleration: %.2f mm/s^2\n", config.extruder_acceleration);
    printf("  Max XY speed: %.2f mm/s\n", config.max_speed_xy);
    printf("  Max travel speed: %.2f mm/s\n", config.max_speed_travel);
    printf("  Max Z speed: %.2f mm/s\n", config.max_speed_z);
    printf("  Default feedrate: %.2f mm/min\n", config.default_feedrate);
    printf("  Filament diameter: %.2f mm\n", config.filament_diameter);
    printf("  Filament density: %.2f g/cm^3\n", config.filament_density);

    return 0;
}
