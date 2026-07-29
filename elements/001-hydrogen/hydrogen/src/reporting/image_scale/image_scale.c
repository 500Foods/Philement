/*
 * Reporting image_scale Endpoint Implementation
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop

#include <src/reporting/reporting_service.h>
#include <src/reporting/helpers/base64_utils.h>
#include <src/reporting/helpers/image_format.h>
#include <src/reporting/helpers/image_xo.h>
#include <src/reporting/helpers/image_scale_core.h>

#include "image_scale.h"

enum MHD_Result image_scale_send_error(struct MHD_Connection *connection,
                                       const char *error,
                                       const char *details,
                                       unsigned int http_status) {
    json_t *response = json_object();
    if (!response) {
        return api_send_error_and_cleanup(connection, NULL, "Failed to build error response", http_status);
    }
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error ? error : "Error"));
    if (details) {
        json_object_set_new(response, "details", json_string(details));
    }
    enum MHD_Result result = api_send_json_response(connection, response, http_status);
    json_decref(response);
    return result;
}

enum MHD_Result handle_reporting_image_scale_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {
    (void)url;

    if (!connection || !method || !upload_data_size || !con_cls) {
        return MHD_NO;
    }

    if (!app_config || !app_config->reporting.Enabled) {
        return image_scale_send_error(connection, "Reporting disabled",
                                      "Reporting subsystem is not enabled",
                                      MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    if (strcmp(method, "POST") != 0) {
        return image_scale_send_error(connection, "Method not allowed",
                                      "Only POST is supported",
                                      MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data,
                                                      upload_data_size, con_cls, &buffer);

    if (buf_result == API_BUFFER_ERROR) {
        return MHD_YES;
    }
    if (buf_result == API_BUFFER_METHOD_ERROR) {
        api_free_post_buffer(con_cls);
        return image_scale_send_error(connection, "Method not allowed",
                                      "Only POST is supported",
                                      MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    if (buf_result == API_BUFFER_CONTINUE) {
        return MHD_YES;
    }

    json_t *request_json = api_parse_json_body(buffer);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        return image_scale_send_error(connection, "Invalid JSON",
                                      "Request body contains invalid JSON",
                                      MHD_HTTP_BAD_REQUEST);
    }

    json_t *j_image = json_object_get(request_json, "image");
    json_t *j_format = json_object_get(request_json, "format");
    json_t *j_width = json_object_get(request_json, "width");
    json_t *j_height = json_object_get(request_json, "height");
    json_t *j_units = json_object_get(request_json, "units");
    json_t *j_dpi = json_object_get(request_json, "dpi");
    json_t *j_algo = json_object_get(request_json, "scale_algorithm");

    if (!json_is_string(j_image)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Missing parameter",
                                      "Field 'image' is required and must be a string",
                                      MHD_HTTP_BAD_REQUEST);
    }
    if (!json_is_string(j_format)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Missing parameter",
                                      "Field 'format' is required and must be a string",
                                      MHD_HTTP_BAD_REQUEST);
    }
    if (!json_is_integer(j_width) || !json_is_integer(j_height)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Missing parameter",
                                      "Fields 'width' and 'height' are required integers",
                                      MHD_HTTP_BAD_REQUEST);
    }

    const char *image_b64 = json_string_value(j_image);
    const char *format = json_string_value(j_format);
    int width = (int)json_integer_value(j_width);
    int height = (int)json_integer_value(j_height);
    const char *units = json_is_string(j_units) ? json_string_value(j_units) : "px";
    int dpi = json_is_integer(j_dpi) ? (int)json_integer_value(j_dpi)
                                     : app_config->reporting.DefaultDPI;
    if (dpi <= 0) {
        dpi = 72;
    }
    const char *algorithm = json_is_string(j_algo) ? json_string_value(j_algo) : "lanczos";

    // Size limits on base64 input length (413 Payload Too Large)
    size_t input_b64_len = strlen(image_b64);
    if (app_config->reporting.MaxInputBytes > 0 &&
        input_b64_len > (size_t)app_config->reporting.MaxInputBytes) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Input too large",
                                      "Base64 image exceeds MaxInputBytes",
                                      MHD_HTTP_CONTENT_TOO_LARGE);
    }

    if (!format_is_supported(format, app_config->reporting.AllowedFormats)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Unsupported format",
                                      "Requested format is not allowed",
                                      MHD_HTTP_BAD_REQUEST);
    }

    int px_w = 0;
    int px_h = 0;
    if (!parse_dimensions(width, height, units, dpi, &px_w, &px_h)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Invalid dimensions",
                                      "Could not convert width/height with given units",
                                      MHD_HTTP_BAD_REQUEST);
    }

    if (app_config->reporting.MaxImageSize > 0 &&
        (px_w > app_config->reporting.MaxImageSize ||
         px_h > app_config->reporting.MaxImageSize)) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Dimensions too large",
                                      "Requested dimensions exceed MaxImageSize",
                                      MHD_HTTP_BAD_REQUEST);
    }

    size_t blob_len = 0;
    unsigned char *blob = reporting_base64_decode(image_b64, &blob_len);
    if (!blob) {
        json_decref(request_json);
        return image_scale_send_error(connection, "Invalid image data",
                                      "Base64 decode failed",
                                      MHD_HTTP_BAD_REQUEST);
    }

    // Decoded binary size must also respect MaxInputBytes
    if (app_config->reporting.MaxInputBytes > 0 &&
        blob_len > (size_t)app_config->reporting.MaxInputBytes) {
        free(blob);
        json_decref(request_json);
        return image_scale_send_error(connection, "Input too large",
                                      "Decoded image exceeds MaxInputBytes",
                                      MHD_HTTP_CONTENT_TOO_LARGE);
    }

    char *core_err = NULL;
    size_t in_w = 0;
    size_t in_h = 0;
    char *input_format = NULL;
    MagickWand *wand = scale_image_core(blob, blob_len, width, height, units, dpi,
                                        algorithm, format, &core_err,
                                        &in_w, &in_h, &input_format);
    free(blob);

    if (!wand) {
        const char *details = core_err ? core_err : "Image processing failed";
        enum MHD_Result r = image_scale_send_error(connection, "Image processing failed",
                                                   details, MHD_HTTP_BAD_REQUEST);
        free(core_err);
        free(input_format);
        json_decref(request_json);
        return r;
    }
    free(core_err);

    char *out_b64 = NULL;
    const char *mime = format_to_mime(format);

    if (format_is_xo(format)) {
        size_t xo_b64_len = 0;
        out_b64 = encode_xo_stream(wand, &xo_b64_len);
        (void)xo_b64_len;
    } else {
        size_t out_blob_len = 0;
        unsigned char *out_blob = MagickGetImageBlob(wand, &out_blob_len);
        if (!out_blob || out_blob_len == 0) {
            char *ex = scale_image_core_exception(wand);
            enum MHD_Result r = image_scale_send_error(connection, "Encode failed",
                                                       ex ? ex : "MagickGetImageBlob failed",
                                                       MHD_HTTP_INTERNAL_SERVER_ERROR);
            free(ex);
            free(input_format);
            DestroyMagickWand(wand);
            json_decref(request_json);
            return r;
        }

        out_b64 = reporting_base64_encode(out_blob, out_blob_len);
        MagickRelinquishMemory(out_blob);
    }

    if (!out_b64) {
        free(input_format);
        DestroyMagickWand(wand);
        json_decref(request_json);
        return image_scale_send_error(connection, "Encode failed",
                                      "Failed to encode output image",
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // MaxOutputBytes limits base64 payload size (matches MaxInputBytes semantics)
    if (app_config->reporting.MaxOutputBytes > 0 &&
        strlen(out_b64) > (size_t)app_config->reporting.MaxOutputBytes) {
        free(out_b64);
        free(input_format);
        DestroyMagickWand(wand);
        json_decref(request_json);
        return image_scale_send_error(connection, "Output too large",
                                      "Encoded image exceeds MaxOutputBytes",
                                      MHD_HTTP_CONTENT_TOO_LARGE);
    }

    json_t *response = json_object();
    if (!response) {
        free(out_b64);
        free(input_format);
        DestroyMagickWand(wand);
        json_decref(request_json);
        return image_scale_send_error(connection, "Internal error",
                                      "Failed to build response",
                                      MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "format", json_string(format));
    json_object_set_new(response, "width", json_integer(px_w));
    json_object_set_new(response, "height", json_integer(px_h));
    json_object_set_new(response, "units", json_string("px"));
    json_object_set_new(response, "image", json_string(out_b64));
    json_object_set_new(response, "mime_type", json_string(mime ? mime : "application/octet-stream"));

    if (input_format && input_format[0] != '\0') {
        char in_mime[64];
        snprintf(in_mime, sizeof(in_mime), "image/%s", input_format);
        for (char *p = in_mime; *p; p++) {
            if (*p >= 'A' && *p <= 'Z') {
                *p = (char)(*p - 'A' + 'a');
            }
        }
        json_object_set_new(response, "input_format", json_string(in_mime));
    }

    json_t *dims = json_object();
    if (dims) {
        json_object_set_new(dims, "width", json_integer((json_int_t)in_w));
        json_object_set_new(dims, "height", json_integer((json_int_t)in_h));
        json_object_set_new(response, "input_dimensions", dims);
    }

    free(out_b64);
    free(input_format);
    DestroyMagickWand(wand);
    json_decref(request_json);

    enum MHD_Result result = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);
    return result;
}
