/*
 * G-code Analysis for High-Quality 3D Printing
 * 
 * Why Advanced Analysis Matters:
 * 1. Print Quality
 *    - Motion path validation
 *    - Acceleration profiles
 *    - Layer timing analysis
 *    - Material flow rates
 * 
 * 2. Resource Management
 *    Why These Calculations?
 *    - Material consumption
 *    - Print time prediction
 *    - Power usage estimation
 *    - Cost calculation
 * 
 * 3. Time Estimation
 *    Why So Precise?
 *    - Scheduling accuracy
 *    - Resource planning
 *    - Progress tracking
 *    - Maintenance timing
 * 
 * 4. Multi-Object Printing
 *    Why This Support?
 *    - Object separation
 *    - Print sequencing
 *    - Failure isolation
 *    - Resource allocation
 * 
 * 5. Safety Features
 *    Why These Checks?
 *    - Motion limit validation
 *    - Temperature profiling
 *    - Tool path verification
 *    - Resource bounds
 */

#ifndef BERYLLIUM_H
#define BERYLLIUM_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../config/configuration.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Max length of a G-Code Line - using system default
#define MAX_LINE_LENGTH DEFAULT_LINE_BUFFER_SIZE

// Maximum number of layers - using system default
#define MAX_LAYERS DEFAULT_MAX_LAYERS

// Default feedrate in mm/min (not configurable as it's a G-code standard)
#define DEFAULT_FEEDRATE   7500.0

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

// Object information
typedef struct {
    char *name;
    int index;
} ObjectInfo;

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
    double **object_times;
    ObjectInfo *object_infos;
    int num_objects;
    bool success;  // Indicates if the analysis completed successfully
} BerylliumStats;

// Function Prototypes

/**
 * Create a BerylliumConfig from AppConfig
 * @param app_config The application configuration
 * @return BerylliumConfig initialized with values from app_config
 */
BerylliumConfig beryllium_create_config(const AppConfig *app_config);

/**
 * Get current timestamp in ISO8601 format
 * @return Static buffer containing timestamp string
 */
char* get_iso8601_timestamp(void);

/**
 * Format time duration into days:hours:minutes:seconds
 * @param seconds Time duration in seconds
 * @param buffer Buffer to store formatted string
 */
void format_time(double seconds, char *buffer);

/**
 * Analyze G-code file and return statistics
 * @param file File pointer to G-code file
 * @param config Printer configuration
 * @return BerylliumStats structure containing analysis results
 * 
 * Note: The returned structure contains dynamically allocated memory.
 * Caller must call beryllium_free_stats() to free this memory when done.
 */
BerylliumStats beryllium_analyze_gcode(FILE *file, const BerylliumConfig *config);

/**
 * Free memory allocated in BerylliumStats structure
 * @param stats Pointer to stats structure to clean up
 * 
 * This function frees all dynamically allocated memory in the stats structure:
 * - object_times array and all its layer arrays
 * - object_infos array and all object names
 * After calling this, the stats structure should not be used again.
 */
void beryllium_free_stats(BerylliumStats *stats);

#endif // BERYLLIUM_H

