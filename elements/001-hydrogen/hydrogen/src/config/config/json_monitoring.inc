/*
 * System Monitoring configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c, with all necessary variables already in scope.
 */

json_t* monitoring = json_object_get(root, "SystemMonitoring");
if (json_is_object(monitoring)) {
    log_config_section_header("SystemMonitoring");
    
    json_t* intervals = json_object_get(monitoring, "Intervals");
    if (json_is_object(intervals)) {
        log_config_section_item("Intervals", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(intervals, "StatusUpdateMs");
        config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
        log_config_section_item("StatusUpdateFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.status_update_ms);
        
        val = json_object_get(intervals, "ResourceCheckMs");
        config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
        log_config_section_item("ResourceCheckFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.resource_check_ms);
        
        val = json_object_get(intervals, "MetricsUpdateMs");
        config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
        log_config_section_item("MetricsUpdateFreq", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->monitoring.metrics_update_ms);
    }

    json_t* thresholds = json_object_get(monitoring, "Thresholds");
    if (json_is_object(thresholds)) {
        log_config_section_item("Thresholds", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(thresholds, "MemoryWarningPercent");
        config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
        log_config_section_item("MemoryWarningPercent", "%d%%", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.memory_warning_percent);
        
        val = json_object_get(thresholds, "DiskSpaceWarningPercent");
        config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
        log_config_section_item("DiskSpaceWarningPercent", "%d%%", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.disk_warning_percent);
        
        val = json_object_get(thresholds, "LoadAverageWarning");
        config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
        log_config_section_item("LoadAverageWarning", "%.1f", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->monitoring.load_warning);
    }
} else {
    config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
    config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
    config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
    config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
    config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
    config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
    
    log_config_section_header("SystemMonitoring");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}