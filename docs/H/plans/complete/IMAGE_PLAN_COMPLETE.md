# IMAGE PLAN — Reporting Service: Image Scale Endpoint

## Overview

This plan describes the implementation of a new **Reporting** REST API service for the
Hydrogen Project. The first endpoint in this service is `image_scale`, which accepts a
JSON parameters block (following the same pattern as the Conduit endpoints) containing a
base64-encoded image, a desired output format, and output dimensions in either `px` or
`pt` units. The endpoint uses **ImageMagick** (via MagickWand C API) to decode, scale,
and re-encode the image, returning the result as a base64 string.

A novel output format called **XO** (PDF XObject data part) is also specified. XO encodes
a bitmap plus transparency layer in the stream format used inside PDF XObjects, serving
as an internal intermediate representation for downstream PDF generation.

## Motivation

The endpoint is intended for integration into a **reporting tool**. A caller supplies a
large source image (e.g. a BMP logo) and specifies the exact pixel or point dimensions
needed for the space it occupies on a page at a given resolution. The server performs the
scaling and format conversion, returning a ready-to-embed image. This offloads ImageMagick
dependencies from the reporting client and centralizes image processing.

## Endpoint Design

### URL

``` API
POST /api/reporting/image_scale
```

### Request JSON

```json
{
  "image": "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAA...",
  "format": "png",
  "width": 200,
  "height": 200,
  "units": "px",
  "dpi": 300,
  "scale_algorithm": "lanczos"
}
```

### Parameters

| Parameter         | Type   | Required | Default   | Description                                                                 |
| ----------------- | ------ | -------- | --------- | --------------------------------------------------------------------------- |
| `image`           | string | yes      | —         | Base64-encoded image data. May include a `data:` URI prefix.                |
| `format`          | string | yes      | —         | Output format: `jpg`, `png`, `bmp`, `svg`, `ico`, `webp`, `xo`, or others.  |
| `width`           | int    | yes      | —         | Output width in the specified units.                                        |
| `height`          | int    | yes      | —         | Output height in the specified units.                                       |
| `units`           | string | no       | `px`      | Unit of width/height: `px` (pixels) or `pt` (points, 1/72 inch).            |
| `dpi`             | int    | no       | `72`      | DPI used for pt-to-px conversion.                                           |
| `scale_algorithm` | string | no       | `lanczos` | Scaling algorithm: `nearest`, `bilinear`, `bicubic`, `lanczos`, `mitchell`. |

### Response JSON (success)

```json
{
  "success": true,
  "format": "png",
  "width": 200,
  "height": 200,
  "units": "px",
  "image": "iVBORw0KGgoAAAANSUhEUgAA...",
  "mime_type": "image/png",
  "input_format": "image/bmp",
  "input_dimensions": { "width": 1920, "height": 1080 }
}
```

### Response JSON (error)

```json
{
  "success": false,
  "error": "Invalid image data",
  "details": "Base64 decode failed: invalid character at position 42"
}
```

### Supported Input Formats

Any format ImageMagick can decode. The endpoint auto-detects the input format from the
blob. Common inputs: BMP, PNG, JPEG, GIF, TIFF, WEBP, SVG, ICO.

### Supported Output Formats

| Format  | MIME type         | Notes                                                    |
| ------- | ----------------- | -------------------------------------------------------- |
| `jpg`   | `image/jpeg`      | RGB, quality configurable via future extension.          |
| `png`   | `image/png`       | Supports palette, RGB, RGBA.                             |
| `bmp`   | `image/bmp`       | Uncompressed.                                            |
| `svg`   | `image/svg+xml`   | Vector output; scaling applies to the SVG viewport.      |
| `ico`   | `image/x-icon`    | Windows icon; may contain multiple resolutions.          |
| `webp`  | `image/webp`      | Modern format with lossy/lossless support.               |
| `xo`    | `application/x-xo`| PDF XObject data stream (see XO format below).           |

Additional formats supported by ImageMagick (e.g. `tiff`, `gif`, `ppm`, `tga`) are
available automatically.

## XO Format — PDF XObject Data Stream

The **XO** format is an internal intermediate representation used when building PDFs
elsewhere in the reporting pipeline. It is not intended as a standalone deliverable
format for end users but is exposed through the API for consistency.

### Structure

An XO stream consists of:

1. **Bitmap data** — the pixel data in a PDF-compatible color space (DeviceRGB or
   DeviceGray, or DeviceCMYK for CMYK sources).
2. **Transparency mask** — a separate 1-byte-per-pixel alpha channel encoded as a PDF
   Image XObject with `BitsPerComponent=8` and `ColorSpace=DeviceGray`.

### Encoding

The output is a base64-encoded byte sequence containing:

```format
[4 bytes: width (big-endian uint32)]
[4 bytes: height (big-endian uint32)]
[1 byte:  color_space  (0=Gray, 1=RGB, 2=CMYK)]
[1 byte:  has_alpha    (0 or 1)]
[1 byte:  reserved     (0)]
[1 byte:  reserved     (0)]
[N bytes: pixel data]           — FlateDecode-compressed (zlib/deflate)
[N bytes: alpha data (if has_alpha)] — FlateDecode-compressed (zlib/deflate)
```

Where `channels` = 1 for Gray, 3 for RGB, 4 for CMYK.

The pixel data and alpha data are each compressed with **FlateDecode** (zlib/deflate), which is the standard stream filter for image data in PDF XObjects. The uncompressed size of each stream is `width * height * channels` for pixel data and `width * height` for alpha data. The PDF builder uses these dimensions and the color space to construct the Image XObject dictionaries, then inserts the compressed streams directly into the PDF content stream.

### PDF Integration

When building a PDF elsewhere, the XO data is used to construct two PDF Image XObjects:

- **Color image XObject** — with `/ColorSpace`, `/BitsPerComponent 8`, `/Width`,
  `/Height`, and the FlateDecode-compressed pixel data stream.
- **Mask image XObject** (if `has_alpha=1`) — `/ColorSpace /DeviceGray`,
  `/BitsPerComponent 8`, with the FlateDecode-compressed alpha data stream. This is
  referenced via the `/Mask` key on the color image XObject.

Both streams are already FlateDecode-compressed in the XO format, so the PDF builder
inserts them directly without additional compression. The pixel data is stored in
**top-to-bottom, left-to-right** order, matching PDF's default `ImageOrientation`
(the first pixel is the top-left).

## Implementation Phases

---

## Phase 0: Environment & CMake Integration

### Goals

- Add ImageMagick (MagickWand) as a build dependency.
- Add CMake `pkg_check_modules` for MagickWand.
- Add `ReportingConfig` to the configuration system.
- Add the Reporting subsystem to the launch/landing sequence.

### Tasks

1. **CMake — `cmake/CMakeLists-init.cmake`**: Add `pkg_check_modules(MAGICKWAND REQUIRED MagickWand-7.Q16HDRI)` (or `MagickWand` as fallback). Add `pkg_check_modules(ZLIB REQUIRED zlib)` for FlateDecode compression in the XO encoder. Add `${MAGICKWAND_LIBRARIES}` and `${ZLIB_LIBRARIES}` to `HYDROGEN_BASE_LIBS` and `${MAGICKWAND_INCLUDE_DIRS}` and `${ZLIB_INCLUDE_DIRS}` to `HYDROGEN_INCLUDE_DIRS`. Add `MAGICKWAND_CFLAGS` and `ZLIB_CFLAGS` to the compile command in `CMakeLists-init.cmake` and `CMakeLists-coverage.cmake`.

2. **CMake — `cmake/CMakeLists-init.cmake`**: Add `MAGICKWAND_CFLAGS` and `ZLIB_CFLAGS` to the per-source compile commands in `hydrogen_add_executable_target()` and the coverage loop.

3. **Config — `src/config/config_forward.h`**: Add forward declarations for `ReportingConfig`.

4. **Config — `src/config/config.h`**: Add `#include "config_reporting.h"` and add section letter `R. Reporting` to the section list comment.

5. **Config — `src/config/config_reporting.h`**: Define `ReportingConfig` struct with fields:
   - `bool Enabled`
   - `int MaxImageSize` (max dimension in pixels, default 8192)
   - `int MaxInputBytes` (max base64 input size, default 50 MB)
   - `int MaxOutputBytes` (max base64 output size, default 50 MB)
   - `int DefaultDPI` (default 72)
   - `char* AllowedFormats` (comma-separated list, or NULL for all)

6. **Config — `src/config/config_reporting.c`**: Implement `load_reporting_config()`, `cleanup_reporting_config()`, `dump_reporting_config()`.

7. **Config — `src/config/config_defaults.h`**: Add `initialize_config_defaults_reporting()` prototype.

8. **Config — `src/config/config_defaults.c`**: Implement `initialize_config_defaults_reporting()` with defaults. Call from `initialize_config_defaults()`.

9. **Config — `src/config/config.c`**: Add `cleanup_reporting_config()` call in the cleanup section.

10. **Launch — `src/launch/launch.c`**: Add `launch_reporting_subsystem()` to the launch plan and review. Add `R` to the subsystem order comment.

11. **Launch — `src/launch/launch.h`**: Add `launch_reporting_subsystem()` prototype if needed.

12. **Landing — `src/landing/`**: Add `landing_reporting_subsystem()` cleanup.

13. **Dependency check — `src/utils/utils_dependency.c`**: Add MagickWand and zlib to the `lib_configs[]` array with appropriate paths and version functions (`MagickGetVersion` for ImageMagick, `zlibVersion` for zlib).

14. **Dependency check — `tests/test_14_library_dependencies.sh`**: Add `check_dependency_log "MagickWand"` and `check_dependency_log "zlib"` to the dependency check list.

15. **Documentation — `docs/H/SETUP.md`**: Add ImageMagick (libmagickwand-dev) to the runtime dependencies list and the Ubuntu build environment example.

16. **Documentation — `docs/H/plans/IMAGE_PLAN.md`**: This file. Update as phases progress.

### Stage Gate 0 — Validation

- **Build**: `zsh -ic 'mkt'` succeeds with MagickWand linked.
- **Config**: `zsh -ic 'mku config_test'` or a new `config_reporting_test` passes (defaults load correctly).
- **Dependency check**: `zsh -ic 'mku utils_dependency_test'` passes with MagickWand added to the config array.
- **Dependency check**: `test_14_library_dependencies.sh` passes with MagickWand detected.

---

## Phase 1: Core Image Processing Module

### Phase 1 Goals

- Create `src/reporting/` directory with the image processing core.
- Implement base64 decode/encode, ImageMagick initialization, format mapping, and the XO encoder.
- All functions non-static (per project rules) with header declarations for Unity testing.

### Directory Structure

```structure
src/reporting/
  reporting_service.c       # Service init/cleanup, name
  reporting_service.h       # Service header with swagger annotations
  image_scale/
    image_scale.c           # Endpoint handler
    image_scale.h           # Endpoint header with swagger annotations
  helpers/
    base64_utils.c          # Base64 decode/encode with data: URI stripping
    base64_utils.h
    image_format.c          # Format string <-> ImageMagick format mapping
    image_format.h
    image_xo.c              # XO format encoding
    image_xo.h
    image_scale_core.c      # Core scaling logic using MagickWand
    image_scale_core.h
```

### Phase 1 Tasks

1. **Create `src/reporting/reporting_service.h`**: Service-level swagger annotations (`//@ swagger:tag "Reporting Service"`).

2. **Create `src/reporting/reporting_service.c`**: `reporting_service_init()`, `reporting_service_cleanup()`, `reporting_service_name()`.

3. **Create `src/reporting/helpers/base64_utils.h` + `.c`**:
   - `char* base64_decode(const char* input, size_t* out_len)` — strips `data:` URI prefix, decodes base64.
   - `char* base64_encode(const unsigned char* data, size_t len)` — encodes to base64.
   - `bool parse_data_uri(const char* input, const char** base64_start)` — extracts base64 portion from `data:` URIs.

4. **Create `src/reporting/helpers/image_format.h` + `.c`**:
   - `int format_to_imagemagick(const char* format, char* magick_out, size_t out_len)` — maps `jpg`→`JPEG`, `png`→`PNG`, etc.
   - `const char* format_to_mime(const char* format)` — returns MIME type string.
   - `bool format_is_supported(const char* format, const char* allowed_formats)` — checks against allowed list.
   - `bool format_is_xo(const char* format)` — returns true for `xo`.

5. **Create `src/reporting/helpers/image_xo.h` + `.c`**:
   - `char* encode_xo_stream(MagickWand* wand, size_t* out_len)` — extracts pixel data and alpha, FlateDecode-compresses each stream, builds XO binary with header, returns base64-encoded.
   - `bool xo_needs_alpha(MagickWand* wand)` — checks if image has an alpha channel.

6. **Create `src/reporting/helpers/image_scale_core.h` + `.c`**:
   - `MagickWand* scale_image_core(const unsigned char* data, size_t data_len, int width, int height, const char* units, int dpi, const char* scale_algorithm, const char* format, char** error_msg)` — core function: initializes MagickWand, reads blob, scales, sets output format, returns wand (caller must destroy).
   - `bool parse_dimensions(int width, int height, const char* units, int dpi, int* out_width, int* out_height)` — converts pt to px if needed.
   - `const char* get_scale_filter(const char* algorithm)` — maps algorithm name to ImageMagick filter name.

7. **Create `src/reporting/image_scale/image_scale.h`**: Endpoint header with full swagger annotations matching the request/response schema. Declare `handle_reporting_image_scale_request()`.

8. **Create `src/reporting/image_scale/image_scale.c`**: Endpoint handler following the conduit pattern:
   - Use `api_buffer_post_data()` for POST body buffering.
   - Parse JSON body with jansson.
   - Validate all parameters.
   - Call `scale_image_core()`.
   - If format is `xo`, call `encode_xo_stream()`.
   - Otherwise, use `MagickImage()` to get the encoded blob.
   - Base64-encode the result.
   - Build and send JSON response via `api_send_json_response()`.

### Stage Gate 1 — Validation

- **Build**: `zsh -ic 'mkt'` succeeds.
- **Unit tests**: `zsh -ic 'mku base64_utils_test'` passes — tests base64 encode/decode, data URI stripping.
- **Unit tests**: `zsh -ic 'mku image_format_test'` passes — tests format mapping, MIME types, XO detection.
- **Unit tests**: `zsh -ic 'mku image_xo_test'` passes — tests XO encoding with known pixel data.
- **Unit tests**: `zsh -ic 'mku image_scale_core_test'` passes — tests dimension conversion, filter mapping, core scaling with a small test image.
- **cppcheck**: `zsh -ic 'mkp'` passes with no issues.

---

## Phase 2: API Endpoint Wiring

### Phase 2 Goals

- Register the endpoint in the API service.
- Ensure swagger annotations are discovered (see layout decision below).
- Add to JSON endpoint and auth lists.
- Confirm JWT middleware + disabled-subsystem behavior.

### Phase 2 Status Note (post–Phase 1)

Core handler and swagger annotations already exist under `src/reporting/`. Launch/landing
already call `reporting_service_init` / `cleanup`. Phase 2 is **wiring only**, not
re-implementing the handler — but swagger discovery and include paths differ from the
original Phase 2 draft (corrected below).

### Phase 2 Tasks

1. **Edit `src/api/api_service.c`**:
   - Include with project path style (not relative `"reporting/..."`):

     ```c
     #include <src/reporting/image_scale/image_scale.h>
     ```

     (`reporting_service.h` is optional here; the handler does not require it at the
     route layer.)

   - Add route in `handle_api_request()` (near mailrelay routes):

     ```c
     else if (strcmp(path, "reporting/image_scale") == 0) {
         return handle_reporting_image_scale_request(connection, url, method, upload_data,
                                                      upload_data_size, con_cls);
     }
     ```

   - Add `"reporting/image_scale"` to `json_endpoints[]` in `api_service_endpoint_expects_json()`.
   - Add `"reporting/image_scale"` to `protected_endpoints[]` in `api_service_endpoint_requires_auth()`
     (plan decision: reporting requires JWT via middleware — handler does **not** currently
     re-validate JWT itself; middleware is required for auth).
   - Add log line in `register_api_endpoints()` for the new endpoint.

2. **Launch wiring**: **Already done in Phase 0** (`launch_reporting_subsystem` /
   `land_reporting_subsystem` / readiness). No further launch.c work unless registration
   log text is desired.

3. **Swagger layout (required decision — see Phase 2 Assessment)**:
   - `payloads/swagger-generate.sh` scans **only** `src/api/*/` for `*_service.h` and
     endpoint headers. Annotations currently live under `src/reporting/`, so they will
     **not** be discovered as-is.
   - Preferred approach (matches mailrelay): add a thin API surface under
     `src/api/reporting/`:
     - `src/api/reporting/reporting_service.h` — service-level swagger tags (may mirror
       or `#include` / duplicate annotations from `src/reporting/reporting_service.h`)
     - `src/api/reporting/image_scale/image_scale.h` — endpoint swagger + thin declare
       of `handle_reporting_image_scale_request` (implementation stays in
       `src/reporting/image_scale/image_scale.c`)
   - Alternative: extend `swagger-generate.sh` to also scan `src/reporting/`. Avoid unless
     there is a strong reason not to follow the mailrelay split.

4. **Auth / JSON middleware smoke** (manual or minimal check before Phase 3):
   - With Reporting disabled: expect 503 from handler (`Reporting disabled`).
   - With Reporting enabled, no JWT: expect 401 from middleware.
   - With JWT + valid body: expect 200 (full matrix is Phase 3 / Test 27).

### Stage Gate 2 — Validation

- **Build**: `zsh -ic 'mkt'` succeeds.
- **Swagger**: Run `swagger-generate.sh` and verify `/api/reporting/image_scale` (or
  configured path) appears in `swagger.json` under Reporting Service.
- **cppcheck**: `zsh -ic 'mkp'` passes.
- **Optional smoke**: one curl with/without JWT against a local binary (full blackbox is Phase 3).

---

## Phase 3: Blackbox Test (Test 27)

### Phase 3 Goals

- Create `tests/test_27_reporting_image_scale.sh`.
- Create test configuration JSON.
- Add sample images to `tests/artifacts/images/`.
- Test the endpoint end-to-end with the Hydrogen server.

### Test Artifacts

Add sample images to `tests/artifacts/images/`:

| File           | Format | Dimensions  | Purpose                              |
| -------------- | ------ | ----------- | ------------------------------------ |
| `sample.png`   | PNG    | 256x256     | Existing file, used for PNG→PNG.     |
| `sample.bmp`   | BMP    | 1920x1080   | Large BMP for scaling down to logo.  |
| `sample.jpg`   | JPEG   | 800x600     | JPEG input/output testing.           |
| `sample.webp`  | WEBP   | 512x512     | WebP input/output testing.           |
| `sample_ico`   | ICO    | 64x64       | ICO input/output testing.            |
| `sample.svg`   | SVG    | vector      | SVG input/output testing.            |

### Test Configuration

Create `tests/configs/hydrogen_test_27_reporting.json`:

```json
{
  "WebServer": {
    "Port": 5270,
    "BindAddress": "127.0.0.1"
  },
  "API": {
    "Prefix": "/api"
  },
  "Reporting": {
    "Enabled": true,
    "MaxImageSize": 8192,
    "MaxInputBytes": 52428800,
    "MaxOutputBytes": 52428800,
    "DefaultDPI": 72
  }
}
```

Port `5270` follows the `5<T#>x` scheme (Test 27 → `5270`).

### Test Script Structure

Following the pattern of `test_50_conduit_query.sh`:

```bash
#!/usr/bin/env bash
# Test: Reporting Image Scale Endpoint
# Tests the /api/reporting/image_scale endpoint

set -euo pipefail

TEST_NAME="Reporting Image Scale"
TEST_ABBR="RIS"
TEST_NUMBER="27"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# Source framework
source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_27_reporting.json"
LOG_SUFFIX="reporting_image_scale"
DESCRIPTION="RIS"

# Functions:
# test_image_scale_png_to_png()
# test_image_scale_bmp_to_png()
# test_image_scale_jpg_to_webp()
# test_image_scale_pt_units()
# test_image_scale_xo_format()
# test_image_scale_invalid_image()
# test_image_scale_missing_param()
# test_image_scale_invalid_format()
# test_image_scale_unsupported_format()
# test_image_scale_auth_required()
# test_image_scale_no_auth()
```

### Test Cases

1. **PNG to PNG scale**: Send `sample.png` (256x256), request 128x128 PNG. Verify response `success=true`, `format=png`, dimensions match.
2. **BMP to PNG scale**: Send `sample.bmp` (1920x1080), request 200x200 PNG. Verify scaled output.
3. **JPEG to WebP**: Send `sample.jpg`, request WebP output. Verify format and MIME type.
4. **Point units**: Send image, request 2x2 inches at 300 DPI → 600x600 px. Verify dimensions.
5. **XO format**: Send image, request `xo` format. Verify response contains XO data with correct header bytes.
6. **Invalid image**: Send garbage base64. Expect 400 with error.
7. **Missing parameter**: Omit `format`. Expect 400.
8. **Invalid format**: Request `format=invalid`. Expect 400.
9. **Unsupported format**: Request a format not in the allowed list (if configured). Expect 400.
10. **Auth required**: Request without JWT. Expect 401.
11. **No auth (public)**: If endpoint is public, verify it works without JWT. (Decision: reporting endpoints require auth, so this tests the 401 path.)
12. **Scale algorithm**: Test with `nearest`, `bilinear`, `bicubic`, `lanczos`.

### Stage Gate 3 — Validation

- **Build**: `zsh -ic 'mkt'` succeeds.
- **Blackbox**: `zsh -ic 'tests/test_27_reporting_image_scale.sh'` passes all test cases.
- **Coverage**: Run `extras/add_coverage.sh reporting/image_scale/image_scale.c` and verify coverage is reasonable.

---

## Phase 4: Unity Unit Test Expansion

### Phase 4 Goals

- Expand Unity unit tests for the reporting module to improve coverage.
- Target high-value functions that are testable in isolation.

### PHase 4 Tasks

1. **`tests/unity/src/reporting/base64_utils_test.c`**: Test base64 encode/decode round-trip, data URI stripping, empty input, invalid characters.

2. **`tests/unity/src/reporting/image_format_test.c`**: Test format mapping for all supported formats, MIME type lookup, XO detection, unsupported format rejection.

3. **`tests/unity/src/reporting/image_xo_test.c`**: Test XO encoding with a synthetic pixel buffer (known RGB data + alpha). Verify header bytes, pixel data order, alpha channel presence.

4. **`tests/unity/src/reporting/image_scale_core_test.c`**: Test `parse_dimensions()` for px/pt conversion at various DPIs. Test `get_scale_filter()` for all algorithm names. Test `scale_image_core()` with a small embedded PNG.

5. **`tests/unity/src/reporting/image_scale/image_scale_test_handle_reporting_image_scale_request.c`**: Test the endpoint handler with mocked `scale_image_core` (if needed) or with real ImageMagick on a small image. Test parameter validation, missing params, invalid JSON, invalid format.

### Stage Gate 4 — Validation

- **Unity**: All `mku` tests pass.
- **Coverage**: `extras/add_coverage.sh` shows improved coverage for `src/reporting/` files.
- **cppcheck**: `zsh -ic 'mkp'` passes.

---

## Phase 5: SVG Considerations & Edge Cases

### Phase 5 Goals

- Handle SVG input and output properly.
- Handle edge cases like animated images, multi-frame images, EXIF orientation.

### Phase 5 Tasks

1. **SVG input**: When reading an SVG, ImageMagick rasterizes it. The `density` setting controls the output resolution. Ensure `density` is set from the `dpi` parameter before reading.

2. **SVG output**: When outputting SVG, the scaling is applied to the SVG viewport element. ImageMagick handles this via the `svg` coder.

3. **EXIF orientation**: Call `MagickAutoOrientImage()` after reading to apply EXIF orientation tags.

4. **Multi-frame images**: For GIF/animated WebP, only the first frame is processed. Document this limitation.

5. **Color profile**: Strip ICC profiles from output for web formats to reduce size. For BMP, profiles are not applicable.

6. **Transparency**: When converting to JPEG (no alpha), flatten onto a white background. When converting to formats with alpha (PNG, WEBP), preserve the alpha channel.

### Stage Gate 5 — Validation

- **Blackbox**: Test SVG input → PNG output. Test PNG with transparency → JPEG (verify no transparency artifacts).
- **Build**: `zsh -ic 'mkt'` succeeds.

---

## Phase 6: Performance & Limits

### Phase 6 Goals

- Enforce input/output size limits.
- Add timeout for long-running operations.
- Document performance characteristics.

### Phase 6 Tasks

1. **Size limits**: In `image_scale.c`, check `Content-Length` or accumulated buffer size against `ReportingConfig.MaxInputBytes`. Check output blob size against `MaxOutputBytes`.

2. **Dimension limits**: Check requested `width`/`height` against `MaxImageSize`. Reject if either exceeds.

3. **Timeout**: Set a resource limit on ImageMagick operations using `MagickSetResourceLimit()` to prevent memory exhaustion. Set `MAGICK_MEMORY_LIMIT` and `MAGICK_MAP_LIMIT`.

4. **Memory**: Ensure all MagickWand objects are properly destroyed with `DestroyMagickWand()` and `MagickWandTerminus()` is called on cleanup.

### Stage Gate 6 — Validation

- **Blackbox**: Test oversized input (reject with 413 or 400). Test oversized dimensions (reject with 400).
- **ASAN**: `zsh -ic 'mku image_scale_core_test'` under ASAN shows no leaks.
- **Build**: `zsh -ic 'mkt'` succeeds.

---

## Phase 7: Documentation & Closeout

### Phase 7 Goals

- Add documentation for the Reporting service.
- Update all index files.
- Move plan to `complete/` when done.

### Phase 7 Tasks

1. **Create `docs/H/api/reporting/reporting_endpoints.md`**: Done — API documentation for the Reporting service.

2. **Update `docs/H/plans/README.md`**: Done — removed from active; listed under completed as `IMAGE_PLAN_COMPLETE.md`.

3. **Update `docs/H/INSTRUCTIONS.md`**: Done earlier (Test 27 already listed).

4. **Update `docs/H/tests/TESTING.md`**: Done earlier (Test 27 already listed); Test 27 md filled in.

5. **Update `docs/H/SITEMAP.md`**: Done — reporting API, Test 27, complete plan.

6. **Update `docs/H/STRUCTURE.md`**: Done — reporting sources, config, launch/landing, test 27.

7. **Update `RELEASES.md`**: Done — 2026-07-29 entry + daily notes.

### Stage Gate 7 — Validation

- **Markdown lint**: `zsh -ic 'tests/test_90_markdownlint.sh'` passes.
- **Link check**: `zsh -ic 'tests/test_04_check_links.sh'` passes.
- **All tests**: `zsh -ic 'tests/test_00_all.sh 27_reporting_image_scale'` passes.

---

## CMake Integration Details

### Adding MagickWand

In `cmake/CMakeLists-init.cmake`, add after the existing `pkg_check_modules` calls:

```cmake
pkg_check_modules(MAGICKWAND REQUIRED MagickWand-7.Q16HDRI)
```

If the HDRI variant is not available, fall back to:

```cmake
pkg_check_modules(MAGICKWAND REQUIRED MagickWand)
```

Also add zlib for FlateDecode compression in the XO encoder:

```cmake
pkg_check_modules(ZLIB REQUIRED zlib)
```

Add to `HYDROGEN_BASE_LIBS`:

```cmake
${MAGICKWAND_LIBRARIES}
${ZLIB_LIBRARIES}
```

Add to `HYDROGEN_INCLUDE_DIRS`:

```cmake
${MAGICKWAND_INCLUDE_DIRS}
${ZLIB_INCLUDE_DIRS}
```

Add `MAGICKWAND_CFLAGS` and `ZLIB_CFLAGS` to the compile commands in:

- `hydrogen_add_executable_target()` in `CMakeLists-init.cmake`
- The coverage object file loop in `CMakeLists-coverage.cmake`

### Source Discovery

No CMake changes are needed for source file discovery — `file(GLOB_RECURSE HYDROGEN_SOURCES "../src/*.c")` in `CMakeLists-init.cmake` automatically picks up new `.c` files under `src/reporting/`.

### Unity Test Discovery

Unity tests are auto-discovered via `file(GLOB_RECURSE UNITY_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/src/*_test*.c")`. New test files under `tests/unity/src/reporting/` will be built automatically.

### Mock System

The reporting module does not require database, network, or threading mocks. The only external dependency is ImageMagick itself. For Unity tests, the `mock_system` (malloc/free/strdup) is globally defined and will be active. If the reporting code needs to be tested with mocked ImageMagick calls, a `mock_magickwand` can be added later, but the initial approach is to test with real ImageMagick on small images.

---

## Swagger Integration

The swagger generation script (`payloads/swagger-generate.sh`) scans `src/api/*/` for service directories containing `*_service.h` files and endpoint subdirectories with `*.h` files containing `//@ swagger:path` annotations.

### Service-level annotations (`src/reporting/reporting_service.h`)

```c
//@ swagger:title Reporting Service API
//@ swagger:description REST API for image processing and reporting operations.
//@ swagger:version 1.0.0
//@ swagger:tag "Reporting Service" Provides image processing and conversion services.
```

### Endpoint-level annotations (`src/reporting/image_scale/image_scale.h`)

```c
//@ swagger:path /api/reporting/image_scale
//@ swagger:method POST
//@ swagger:operationId imageScale
//@ swagger:tags "Reporting Service"
//@ swagger:summary Scale and convert an image
//@ swagger:description Scales and converts a base64-encoded image to a specified format and dimensions.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["image","format","width","height"],"properties":{...}}
//@ swagger:response 200 application/json {"type":"object","properties":{"success":{"type":"boolean"},"image":{"type":"string"},"format":{"type":"string"},"width":{"type":"integer"},"height":{"type":"integer"},"units":{"type":"string"},"mime_type":{"type":"string"},"input_format":{"type":"string"},"input_dimensions":{"type":"object"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean"},"error":{"type":"string"},"details":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean"},"error":{"type":"string"}}}
```

---

## Test Artifacts & Sample Images

### Directory: `tests/artifacts/images/`

Existing: `sample.png` (256x256 PNG).

To add:

| File          | Source          | How to create                                                |
| ------------- | --------------- | ------------------------------------------------------------ |
| `sample.bmp`  | Generated       | Use ImageMagick: `convert -size 1920x1080 xc:red sample.bmp` |
| `sample.jpg`  | Generated       | `convert -size 800x600 plasma: sample.jpg`                   |
| `sample.webp` | Generated       | `convert -size 512x512 plasma: sample.webp`                  |
| `sample.ico`  | Generated       | `convert -size 64x64 xc:blue sample.ico`                     |
| `sample.svg`  | Hand-written    | Simple SVG with a rectangle and circle.                      |

### Test Framework

The test script will use the existing test framework (`tests/lib/framework.sh`) which provides:

- `setup_test_environment` — sets up paths, counters, etc.
- `print_subtest`, `print_result`, `print_message` — output formatting.
- `validate_config_file` — validates JSON config.
- `find_hydrogen_binary` — locates the Hydrogen binary.
- `run_conduit_server` / `shutdown_conduit_server` — server lifecycle (adapted for reporting).

The test will start Hydrogen with the test config, acquire a JWT (via `/api/auth/login`), and make requests to `/api/reporting/image_scale`.

---

## Port Numbering

Test 27 uses port `5270` (format: `5<T#>x` = `5` + `27` + `0`).

The port is set in `tests/configs/hydrogen_test_27_reporting.json` under `WebServer.Port`.

---

## Configuration Schema

The `Reporting` section in JSON config:

```json
{
  "Reporting": {
    "Enabled": true,
    "MaxImageSize": 8192,
    "MaxInputBytes": 52428800,
    "MaxOutputBytes": 52428800,
    "DefaultDPI": 72,
    "AllowedFormats": "jpg,png,bmp,svg,ico,webp,xo"
  }
}
```

- `Enabled` (bool, default `false`): Master switch for the Reporting service.
- `MaxImageSize` (int, default `8192`): Maximum output dimension in pixels.
- `MaxInputBytes` (int, default `52428800`): Maximum base64 input size in bytes (50 MB).
- `MaxOutputBytes` (int, default `52428800`): Maximum base64 output size in bytes (50 MB).
- `DefaultDPI` (int, default `72`): Default DPI for pt-to-px conversion.
- `AllowedFormats` (string, default `NULL`): Comma-separated list of allowed output formats. `NULL` means all formats are allowed.

---

## File Inventory

### New Source Files

| File | Purpose |
| ---- | ------- |
| `src/reporting/reporting_service.h` | Service header with swagger annotations |
| `src/reporting/reporting_service.c` | Service init/cleanup |
| `src/reporting/image_scale/image_scale.h` | Endpoint header with swagger annotations |
| `src/reporting/image_scale/image_scale.c` | Endpoint handler |
| `src/reporting/helpers/base64_utils.h` | Base64 utility declarations |
| `src/reporting/helpers/base64_utils.c` | Base64 encode/decode implementation |
| `src/reporting/helpers/image_format.h` | Format mapping declarations |
| `src/reporting/helpers/image_format.c` | Format mapping implementation |
| `src/reporting/helpers/image_xo.h` | XO encoder declarations |
| `src/reporting/helpers/image_xo.c` | XO encoder implementation |
| `src/reporting/helpers/image_scale_core.h` | Core scaling declarations |
| `src/reporting/helpers/image_scale_core.c` | Core scaling implementation |

### New Config Files

| File | Purpose |
| ---- | ------- |
| `src/config/config_reporting.h` | ReportingConfig struct |
| `src/config/config_reporting.c` | Config loading/cleanup |

### Modified Files

| File | Change |
| ---- | ------ |
| `cmake/CMakeLists-init.cmake` | Add MagickWand pkg_check_modules, libs, includes, CFLAGS |
| `cmake/CMakeLists-coverage.cmake` | Add MagickWand CFLAGS to coverage compile commands |
| `src/config/config_forward.h` | Add ReportingConfig forward declaration |
| `src/config/config.h` | Add `#include "config_reporting.h"`, section R |
| `src/config/config_defaults.h` | Add `initialize_config_defaults_reporting()` |
| `src/config/config_defaults.c` | Implement defaults, call from main init |
| `src/config/config.c` | Add cleanup call |
| `src/api/api_service.c` | Add route, JSON endpoint, auth endpoint |
| `src/launch/launch.c` | Add launch_reporting_subsystem |
| `src/launch/launch.h` | Add prototype if needed |
| `src/landing/` | Add landing_reporting_subsystem |
| `src/utils/utils_dependency.c` | Add MagickWand and zlib to lib_configs[] |

### New Test Files

| File | Purpose |
| ---- | ------- |
| `tests/test_27_reporting_image_scale.sh` | Blackbox test |
| `tests/configs/hydrogen_test_27_reporting.json` | Test config |
| `tests/unity/src/reporting/base64_utils_test.c` | Unity: base64 utils |
| `tests/unity/src/reporting/image_format_test.c` | Unity: format mapping |
| `tests/unity/src/reporting/image_xo_test.c` | Unity: XO encoder |
| `tests/unity/src/reporting/image_scale_core_test.c` | Unity: core scaling |
| `tests/unity/src/reporting/image_scale/image_scale_test_handle_reporting_image_scale_request.c` | Unity: endpoint handler |

### Modified Test Files

| File | Change |
| ---- | ------ |
| `tests/test_14_library_dependencies.sh` | Add `check_dependency_log "MagickWand"` |

### New Test Artifacts

| File | Purpose |
| ---- | ------- |
| `tests/artifacts/images/sample.bmp` | Large BMP for scaling |
| `tests/artifacts/images/sample.jpg` | JPEG input/output |
| `tests/artifacts/images/sample.webp` | WebP input/output |
| `tests/artifacts/images/sample.ico` | ICO input/output |
| `tests/artifacts/images/sample.svg` | SVG input/output |

### New Documentation Files

| File | Purpose |
| ---- | ------- |
| `docs/H/api/reporting/reporting_endpoints.md` | API documentation |

### Modified Documentation Files

| File | Change |
| ---- | ------ |
| `docs/H/plans/README.md` | Add IMAGE_PLAN link |
| `docs/H/INSTRUCTIONS.md` | Add Test 27 to test list |
| `docs/H/tests/TESTING.md` | Add Test 27 to test list |
| `docs/H/SITEMAP.md` | Add new doc links |
| `docs/H/STRUCTURE.md` | Add new source files |
| `RELEASES.md` | Note new Reporting service |

---

## Open Questions & Considerations

Status legend: **Resolved** | **Deferred** | **Open (Phase 2)** | **Open (later)**

1. **MagickWand version** — **Resolved (Phase 0)**: Using `MagickWand-7.Q16HDRI` via pkg-config; environment verified ImageMagick 7.1.x. No IM6 fallback implemented (acceptable for current hosts).

2. **zlib dependency** — **Resolved (Phase 0/1)**: Explicit CMake + `utils_dependency` + XO `compress2()`.

3. **SVG rasterization** — **Resolved (Phase 5)**: `MagickSetOption(density)` + `MagickSetResolution` before `MagickReadImageBlob`; Test 27 SVG→PNG.

4. **Animated images** — **Resolved (Phase 5)**: `scale_image_core_keep_first_frame()`; first frame only (document in Phase 7 API docs).

5. **Color space / transparency** — **Resolved (Phase 5)**: Flatten alpha onto white for JPEG/BMP; preserve alpha for PNG/WEBP; strip ICC on web formats.

6. **Memory limits** — **Resolved (Phase 6)**: `reporting_service_apply_resource_limits()` sets Memory/Map/Width/Height/Area/Time on init.

7. **Thread safety** — **Resolved (Phase 1)**: Per-request wand; genesis/terminus in `reporting_service_init`/`cleanup` via launch/landing.

8. **Error messages** — **Partially done (Phase 1)**: `MagickGetException` → heap string on core failures; can refine wording later.

9. **Base64 line length** — **Resolved (Phase 1)**: `utils_base64_decode` strips whitespace.

10. **JPEG quality** — **Open (later)**: Default ImageMagick quality; optional `quality` param is a future extension.

11. **ICO multi-resolution** — **Resolved (Phase 5 behavior)**: Same first-frame path as multi-frame; multi-res ICO not expanded (document in Phase 7).

12. **CMake modification constraint** — **Resolved (waived for Phases 0–1)**: CFLAGS also required in `CMakeLists-unity.cmake` (Phase 1 lesson). Further CMake edits only if new libs appear.

13. **Swagger / source layout** — **Resolved (Phase 2)**: Thin `src/api/reporting/` headers (service tag + endpoint swagger + handler prototype). Implementation stays in `src/reporting/`. Generator unchanged.

14. **JWT enforcement path** — **Resolved (Phase 2)**: `reporting/image_scale` listed in `protected_endpoints[]`. Middleware rejects without JWT before body work; handler does not re-validate JWT.

15. **POST size vs MaxInputBytes** — **Resolved (Phase 6 testing)**: Test 27 uses MaxIn/Out 9e6 so reporting 413 is reachable under `API_MAX_POST_SIZE` (10 MiB). Production default 50 MB still exceeds API buffer — document in Phase 7 (raise API buffer or lower default).

16. **Double buffering** — **Resolved (Phase 2)**: Same pattern as conduit: JSON middleware buffers via `api_buffer_post_data` into `con_cls`; handler calls `api_buffer_post_data` again and gets `API_BUFFER_COMPLETE` with the existing buffer (no second accumulation). Safe.

---

## Phased Implementation Process

This plan follows a review-implement-verify cycle. Each phase is executed as:

1. **Review** — Assess the phase plan, confirm approach, identify risks.
2. **Implement** — Write code, config, tests, documentation for the phase.
3. **Verify** — Run `zsh -ic 'mkt'`, `zsh -ic 'mkp'`, and `mku` tests. Document results.
4. **Document** — Update this plan's Working Log with outcomes and lessons learned.
5. **Proceed** — Move to the next phase only after stage gate validation passes.

### Phase Execution Order

| Phase | Focus | Key Deliverables | Stage Gate |
| ------- | ------- | ----------------- | ------------ |
| 0 | Environment & CMake | MagickWand/zlib in CMake, ReportingConfig, launch/landing, dependency check | `mkt` builds, config test passes, dependency test passes |
| 1 | Core image processing | base64_utils, image_format, image_xo, image_scale_core, endpoint handler | `mkt` builds, 4 Unity tests pass, `mkp` clean |
| 2 | API endpoint wiring | Route registration, JSON/auth endpoint lists, swagger annotations | `mkt` builds, swagger.json includes endpoint, `mkp` clean |
| 3 | Blackbox test (Test 27) | Test script, config JSON, sample images, 12 test cases | `mkt` builds, blackbox test passes, coverage checked |
| 4 | Unity test expansion | 5 Unity test files for reporting module | All `mku` tests pass, coverage improved, `mkp` clean |
| 5 | SVG & edge cases | SVG density, EXIF orientation, multi-frame, color profiles, transparency | Blackbox SVG tests pass, `mkt` builds |
| 6 | Performance & limits | Size limits, dimension limits, resource limits, ASAN clean | Blackbox limit tests pass, ASAN clean, `mkt` builds |
| 7 | Documentation & closeout | API docs, index updates, markdown lint, link check | `test_90`, `test_04`, `test_27` all pass |

### Build & Test Commands

All commands run via zsh to load aliases from `~/.zshrc`:

```bash
zsh -ic 'mkt'                           # Trial build (C code changes)
zsh -ic 'mku <test_name>'               # Build and run Unity test
zsh -ic 'mkp'                           # cppcheck (C lint)
zsh -ic 'mks'                           # shellcheck (Bash lint)
zsh -ic 'tests/test_27_reporting_image_scale.sh'  # Blackbox test
extras/add_coverage.sh <source.c>       # Coverage analysis
```

### Project Conventions Checklist

- [x] No `static` functions in `src/` (enforced by build)
- [x] `#include <src/hydrogen.h>` first in all `.c` files
- [x] `#include <src/folder/...>` style includes (no `../../..`)
- [x] All functions have header declarations
- [x] Use `log_this` for logging
- [x] No GOTO
- [x] Unity test naming: `<source>_test_<function>.c`
- [x] Bash: use `jq` for JSON, `[[ ]]` over `[ ]`, full `${var}` declarations
- [x] Markdown: absolute links, no line refs, headings not bold

---

## Working Log

- **Phase 0**: Complete. Build passes, config test passes (11/11), dependency test passes (29/29), cppcheck clean. MagickWand+zlib CMake, ReportingConfig, SR_REPORTING launch/landing, dependency check. (Earlier note: shutdown/test_14 noise was pre-existing; `mkt` later passed shutdown cleanly after Phase 1.)
- **Phase 1**: Complete (2026-07-29). Core under `src/reporting/`. Delivered: `reporting_service` (MagickWandGenesis/Terminus), `helpers/base64_utils` (+ shared `utils_base64_decode`), `image_format`, `image_xo`, `image_scale_core`, `image_scale` handler (**not** registered in `api_service.c`). Launch/landing call reporting_service. Unity CMake MagickWand/zlib CFLAGS. Stage gate: `mkt` OK, helper Unity 27/27 (`base64_utils` 8, `image_format` 7, `image_xo` 5, `image_scale_core` 7), launch/landing reporting Unity OK, `mkp` clean (1,859 files).
- **Phase 2**: Complete (2026-07-29). Thin `src/api/reporting/` swagger surface; `api_service.c` route + `json_endpoints[]` + `protected_endpoints[]` + registration log. Stage gate: `mkt` OK, swagger `/reporting/image_scale` POST under Reporting Service with bearerAuth, `mkp` clean (1,861 files).
- **Phase 3**: Complete (2026-07-29). `test_27_reporting_image_scale.sh` + config + sample images. Stage gate: 21/21 subtests, 14/14 image_scale cases, `mks` clean. MaxInputBytes set to 10 MiB in test config to match `API_MAX_POST_SIZE`.
- **Phase 4**: Complete (2026-07-29). Handler Unity + service Unity + expanded helpers. Totals: handler 14, service 4, base64 8, format 10, xo 8, scale_core 7. `mkt`/`mkp` clean.
- **Phase 5**: Complete (2026-07-29). SVG density+resolution before read; EXIF AutoOrient; first-frame only for multi-frame; ICC strip for web formats; JPEG/BMP alpha flatten onto white. Unity image_scale_core 10/10; test_27 23/23 (16 image cases incl. SVG→PNG, PNG→JPEG flatten). `mkt`/`mkp`/`mks` clean.
- **Phase 6**: Complete (2026-07-29). Magick resource limits on init (memory/map 512MiB, width/height/area from MaxImageSize, time 60s). Input/output limits use 413 CONTENT_TOO_LARGE; decoded blob also checked. MaxOutputBytes on base64 length. test_27 25/25 (18 image cases: dims 400, input 413). Unity service 5, handler 14, core 10. `mkt`/`mkp`/`mks` clean. Test config MaxIn/Out 9e6 so handler limit is reachable under API_MAX_POST_SIZE.
- **Phase 7**: Complete (2026-07-29). API docs `docs/H/api/reporting/reporting_endpoints.md`; Test 27 doc filled; SITEMAP/STRUCTURE/API_OVERVIEW/plans indexes; RELEASES 2026-07-29; plan moved to `complete/IMAGE_PLAN_COMPLETE.md`.

### Progress Summary (2026-07-29)

| Phase | Status | Notes |
| ----- | ------ | ----- |
| 0 Environment & CMake | Done | MagickWand/zlib, config, launch/landing, deps |
| 1 Core image processing | Done | `src/reporting/*`, 4 helper Unity suites |
| 2 API wiring | Done | Route + JSON/auth lists + thin `src/api/reporting/` |
| 3 Blackbox Test 27 | Done | 14/14 cases, SQLite JWT, sample images |
| 4 Unity expansion | Done | Handler + service + helper expansion |
| 5 SVG & edge cases | Done | density, EXIF, first frame, strip, flatten |
| 6 Performance & limits | Done | Magick resource caps, 413 input/output, blackbox |
| 7 Documentation & closeout | Done | API docs, indexes, RELEASES, plan → complete/ |

**In tree (complete):**

- Config: `config_reporting.c/.h`, defaults, load/cleanup, `AppConfig.reporting`
- Launch/landing: `launch_reporting.c`, `landing_reporting.c`, registry hooks
- Core: `src/reporting/reporting_service.*`, `helpers/*`, `image_scale/*`
- API surface: `src/api/reporting/reporting_service.h`, `image_scale/image_scale.h`
- Routing: `api_service.c` — `reporting/image_scale` route, JSON + JWT middleware lists
- Shared: `utils_base64_decode` / `utils_base64_char_value` in `utils_crypto`
- CMake: init + coverage + **unity** MagickWand/zlib CFLAGS
- Unity: helpers + `reporting_service_test` + `image_scale_test_handle_reporting_image_scale_request`
- Swagger: `POST /reporting/image_scale` in `payloads/swagger.json`
- Blackbox: `tests/test_27_reporting_image_scale.sh`, config, sample images
- Docs: `docs/H/api/reporting/reporting_endpoints.md`, Test 27 md, indexes, RELEASES

### Assessment (2026-07-28) — Initial plan feasibility

**Verdict: Within abilities to implement completely.**

**Environment verified:**

- ImageMagick 7.1.1-47 Q16-HDRI is installed (runtime + dev headers via `ImageMagick-devel`)
- pkg-config finds `MagickWand-7.Q16HDRI` and `MagickWand`
- zlib is installed (`zlib-ng-compat-devel`), pkg-config finds `zlib`
- CMake structure understood: `cmake/CMakeLists-init.cmake` and `cmake/CMakeLists-coverage.cmake`
- Source auto-discovery via `GLOB_RECURSE` — no CMake changes needed for new `.c` files
- Library linking and include paths must be added to CMake (see Open Question 12)

**Codebase patterns verified:**

- Config system: `config_forward.h` → `config.h` → `config_defaults.c` → `config.c` (pattern matches `config_scripting.*`)
- API routing: `handle_api_request()` in `api_service.c` with `json_endpoints[]` and `protected_endpoints[]` arrays
- Launch/landing: `launch_<subsystem>_subsystem()` in `launch.c`, `land_<subsystem>_subsystem` function pointer in `landing.c`, `SR_*` constant in `globals.h`
- Dependency check: `lib_configs[]` array in `utils_dependency.c`
- Unity tests: auto-discovered via `GLOB_RECURSE` in `tests/unity/src/*_test*.c`

**Risks identified (Phase 0 era — largely closed):**

1. CMake modification constraint — waived and completed (init + coverage; unity added in Phase 1)
2. Phase 0 critical path — completed successfully
3. launch/landing reporting files + SR_REPORTING — completed in Phase 0

### Phase 2 Assessment (2026-07-29) — Completed

**Verdict: Complete. Stage gate 2 passed.**

Delivered:

1. `src/api/reporting/reporting_service.h` — swagger service tag
2. `src/api/reporting/image_scale/image_scale.h` — swagger + handler prototype (required for generator handler grep)
3. `api_service.c` — include, route, `json_endpoints[]`, `protected_endpoints[]`, registration log
4. `swagger-generate.sh` → `POST /reporting/image_scale` under Reporting Service with bearerAuth
5. `mkt` / `mkp` clean

Remaining open for later phases: #15 POST size ceiling before large Test 27 cases.

---

## Phase 2 — Lessons Learned

### 1. swagger-generate requires a visible handler prototype

Scanning only looks for `handle_.*_${endpoint_name}` in the endpoint `.h` under `src/api/`. A header that only `#include`s the core prototype (or only has annotations) is skipped silently — tag appears, path does not. Put the `enum MHD_Result handle_reporting_image_scale_request(...)` declaration in the thin API header.

### 2. Avoid `*/` inside block comments (cppcheck)

A comment like `scans src/api/*/` terminates the C block comment early and yields `Unmatched ')'` / syntaxError from cppcheck. Rephrase (e.g. "src/api service directories").

### 3. JSON middleware + handler both call api_buffer_post_data

When path is in `json_endpoints[]`, middleware buffers first; on the final callback the handler's `api_buffer_post_data` sees non-NULL `con_cls` and returns `API_BUFFER_COMPLETE` with the same buffer. Do not free the buffer in middleware before the handler runs.

### 4. JWT for reporting is middleware-only

Unlike mailrelay (handler-side auth, not on `protected_endpoints[]`), reporting relies on `protected_endpoints[]`. Do not omit the list entry assuming the handler will check JWT.

---

## Phase 3 — Lessons Learned

### 1. Align MaxInputBytes with API_MAX_POST_SIZE for blackbox

Default `MaxInputBytes` (50 MB) exceeds `API_MAX_POST_SIZE` (10 MiB). Test config must use ≤10 MiB or large BMP posts fail at the buffer layer with a generic error before reporting limits apply. 1920x1080 uncompressed BMP base64 is ~8.3 MiB — OK under 10 MiB.

### 2. Build large JSON bodies via files, not shell args

`jq --arg image "$huge_b64"` hits ARG_MAX. Use `jq --rawfile image path.b64` and `curl -d @body.json` so multi-megabyte payloads stay on disk.

### 3. Point-units test: inches are not points

Plan text “2x2 inches at 300 DPI → 600x600” means `width=144, height=144, units=pt, dpi=300` (144 pt = 2 in), not `width=2`.

### 4. XO header check is endian-sensitive

Verify width/height as big-endian uint32 (`00000020` for 32). Decode response base64 to a temp file and `od` the header.

### 5. Single SQLite + demo JWT is enough for Test 27

No multi-engine matrix required for image_scale. Pattern: `hydrogen_test_27_reporting.json` with SQLite hydrodemo, login as `Acuranzo` + demo credentials, then exercise the endpoint. Auth case is no-JWT → 401.

### 6. AllowedFormats enable unsupported-format testing

Set `AllowedFormats` to an explicit allow-list (omit `gif`) so “unsupported format” is a real 400 path, distinct from unknown Magick coder failures.

---

## Phase 4 — Lessons Learned

### 1. Helper Unity already existed from Phase 1

Phase 4 main gap was the endpoint handler and service lifecycle, not rewriting helper suites. Expand helpers only for uncovered lines (fallback Magick name, `format_table_has`, wand-based XO encode).

### 2. MHD two-call POST pattern in Unity

Mirror mailrelay: first call with body (`upload_data_size > 0`) returns `MHD_YES` while buffering; second call with empty body and size 0 completes and processes. Always `api_free_post_buffer` afterward.

### 3. Do not redefine global Unity mocks

`USE_MOCK_LIBMICROHTTPD` / `USE_MOCK_SYSTEM` are CMake-global. Including `mock_system.h` without needing controls is fine only if symbols exist; prefer MHD mock only when system mock APIs are unused.

### 4. Tiny embedded PNG is enough for success paths

A 1×1 PNG base64 plus real MagickWand covers PNG and XO success without fixtures on disk. Validation paths need no Magick at all once config is enabled.

### 5. init reporting_service in setUp for Magick paths

Handler success and wand XO tests require `reporting_service_init()` / `cleanup()` around each test so MagickWand genesis is active.

---

## Phase 5 — Lessons Learned

### 1. Density alone is not enough for SVG

Set both `MagickSetOption(wand, "density", …)` and `MagickSetResolution()` before `MagickReadImageBlob` so SVG rasterization honors `dpi`.

### 2. Flatten before format encode

Call `MagickMergeImageLayers(FlattenLayer)` with a white background while alpha is still present, then `MagickSetImageFormat` to JPEG. Flattening after format set is less reliable.

### 3. MagickStripImage is web-only

Strip ICC/EXIF for jpg/png/webp/gif/ico; skip bmp/svg/xo so intermediate and print-oriented paths keep metadata behavior predictable.

### 4. Multi-frame: GetImage + Clear + AddImage

`MagickGetNumberImages` > 1 → extract first via `MagickGetImage`, `ClearMagickWand`, `MagickAddImage`. Avoid leaving the iterator on a discarded frame.

### 5. Blackbox alpha check via identify %A

After JPEG round-trip, `identify -format '%A'` should be `False` or `Undefined` (not `True`).

---

## Phase 6 — Lessons Learned

### 1. MaxInputBytes must be under API_MAX_POST_SIZE for blackbox 413

If test MaxInputBytes equals 10 MiB, a payload that exceeds it also hits `api_buffer_post_data` → `API_BUFFER_ERROR` (HTTP 100/connection abort). Use MaxInputBytes=9e6 and a 9.5e6 base64 body so the handler returns 413.

### 2. 413 for size, 400 for dimensions

`MHD_HTTP_CONTENT_TOO_LARGE` for MaxInputBytes/MaxOutputBytes; keep 400 for MaxImageSize dimension rejects (not a payload-size problem).

### 3. MaxOutputBytes applies to base64 string length

Match MaxInputBytes semantics (base64 field size), not raw Magick blob bytes. Check after encode for both XO and raster paths.

### 4. MagickSetResourceLimit after Genesis

Call `reporting_service_apply_resource_limits()` immediately after `MagickWandGenesis()` and again on double-init so config-driven MaxImageSize can refresh width/height/area caps.

### 5. size_t vs int for limit compares

Use `(size_t)MaxInputBytes` comparisons, not `(int)strlen` — large base64 lengths can exceed INT_MAX edge cases and signed truncation.

---

## Phase 0 — Lessons Learned

### 1. LaunchReadiness struct field name

The `LaunchReadiness` struct (defined in `src/state/state_types.h`) uses `subsystem`, not `name`. Always check the struct definition before writing readiness check functions. The pattern is:

```c
LaunchReadiness readiness = {0};
readiness.subsystem = SR_REPORTING;
readiness.ready = true;
readiness.messages = NULL;
```

### 2. MagickWand version function signature mismatch

`MagickGetVersion` returns `unsigned long`, not `const char*`. The generic version-checking code in `utils_dependency.c` calls version functions as `const char *(*void_func)(void)`, which causes a segfault when applied to `MagickGetVersion`. Always add a special case for functions with non-standard signatures, similar to how OpenSSL, libtar, and libbrotlidec are handled.

### 3. CMake CFLAGS must be added in two places

MagickWand and zlib CFLAGS must be added to both `cmake/CMakeLists-init.cmake` (in `hydrogen_add_executable_target()`) and `cmake/CMakeLists-coverage.cmake` (in the coverage object file loop). Missing either causes compilation failures for coverage builds.

### 4. SETUP.md was pre-updated

The `docs/H/SETUP.md` file already contained `libmagickwand-dev` and `zlib1g-dev` in the Ubuntu build environment, apps.json, and runtime dependencies sections. No documentation changes were needed for Phase 0.

### 5. test_14_library_dependencies.sh was pre-updated

The test script already had `check_dependency_log "MagickWand"` and `check_dependency_log "zlib"` at lines 231-232. Only the C-side `lib_configs[]` array needed updating.

### 6. Shutdown test failure is pre-existing

The `mkt` shutdown test fails with no clear error message. This is not caused by Phase 0 changes — the Reporting subsystem is disabled by default and returns success (1) from both launch and landing functions when disabled.

### 7. Source file discovery is fully automatic

New `.c` files under `src/` are automatically picked up by `file(GLOB_RECURSE HYDROGEN_SOURCES "../src/*.c")` in `CMakeLists-init.cmake`. No CMake changes are needed for source discovery — only for library linking and include paths.

---

## Phase 1 — Lessons Learned

### 1. MagickWand headers trip `-Wswitch-enum` under `-Werror`

Including `<MagickWand/MagickWand.h>` pulls MagickCore inline headers (e.g. `pixel-accessor.h`) with incomplete `switch` over channel enums. Wrap Magick includes with:

```c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <MagickWand/MagickWand.h>
#pragma GCC diagnostic pop
```

### 2. Unity CMake needs MagickWand/zlib CFLAGS too

Phase 0 added CFLAGS to `CMakeLists-init.cmake` and `CMakeLists-coverage.cmake` only. Unity object compilation is a third path in `CMakeLists-unity.cmake` (source objects, test objects, and mock object libs). Without `${MAGICKWAND_CFLAGS}` / `${ZLIB_CFLAGS}`, Unity builds fail with `MagickWand/MagickWand.h: No such file or directory`.

### 3. Prefer shared crypto base64; add standard decode

`utils_base64_encode` already existed; standard `utils_base64_decode` did not (only base64url). Reporting `base64_utils` wraps shared encode/decode and adds `data:` URI stripping. Avoid duplicating alphabet tables in reporting.

### 4. Module lives under `src/reporting/`, API under `src/api/` later

Swagger discovery scans `src/api/*/`. Phase 1 placed the service and endpoint under `src/reporting/` for subsystem ownership (like mailrelay core vs API). Phase 2 must either move/copy swagger headers under `src/api/reporting/` or extend swagger-generate.sh — do not assume `src/reporting/` is auto-discovered for OpenAPI.

### 5. Capture input dimensions before resize

`scale_image_core` must return pre-scale width/height/format via out-params; after `MagickResizeImage` the wand only has output size.

### 6. CMake CFLAGS are needed in three places, not two

Phase 0 lesson #3 said init + coverage. Phase 1 proved **unity** is a third compile path (`CMakeLists-unity.cmake` source objects, test objects, and mock object libs). Any new pkg-config CFLAGS must be mirrored in all three.

### 7. API surface vs subsystem core (mailrelay pattern)

Mailrelay keeps HTTP under `src/api/mailrelay/` and core under `src/mailrelay/`. Reporting Phase 1 put both core and handler under `src/reporting/`. That is fine for implementation, but OpenAPI generation only walks `src/api/`. Phase 2 should introduce a thin `src/api/reporting/` annotation/declaration layer rather than moving all of `src/reporting/` or special-casing swagger-generate without need.
