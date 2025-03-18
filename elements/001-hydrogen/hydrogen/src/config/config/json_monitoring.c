/*
 * System Monitoring configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"
#include "../config_utils.h"
#include "../monitor/config_monitoring.h"
#include "json_monitoring.h"

bool load_json_monitoring(json_t* root, AppConfig* config) {
    // System Monitoring Configuration
    json_t* monitoring = json_object_get(root, "SystemMonitoring");
    if (json_is_object(monitoring)) {
        log_config_section_header("SystemMonitoring");
        
        // Update intervals
        json_t* intervals = json_object_get(monitoring, "UpdateIntervals");
        if (json_is_object(intervals)) {
            log_config_section_item("UpdateIntervals", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(intervals, "StatusUpdateMS");
            config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
            log_config_section_item("StatusUpdateMS", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", "Config", config->monitoring.status_update_ms);
            
            val = json_object_get(intervals, "ResourceCheckMS");
            config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
            log_config_section_item("ResourceCheckMS", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", "Config", config->monitoring.resource_check_ms);
            
            val = json_object_get(intervals, "MetricsUpdateMS");
            config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
            log_config_section_item("MetricsUpdateMS", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", "Config", config->monitoring.metrics_update_ms);
        }
        
        // Warning thresholds
        json_t* thresholds = json_object_get(monitoring, "WarningThresholds");
        if (json_is_object(thresholds)) {
            log_config_section_item("WarningThresholds", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
            log_config_section_item("MemoryWarningPercent", "%d", LOG_LEVEL_STATE, !val, 1, "%", "%", "Config", config->monitoring.memory_warning_percent);
            
            val = json_object_get(thresholds, "DiskWarningPercent");
            config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
            log_config_section_item("DiskWarningPercent", "%d", LOG_LEVEL_STATE, !val, 1, "%", "%", "Config", config->monitoring.disk_warning_percent);
            
            val = json_object_get(thresholds, "LoadWarning");
            config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
            log_config_section_item("LoadWarning", "%.1f", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->monitoring.load_warning);
        }
        
        // Validate configuration
        if (config_monitoring_validate(&config->monitoring) != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            config_monitoring_init(&config->monitoring);
        }
        
        return true;
    } else {
        // Set defaults if no monitoring configuration is provided
        config_monitoring_init(&config->monitoring);
        
        log_config_section_header("SystemMonitoring");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        return true;
    }
}