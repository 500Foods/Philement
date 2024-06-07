// beryllium_analyze.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "beryllium.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return 1;
    }

    char *start_time = get_iso8601_timestamp();
    printf("Parsing start: %s\n", start_time);

    clock_t start = clock();
    BerylliumStats stats = beryllium_analyze_gcode(file);
    clock_t end = clock();

    fclose(file);

    char print_time_str[20];
    format_time(stats.print_time, print_time_str);

    printf("File size: %ld bytes\n", stats.file_size);
    printf("Total lines: %d\n", stats.total_lines);
    printf("G-code lines: %d\n", stats.gcode_lines);
    printf("Number of layers: %d\n", stats.layer_count);
    printf("Estimated print time: %s\n", print_time_str);
    printf("Filament used: %.2f mm (%.2f cm^3)\n", stats.extrusion, stats.filament_volume);
    printf("Filament weight: %.2f grams\n", stats.filament_weight);

    char *end_time = get_iso8601_timestamp();
    printf("Parsing end: %s\n", end_time);

    double elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    printf("Parsing duration: %.6f ms\n", elapsed_time);

    return 0;
}
