/*
 * G-code Analysis
 */

#ifndef BERYLLIUM_H
#define BERYLLIUM_H

#include <src/globals.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <src/config/config.h>  // For buffer and configuration constants

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
    double layer_height;  // Average layer height in mm
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
BerylliumConfig beryllium_create_config(void);

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
 * Format numbers with thousands separators for better readability
 * @param value Number to format
 * @param decimals Number of decimal places (-1 for integer)
 * @return Static buffer containing formatted string
 */
char* format_number_with_separators(double value, int decimals);

/**
 * Format and display analysis results with improved readability
 * @param stats Pointer to BerylliumStats structure containing analysis results
 */
void beryllium_format_stats(const BerylliumStats *stats);

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

// Function prototypes
char* parse_parameter_string(const char *line, const char *parameter);
char* parse_name_parameter(const char *line);

// Additional function prototypes for extracted helper functions
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

#endif // BERYLLIUM_H
