/*
 * System Resources configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_resources.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// System Resources Configuration
json_t* resources = json_object_get(root, "SystemResources");
if (json_is_object(resources)) {
    log_config_section_header("SystemResources");
    
    json_t* queues = json_object_get(resources, "Queues");
    if (json_is_object(queues)) {
        log_config_section_item("Queues", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(queues, "MaxQueueBlocks");
        config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
        log_config_section_item("MaxQueueBlocks", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.max_queue_blocks);
        
        val = json_object_get(queues, "QueueHashSize");
        config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
        log_config_section_item("QueueHashSize", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.queue_hash_size);
        
        val = json_object_get(queues, "DefaultQueueCapacity");
        config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
        log_config_section_item("DefaultQueueCapacity", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, config->resources.default_capacity);
    }

    json_t* buffers = json_object_get(resources, "Buffers");
    if (json_is_object(buffers)) {
        log_config_section_item("Buffers", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* val;
        val = json_object_get(buffers, "DefaultMessageBuffer");
        config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
        log_config_section_item("DefaultMessageBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.message_buffer_size);
        
        val = json_object_get(buffers, "MaxLogMessageSize");
        config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
        log_config_section_item("MaxLogMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.max_log_message_size);
        
        val = json_object_get(buffers, "LineBufferSize");
        config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);
        log_config_section_item("LineBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.line_buffer_size);

        val = json_object_get(buffers, "PostProcessorBuffer");
        config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);
        log_config_section_item("PostProcessorBuffer", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.post_processor_buffer_size);

        val = json_object_get(buffers, "LogBufferSize");
        config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);
        log_config_section_item("LogBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.log_buffer_size);

        val = json_object_get(buffers, "JsonMessageSize");
        config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);
        log_config_section_item("JsonMessageSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.json_message_size);

        val = json_object_get(buffers, "LogEntrySize");
        config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
        log_config_section_item("LogEntrySize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "MB", config->resources.log_entry_size);
    }
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
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}