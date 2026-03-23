/*
 * WebSocket Media Upload Handler Implementation
 *
 * Handles media upload messages via WebSocket protocol.
 * Supports single-message and chunked uploads.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/utils/utils_crypto.h>

// Local includes
#include "websocket_server_media.h"
#include "websocket_server_internal.h"
#include "websocket_server.h"
#include "websocket_server_message.h"

// Media storage includes
#include "../api/conduit/chat_common/chat_storage.h"
#include "../api/conduit/chat_common/chat_engine_cache.h"

// Queue for thread-safe chunk passing (if needed)
#include <src/queue/queue.h>

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Logging source for WebSocket media
static const char* SR_WEBSOCKET_MEDIA = "WEBSOCKET_MEDIA";

// Forward declarations
static void send_media_upload_error(struct lws *wsi, const char* error_message, const char* request_id);
static void send_media_upload_success(struct lws *wsi, const char* request_id, const char* media_hash, 
                                      size_t media_size, const char* mime_type);

int handle_media_upload_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json) {
    if (!wsi || !request_json) {
        return -1;
    }
    log_this(SR_WEBSOCKET_MEDIA, "Handling media upload request", LOG_LEVEL_STATE, 0);
    
    // Extract request ID (optional)
    const char* request_id = NULL;
    json_t* id_json = json_object_get(request_json, "id");
    if (id_json && json_is_string(id_json)) {
        request_id = json_string_value(id_json);
    }
    
    // Extract payload
    json_t* payload = json_object_get(request_json, "payload");
    if (!payload || !json_is_object(payload)) {
        send_media_upload_error(wsi, "Missing or invalid payload", request_id);
        json_decref(request_json);
        return -1;
    }
    
    // JWT authentication - check if already authenticated for chat
    if (!session->chat_database) {
        // Need JWT from payload
        json_t* jwt_json = json_object_get(payload, "jwt");
        if (!jwt_json || !json_is_string(jwt_json)) {
            send_media_upload_error(wsi, "JWT required for media upload", request_id);
            json_decref(request_json);
            return -1;
        }
        
        const char* jwt = json_string_value(jwt_json);
        jwt_validation_result_t jwt_result = {0};
        if (!extract_and_validate_jwt(jwt, &jwt_result)) {
            send_media_upload_error(wsi, "Invalid JWT", request_id);
            json_decref(request_json);
            return -1;
        }
        
        // Extract database from claims
        if (!jwt_result.claims || !jwt_result.claims->database) {
            send_media_upload_error(wsi, "JWT missing database claim", request_id);
            free_jwt_validation_result(&jwt_result);
            json_decref(request_json);
            return -1;
        }
        
        // Store in session for future messages
        session->chat_database = strdup(jwt_result.claims->database);
        session->chat_claims = jwt_result.claims; // ownership transferred
        free_jwt_validation_result(&jwt_result);
    }
    
    const char* database = session->chat_database;
    
    // Extract media data (base64 encoded)
    json_t* data_json = json_object_get(payload, "data");
    if (!data_json || !json_is_string(data_json)) {
        send_media_upload_error(wsi, "Missing or invalid 'data' field", request_id);
        json_decref(request_json);
        return -1;
    }
    const char* base64_data = json_string_value(data_json);
    
    // Extract MIME type
    json_t* mime_json = json_object_get(payload, "mime_type");
    const char* mime_type = NULL;
    if (mime_json && json_is_string(mime_json)) {
        mime_type = json_string_value(mime_json);
    }
    
    // Decode base64url data
    size_t decoded_length = 0;
    unsigned char* decoded_data = utils_base64url_decode(base64_data, &decoded_length);
    if (!decoded_data) {
        send_media_upload_error(wsi, "Invalid base64 data", request_id);
        json_decref(request_json);
        return -1;
    }
    
    // Compute SHA-256 hash
    char* media_hash = utils_sha256_hash(decoded_data, decoded_length);
    if (!media_hash) {
        free(decoded_data);
        send_media_upload_error(wsi, "Failed to compute hash", request_id);
        json_decref(request_json);
        return -1;
    }
    
    // Store in media_assets table via chat_storage
    bool stored = chat_storage_store_media(database, media_hash, decoded_data, decoded_length, mime_type);
    
    if (!stored) {
        free(media_hash);
        free(decoded_data);
        send_media_upload_error(wsi, "Failed to store media", request_id);
        json_decref(request_json);
        return -1;
    }
    
    // Send success response
    send_media_upload_success(wsi, request_id, media_hash, decoded_length, mime_type);
    
    free(media_hash);
    free(decoded_data);
    json_decref(request_json);
    return 0;
}

int handle_media_chunk_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json) {
    // TODO: Implement chunked upload support
    (void)wsi;
    (void)session;
    (void)request_json;
    return -1;
}

int media_subsystem_init(void) {
    // Nothing to initialize for now
    return 0;
}

void media_subsystem_cleanup(void) {
    // Nothing to cleanup
}

void media_session_cleanup(WebSocketSessionData *session) {
    // Nothing to clean up for media-specific session data
    (void)session;
}

// Helper functions
static void send_media_upload_error(struct lws *wsi, const char* error_message, const char* request_id) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("media_upload_error"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    json_object_set_new(response, "error", json_string(error_message ? error_message : "Unknown error"));
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}

static void send_media_upload_success(struct lws *wsi, const char* request_id, const char* media_hash, 
                                      size_t media_size, const char* mime_type) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("media_upload_success"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    json_t* data = json_object();
    json_object_set_new(data, "media_hash", json_string(media_hash));
    json_object_set_new(data, "media_size", json_integer((json_int_t)media_size));
    if (mime_type) {
        json_object_set_new(data, "mime_type", json_string(mime_type));
    }
    json_object_set_new(response, "data", data);
    
    ws_write_json_response(wsi, response);
    json_decref(response);
}

