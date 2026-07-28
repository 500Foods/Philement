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

```
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
| `image`           | string | yes      | —         | Base64-encoded image data. May include a `data:` URI prefix.              |
| `format`          | string | yes      | —         | Output format: `jpg`, `png`, `bmp`, `svg`, `ico`, `webp`, `xo`, or others.  |
| `width`           | int    | yes      | —         | Output width in the specified units.                                        |
| `height`          | int    | yes      | —         | Output height in the specified units.                                       |
| `units`           | string | no       | `px`      | Unit of width/height: `px` (pixels) or `pt` (points, 1/72 inch).          |
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

```
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

### Goals

- Create `src/reporting/` directory with the image processing core.
- Implement base64 decode/encode, ImageMagick initialization, format mapping, and the XO encoder.
- All functions non-static (per project rules) with header declarations for Unity testing.

### Directory Structure

```
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

### Tasks

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

### Goals

- Register the endpoint in the API service.
- Add swagger annotations.
- Add to JSON endpoint and auth lists.

### Tasks

1. **Edit `src/api/api_service.c`**:
   - Add `#include "reporting/reporting_service.h"` and `#include "reporting/image_scale/image_scale.h"`.
   - Add route in `handle_api_request()`:
     ```c
     else if (strcmp(path, "reporting/image_scale") == 0) {
         return handle_reporting_image_scale_request(connection, url, method, upload_data,
                                                      upload_data_size, con_cls);
     }
     ```
   - Add `"reporting/image_scale"` to the `json_endpoints[]` array in `api_service_endpoint_expects_json()`.
   - Add `"reporting/image_scale"` to the `protected_endpoints[]` array in `api_service_endpoint_requires_auth()` (reporting endpoints require JWT).
   - Add log line in `register_api_endpoints()` for the new endpoint.

2. **Edit `src/launch/launch.c`**: Add `launch_reporting_subsystem()` call in the launch plan.

3. **Swagger**: Verify `swagger-generate.sh` picks up the new annotations. The script scans `src/api/*/` for service directories and endpoint subdirectories. Since Reporting is under `src/api/reporting/`, it will be discovered automatically.

### Stage Gate 2 — Validation

- **Build**: `zsh -ic 'mkt'` succeeds.
- **Swagger**: Run `swagger-generate.sh` and verify `/reporting/image_scale` appears in `swagger.json`.
- **cppcheck**: `zsh -ic 'mkp'` passes.

---

## Phase 3: Blackbox Test (Test 27)

### Goals

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

### Goals

- Expand Unity unit tests for the reporting module to improve coverage.
- Target high-value functions that are testable in isolation.

### Tasks

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

### Goals

- Handle SVG input and output properly.
- Handle edge cases like animated images, multi-frame images, EXIF orientation.

### Tasks

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

### Goals

- Enforce input/output size limits.
- Add timeout for long-running operations.
- Document performance characteristics.

### Tasks

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

### Goals

- Add documentation for the Reporting service.
- Update all index files.
- Move plan to `complete/` when done.

### Tasks

1. **Create `docs/H/api/reporting/reporting_endpoints.md`**: API documentation for the Reporting service.

2. **Update `docs/H/plans/README.md`**: Add link to `IMAGE_PLAN.md` in the active plans section.

3. **Update `docs/H/INSTRUCTIONS.md`**: Add `tests/test_27_reporting_image_scale.sh` to the test list.

4. **Update `docs/H/tests/TESTING.md`**: Add Test 27 to the test list.

5. **Update `docs/H/SITEMAP.md`**: Add links to new documentation files.

6. **Update `docs/H/STRUCTURE.md`**: Add new source files.

7. **Update `RELEASES.md`**: Note the new Reporting service.

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

### Service-level annotations (`src/reporting/reporting_service.h`):

```c
//@ swagger:title Reporting Service API
//@ swagger:description REST API for image processing and reporting operations.
//@ swagger:version 1.0.0
//@ swagger:tag "Reporting Service" Provides image processing and conversion services.
```

### Endpoint-level annotations (`src/reporting/image_scale/image_scale.h`):

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

| File          | Source          | How to create                                              |
| ------------- | --------------- | ---------------------------------------------------------- |
| `sample.bmp`  | Generated       | Use ImageMagick: `convert -size 1920x1080 xc:red sample.bmp` |
| `sample.jpg`  | Generated       | `convert -size 800x600 plasma: sample.jpg`                 |
| `sample.webp` | Generated       | `convert -size 512x512 plasma: sample.webp`                |
| `sample.ico`  | Generated       | `convert -size 64x64 xc:blue sample.ico`                   |
| `sample.svg`  | Hand-written    | Simple SVG with a rectangle and circle.                    |

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

1. **MagickWand version**: The system may have ImageMagick 6 or 7. The pkg-config name differs (`MagickWand` vs `MagickWand-7.Q16HDRI`). The CMake should try the HDRI variant first, then fall back. The version function for dependency checking is `MagickGetVersion` (ImageMagick 7) or `MagickCoreGetVersion` (ImageMagick 6).

2. **zlib dependency**: The XO encoder uses `compress2()` from zlib for FlateDecode compression. zlib is already available as a transitive dependency through brotli and OpenSSL, but must be added explicitly to `pkg_check_modules` and `HYDROGEN_BASE_LIBS` in CMake. zlib must also be added to `tests/test_14_library_dependencies.sh` and `src/utils/utils_dependency.c`.

3. **SVG rasterization**: SVG input requires a `density` setting before reading. The `dpi` parameter controls this. For SVG output, ImageMagick's SVG coder produces a vector SVG with the scaled viewport.

2. **SVG rasterization**: SVG input requires a `density` setting before reading. The `dpi` parameter controls this. For SVG output, ImageMagick's SVG coder produces a vector SVG with the scaled viewport.

3. **Animated images**: GIF and animated WebP have multiple frames. The initial implementation processes only the first frame. This should be documented.

4. **Color space handling**: When converting from CMYK to RGB (or vice versa), ImageMagick handles the conversion. The XO format reports the color space in its header.

5. **Memory limits**: ImageMagick has its own resource limits. We should set `MAGICK_MEMORY_LIMIT` and `MAGICK_MAP_LIMIT` via `MagickSetResourceLimit()` to prevent memory exhaustion from malicious inputs.

6. **Thread safety**: MagickWand is not thread-safe by default. Each request should create and destroy its own MagickWand instance. The `reporting_service_init()` function should call `MagickWandGenesis()` and `reporting_service_cleanup()` should call `MagickWandTerminus()`.

7. **Error messages**: ImageMagick error messages can be verbose. The endpoint should extract a concise error message from the MagickException.

8. **Base64 line length**: The `data:` URI spec allows base64 with or without line breaks. The decoder should handle both.

9. **JPEG quality**: Currently defaults to ImageMagick's default (92). A future extension could add a `quality` parameter.

10. **ICO multi-resolution**: ICO files can contain multiple resolutions. When reading, ImageMagick picks the first. When writing, a single resolution is produced.

---

## Working Log

- **Phase 0**: Not started.
- **Phase 1**: Not started.
- **Phase 2**: Not started.
- **Phase 3**: Not started.
- **Phase 4**: Not started.
- **Phase 5**: Not started.
- **Phase 6**: Not started.
- **Phase 7**: Not started.
