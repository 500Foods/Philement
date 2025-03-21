/*
 * System Resources configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"
#include "../config_utils.h"
#include "json_resources.h"

bool load_json_resources(json_t* root, AppConfig* config) {
    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        log_config_section_header("SystemResources");
        
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_section_item("Queues", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(queues, "MaxQueueBlocks");
            config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
            log_config_section_item("MaxQueueBlocks", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->resources.max_queue_blocks);
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
            log_config_section_item("QueueHashSize", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->resources.queue_hash_size);
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
            log_config_section_item("DefaultQueueCapacity", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->resources.default_capacity);
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            log_config_section_item("Buffers", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
            log_config_section_item("DefaultMessageBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.message_buffer_size);
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
            log_config_section_item("MaxLogMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.max_log_message_size);
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);
            log_config_section_item("LineBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.line_buffer_size);

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);
            log_config_section_item("PostProcessorBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.post_processor_buffer_size);

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);
            log_config_section_item("LogBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.log_buffer_size);

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);
            log_config_section_item("JsonMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.json_message_size);

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
            log_config_section_item("LogEntrySize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", "Config", config->resources.log_entry_size);
        }
        return true;
    } else {
        config->resources.max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
        config->resources.queue_hash_size = DEFAULT_QUEUE_HASH_SIZE;
        config->resources.default_capacity = DEFAULT_QUEUE_CAPACITY;
        config->resources.message_buffer_size = DEFAULT_MESSAGE_BUFFER_SIZE;
        config->resources.max_log_message_size = DEFAULT_MAX_LOG_MESSAGE_SIZE;
        config->resources.line_buffer_size = DEFAULT_LINE_BUFFER_SIZE;
        config->resources.post_processor_buffer_size = DEFAULT_POST_PROCESSOR_BUFFER_SIZE;
        config->resources.log_buffer_size = DEFAULT_LOG_BUFFER_SIZE;
        config->resources.json_message_size = DEFAULT_JSON_MESSAGE_SIZE;
        config->resources.log_entry_size = DEFAULT_LOG_ENTRY_SIZE;
        
        log_config_section_header("SystemResources");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config", NULL);
        return true;
    }
}