/*
 * Reporting image_scale Endpoint
 *
 * POST /api/reporting/image_scale — scale and convert a base64-encoded image.
 */

#ifndef REPORTING_IMAGE_SCALE_H
#define REPORTING_IMAGE_SCALE_H

#include <microhttpd.h>

//@ swagger:path /api/reporting/image_scale
//@ swagger:method POST
//@ swagger:operationId imageScale
//@ swagger:tags "Reporting Service"
//@ swagger:summary Scale and convert an image
//@ swagger:description Scales and converts a base64-encoded image to a specified format and dimensions. Accepts optional data: URI prefix on the image field. Supports px/pt units, DPI conversion, and XO (PDF XObject) output.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["image","format","width","height"],"properties":{"image":{"type":"string","description":"Base64-encoded image, optional data: URI prefix"},"format":{"type":"string","description":"Output format (jpg,png,bmp,svg,ico,webp,xo,...)"},"width":{"type":"integer","description":"Output width in units"},"height":{"type":"integer","description":"Output height in units"},"units":{"type":"string","enum":["px","pt"],"default":"px"},"dpi":{"type":"integer","default":72},"scale_algorithm":{"type":"string","enum":["nearest","bilinear","bicubic","lanczos","mitchell"],"default":"lanczos"}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"success":{"type":"boolean"},"image":{"type":"string"},"format":{"type":"string"},"width":{"type":"integer"},"height":{"type":"integer"},"units":{"type":"string"},"mime_type":{"type":"string"},"input_format":{"type":"string"},"input_dimensions":{"type":"object"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean"},"error":{"type":"string"},"details":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean"},"error":{"type":"string"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"success":{"type":"boolean"},"error":{"type":"string"},"details":{"type":"string"}}}
enum MHD_Result handle_reporting_image_scale_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

// Error JSON helper (non-static for Unity / no-static policy).
enum MHD_Result image_scale_send_error(struct MHD_Connection *connection,
                                       const char *error,
                                       const char *details,
                                       unsigned int http_status);

#endif /* REPORTING_IMAGE_SCALE_H */
