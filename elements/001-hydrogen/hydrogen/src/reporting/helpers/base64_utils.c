/*
 * Reporting Base64 Utilities Implementation
 */

#include <src/hydrogen.h>
#include <src/utils/utils_crypto.h>

#include "base64_utils.h"

bool parse_data_uri(const char* input, const char** base64_start) {
    if (!input || !base64_start) {
        return false;
    }

    *base64_start = NULL;

    // data:[<mediatype>][;base64],<data>
    if (strncmp(input, "data:", 5) == 0) {
        const char* comma = strchr(input, ',');
        if (!comma || comma[1] == '\0') {
            return false;
        }
        // Require ;base64 before the comma (case-insensitive check on "base64")
        const char* semi = strstr(input, ";base64");
        if (!semi || semi > comma) {
            // Also accept ";BASE64" variants
            const char* p = input + 5;
            bool found_b64 = false;
            while (p < comma) {
                if ((p[0] == ';' || p[0] == ',') &&
                    (p + 7 <= comma) &&
                    (strncasecmp(p + 1, "base64", 6) == 0)) {
                    found_b64 = true;
                    break;
                }
                p++;
            }
            if (!found_b64) {
                // Treat payload after comma as raw base64 anyway (common clients)
                *base64_start = comma + 1;
                return true;
            }
        }
        *base64_start = comma + 1;
        return true;
    }

    *base64_start = input;
    return true;
}

unsigned char* reporting_base64_decode(const char* input, size_t* out_len) {
    if (!input || !out_len) {
        return NULL;
    }

    const char* b64 = NULL;
    if (!parse_data_uri(input, &b64) || !b64) {
        return NULL;
    }

    return utils_base64_decode(b64, out_len);
}

char* reporting_base64_encode(const unsigned char* data, size_t len) {
    return utils_base64_encode(data, len);
}
