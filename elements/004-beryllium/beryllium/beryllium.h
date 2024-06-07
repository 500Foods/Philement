// beryllium.h
#ifndef BERYLLIUM_H
#define BERYLLIUM_H

#include <stdbool.h>

#define MAX_LINE_LENGTH 1024
#define FILAMENT_DENSITY 1.05  // Filament density in g/cm^3
#define FILAMENT_DIAMETER 1.75  // Filament diameter in mm
#define MAX_SPEED_XY 5000.0  // Maximum speed for printing moves (mm/s)
#define MAX_SPEED_TRAVEL 5000.0  // Maximum speed for non-printing moves (mm/s)
#define MAX_SPEED_Z 10.0   // Maximum speed in Z-axis (mm/s)

typedef struct {
    long file_size;
    int total_lines;
    int gcode_lines;
    int layer_count;
    double print_time;
    double extrusion;
    double filament_volume;
    double filament_weight;
} BerylliumStats;

char* get_iso8601_timestamp(void);
void format_time(double seconds, char *buffer);
BerylliumStats beryllium_analyze_gcode(FILE *file);

#endif // BERYLLIUM_H
