/*
 * Print Queue configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c, with all necessary variables already in scope.
 */

json_t* print_queue = json_object_get(root, "PrintQueue");
if (json_is_object(print_queue)) {
    log_config_section_header("PrintQueue");
    
    json_t* enabled = json_object_get(print_queue, "Enabled");
    config->print_queue.enabled = get_config_bool(enabled, 1);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
             config->print_queue.enabled ? "true" : "false");

    json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
    if (json_is_object(queue_settings)) {
        log_config_section_item("QueueSettings", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(queue_settings, "DefaultPriority");
        config->print_queue.priorities.default_priority = get_config_int(val, 1);
        log_config_section_item("DefaultPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.default_priority);
        
        val = json_object_get(queue_settings, "EmergencyPriority");
        config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
        log_config_section_item("EmergencyPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.emergency_priority);
        
        val = json_object_get(queue_settings, "MaintenancePriority");
        config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
        log_config_section_item("MaintenancePriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.maintenance_priority);
        
        val = json_object_get(queue_settings, "SystemPriority");
        config->print_queue.priorities.system_priority = get_config_int(val, 3);
        log_config_section_item("SystemPriority", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->print_queue.priorities.system_priority);
    }

    json_t* timeouts = json_object_get(print_queue, "Timeouts");
    if (json_is_object(timeouts)) {
        log_config_section_item("Timeouts", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(timeouts, "ShutdownWaitMs");
        config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
        log_config_section_item("ShutdownDelay", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms",
               config->print_queue.timeouts.shutdown_wait_ms);
        
        val = json_object_get(timeouts, "JobProcessingTimeoutMs");
        config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
        log_config_section_item("JobProcessingTimeout", "%zu", LOG_LEVEL_STATE, !val, 1, "ms", "ms", config->print_queue.timeouts.job_processing_timeout_ms);
    }

    json_t* buffers = json_object_get(print_queue, "Buffers");
    if (json_is_object(buffers)) {
        log_config_section_item("Buffers", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(buffers, "JobMessageSize");
        config->print_queue.buffers.job_message_size = get_config_size(val, 256);
        log_config_section_item("JobMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->print_queue.buffers.job_message_size);
        
        val = json_object_get(buffers, "StatusMessageSize");
        config->print_queue.buffers.status_message_size = get_config_size(val, 256);
        log_config_section_item("StatusMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->print_queue.buffers.status_message_size);
    }
} else {
    config->print_queue.enabled = 1;
    config->print_queue.priorities.default_priority = 1;
    config->print_queue.priorities.emergency_priority = 0;
    config->print_queue.priorities.maintenance_priority = 2;
    config->print_queue.priorities.system_priority = 3;
    config->print_queue.timeouts.shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
    config->print_queue.timeouts.job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
    config->print_queue.buffers.job_message_size = 256;
    config->print_queue.buffers.status_message_size = 256;
    
    log_config_section_header("PrintQueue");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}