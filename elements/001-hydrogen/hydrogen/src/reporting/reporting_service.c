/*
 * Reporting Service Implementation
 *
 * Owns MagickWand process-wide genesis/terminus lifecycle and
 * Magick resource limits to bound memory/time for image_scale.
 */

#include <src/hydrogen.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop

#include "reporting_service.h"

// Default Magick caps when config is unavailable or zero
#define REPORTING_DEFAULT_MEMORY_BYTES  ((MagickSizeType)512ULL * 1024ULL * 1024ULL)
#define REPORTING_DEFAULT_MAP_BYTES     ((MagickSizeType)512ULL * 1024ULL * 1024ULL)
#define REPORTING_DEFAULT_MAX_DIM       ((MagickSizeType)8192ULL)
#define REPORTING_DEFAULT_TIME_SECONDS  ((MagickSizeType)60ULL)

static bool reporting_magick_initialized = false;

void reporting_service_apply_resource_limits(void) {
    MagickSizeType max_dim = REPORTING_DEFAULT_MAX_DIM;
    MagickSizeType memory_bytes = REPORTING_DEFAULT_MEMORY_BYTES;
    MagickSizeType map_bytes = REPORTING_DEFAULT_MAP_BYTES;
    MagickSizeType time_sec = REPORTING_DEFAULT_TIME_SECONDS;

    if (app_config && app_config->reporting.MaxImageSize > 0) {
        max_dim = (MagickSizeType)app_config->reporting.MaxImageSize;
    }

    // Area ~ max_dim^2 pixels (prevents absurd intermediate buffers)
    MagickSizeType area = max_dim * max_dim;
    if (area < max_dim) {
        area = max_dim;
    }

    MagickSetResourceLimit(MemoryResource, memory_bytes);
    MagickSetResourceLimit(MapResource, map_bytes);
    MagickSetResourceLimit(WidthResource, max_dim);
    MagickSetResourceLimit(HeightResource, max_dim);
    MagickSetResourceLimit(AreaResource, area);
    MagickSetResourceLimit(TimeResource, time_sec);

    log_this(SR_REPORTING,
             "Magick resource limits: memory=%llu map=%llu dim=%llu area=%llu time=%llus",
             LOG_LEVEL_DEBUG, 5,
             (unsigned long long)memory_bytes,
             (unsigned long long)map_bytes,
             (unsigned long long)max_dim,
             (unsigned long long)area,
             (unsigned long long)time_sec);
}

bool reporting_service_init(void) {
    if (reporting_magick_initialized) {
        log_this(SR_REPORTING, "Reporting service already initialized", LOG_LEVEL_DEBUG, 0);
        // Re-apply limits in case config became available after first init
        reporting_service_apply_resource_limits();
        return true;
    }

    MagickWandGenesis();
    reporting_magick_initialized = true;
    reporting_service_apply_resource_limits();
    log_this(SR_REPORTING, "Reporting service initialized (MagickWandGenesis)", LOG_LEVEL_DEBUG, 0);
    return true;
}

void reporting_service_cleanup(void) {
    if (!reporting_magick_initialized) {
        log_this(SR_REPORTING, "Reporting service cleanup skipped (not initialized)", LOG_LEVEL_DEBUG, 0);
        return;
    }

    MagickWandTerminus();
    reporting_magick_initialized = false;
    log_this(SR_REPORTING, "Reporting service cleaned up (MagickWandTerminus)", LOG_LEVEL_DEBUG, 0);
}

const char* reporting_service_name(void) {
    return "Reporting";
}

bool reporting_service_is_initialized(void) {
    return reporting_magick_initialized;
}
