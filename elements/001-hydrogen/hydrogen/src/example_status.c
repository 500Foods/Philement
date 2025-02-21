// Example showing both explicit and implicit percentage formatting patterns

#include <stdio.h>
#include <jansson.h>

// Format all percentage values as strings with 3 decimal places for consistency
// with system status reporting. This ensures uniform presentation of percentage
// values throughout the API responses.
json_t* create_percentage_status(void) {
    json_t *status = json_object();
    
    // Memory percentages (following utils_status.c pattern)
    char memory_percent_str[16];
    double memory_percent = 45.6789;
    snprintf(memory_percent_str, sizeof(memory_percent_str), "%.3f", memory_percent);
    json_object_set_new(status, "memory_percent", json_string(memory_percent_str));
    
    // CPU percentages (following utils_status.c pattern)
    char cpu_percent_str[16];
    double cpu_percent = 78.1234;
    snprintf(cpu_percent_str, sizeof(cpu_percent_str), "%.3f", cpu_percent);
    json_object_set_new(status, "cpu_percent", json_string(cpu_percent_str));
    
    return status;
}