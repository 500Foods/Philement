// beryllium.h
#ifndef BERYLLIUM_H
#define BERYLLIUM_H

#include <stdbool.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Max length of a G-Code Line
#define MAX_LINE_LENGTH 1024

// Maximum number of layers to output for timing function
#define MAX_LAYERS 10000

// Physical printer capabilities
#define ACCELERATION       1000.0  // Acceleration
#define Z_ACCELERATION     250.0   // Z Acceleration
#define E_ACCELERATION     2000.0  // E Acceleration
#define MAX_SPEED_XY       5000.0  // Maximum speed for printing moves (mm/s)
#define MAX_SPEED_TRAVEL   5000.0  // Maximum speed for non-printing moves (mm/s)
#define MAX_SPEED_Z        10.0    // Maximum speed in Z-axis (mm/s)
#define DEFAULT_FEEDRATE   7500.0  // Default feedrate in mm/min
				   
// Filament properties
#define DEFAULT_FILAMENT_DIAMETER 1.75  // mm
#define DEFAULT_FILAMENT_DENSITY 1.04   // g/cm^3 

// What we're sending in
typedef struct {
    double acceleration;
    double z_acceleration;
    double extruder_acceleration;
    double max_speed_xy;
    double max_speed_travel;
    double max_speed_z;
    double default_feedrate;
    double filament_diameter;
    double filament_density;
} BerylliumConfig;

// What we're expecting to get back
typedef struct {
    long file_size;
    int total_lines;
    int gcode_lines;
    int layer_count_height;
    int layer_count_slicer;
    double print_time;
    double extrusion;
    double filament_volume;
    double filament_weight;
    double layer_times[MAX_LAYERS];
} BerylliumStats;

// Function Prototypes
char* get_iso8601_timestamp(void);
void format_time(double seconds, char *buffer);
BerylliumStats beryllium_analyze_gcode(FILE *file, const BerylliumConfig *config);

#endif // BERYLLIUM_H
