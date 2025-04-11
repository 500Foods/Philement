/*
 * Print Queue configuration JSON parsing
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "json_print_queue.h"
#include "../config.h"
#include "../config_utils.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../types/config_size.h"
#include "../print/config_print_queue.h"
#include "../../logging/logging.h"

bool load_json_print_queue(json_t* root, AppConfig* config) {
    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    bool using_defaults = !json_is_object(print_queue);
    
    log_config_section("PrintQueue", using_defaults);
    
    if (!using_defaults) {
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = get_config_bool(enabled, 1);
        log_config_item("Enabled", config->print_queue.enabled ? "true" : "false", !enabled, 0);

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            log_config_item("QueueSettings", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = get_config_int(val, 1);
            log_config_item("DefaultPriority", format_int_buffer(config->print_queue.priorities.default_priority), !val, 1);
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
            log_config_item("EmergencyPriority", format_int_buffer(config->print_queue.priorities.emergency_priority), !val, 1);
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
            log_config_item("MaintenancePriority", format_int_buffer(config->print_queue.priorities.maintenance_priority), !val, 1);
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = get_config_int(val, 3);
            log_config_item("SystemPriority", format_int_buffer(config->print_queue.priorities.system_priority), !val, 1);
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            log_config_item("Timeouts", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
            char shutdown_buffer[64];
            snprintf(shutdown_buffer, sizeof(shutdown_buffer), "%sms", 
                    format_int_buffer(config->print_queue.timeouts.shutdown_wait_ms));
            log_config_item("ShutdownDelay", shutdown_buffer, !val, 1);
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
            char timeout_buffer[64];
            snprintf(timeout_buffer, sizeof(timeout_buffer), "%sms", 
                    format_int_buffer(config->print_queue.timeouts.job_processing_timeout_ms));
            log_config_item("JobProcessingTimeout", timeout_buffer, !val, 1);
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            log_config_item("Buffers", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = get_config_size(val, 256);
            char job_buffer[64];
            snprintf(job_buffer, sizeof(job_buffer), "%sMB", 
                    format_int_buffer(config->print_queue.buffers.job_message_size / (1024 * 1024)));
            log_config_item("JobMessageSize", job_buffer, !val, 1);
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = get_config_size(val, 256);
            char status_buffer[64];
            snprintf(status_buffer, sizeof(status_buffer), "%sMB", 
                    format_int_buffer(config->print_queue.buffers.status_message_size / (1024 * 1024)));
            log_config_item("StatusMessageSize", status_buffer, !val, 1);
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
        
        log_config_item("Status", "Section missing, using defaults", true, 0);
    }
    
    return true;
}