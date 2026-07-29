# Reporting Service Endpoints

## Overview

The Reporting service provides image processing for reporting pipelines. The first
endpoint is `image_scale`: callers send a base64-encoded image plus target format
and dimensions; Hydrogen decodes, scales, and re-encodes via ImageMagick
(MagickWand), and returns base64 output.

A novel output format **XO** (PDF XObject data part) packs FlateDecode-compressed
pixel and optional alpha streams for downstream PDF builders.

Implementation plan (complete): [IMAGE_PLAN](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md).

## Base URL

```text
http://your-server:port/api/reporting/image_scale
```

The API prefix is configurable (`API.Prefix`, default `/api`).

## Configuration

Reporting is **disabled by default**. Enable it under the `Reporting` section:

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

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `Enabled` | bool | `false` | Master switch for the Reporting subsystem |
| `MaxImageSize` | int | `8192` | Maximum output width or height in pixels |
| `MaxInputBytes` | int | `52428800` | Maximum length of the request `image` base64 string (50 MB) |
| `MaxOutputBytes` | int | `52428800` | Maximum length of the response `image` base64 string (50 MB) |
| `DefaultDPI` | int | `72` | Default DPI when request omits `dpi` (pt→px conversion) |
| `AllowedFormats` | string | `null` | Comma-separated allow-list; `null`/omit allows all Magick-supported formats |

### POST size vs MaxInputBytes

The JSON middleware buffers POST bodies up to `API_MAX_POST_SIZE` (10 MiB). If
`MaxInputBytes` is larger than that ceiling, oversized payloads fail at the buffer
layer before Reporting limits apply. For production large-image workloads, either
raise the API buffer or lower `MaxInputBytes` so handler 413 paths are reachable.
Blackbox Test 27 uses `MaxInputBytes` / `MaxOutputBytes` of 9e6 under that ceiling.

Build dependency: ImageMagick MagickWand 7 (Q16 HDRI) and zlib. See
[SETUP.md](/docs/H/SETUP.md).

## Authentication

`POST /api/reporting/image_scale` requires a JWT (`Authorization: Bearer <token>`).
Auth is enforced by API middleware (`protected_endpoints[]`); the handler does not
re-validate JWT.

Without a token: HTTP **401**. With Reporting disabled: HTTP **503**.

## Request

```bash
curl -X POST http://localhost:5000/api/reporting/image_scale \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <jwt>" \
  -d '{
    "image": "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAA...",
    "format": "png",
    "width": 200,
    "height": 200,
    "units": "px",
    "dpi": 300,
    "scale_algorithm": "lanczos"
  }'
```

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `image` | string | yes | — | Base64 image data; optional `data:` URI prefix |
| `format` | string | yes | — | Output format: `jpg`, `png`, `bmp`, `svg`, `ico`, `webp`, `xo`, or other Magick coders |
| `width` | int | yes | — | Output width in `units` |
| `height` | int | yes | — | Output height in `units` |
| `units` | string | no | `px` | `px` (pixels) or `pt` (points, 1/72 inch) |
| `dpi` | int | no | config `DefaultDPI` | DPI for pt→px: `px = pt * dpi / 72` |
| `scale_algorithm` | string | no | `lanczos` | `nearest`, `bilinear`, `bicubic`, `lanczos`, `mitchell` |

## Response (success)

HTTP **200**:

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

| Field | Description |
|-------|-------------|
| `image` | Base64-encoded output (no `data:` prefix) |
| `mime_type` | MIME for the output format (`application/x-xo` for XO) |
| `input_format` | Detected input MIME/format from Magick |
| `input_dimensions` | Source width/height before scale |

## Response (error)

```json
{
  "success": false,
  "error": "Invalid image data",
  "details": "Base64 decode failed: ..."
}
```

| HTTP | Typical trigger |
|------|-----------------|
| 400 | Missing/invalid params, bad base64/image, dimension over `MaxImageSize`, format not allowed |
| 401 | Missing/invalid JWT (middleware) |
| 405 | Non-POST method |
| 413 | `image` base64 longer than `MaxInputBytes`; decoded blob too large; output base64 longer than `MaxOutputBytes` |
| 500 | Magick/encode failure |
| 503 | `Reporting.Enabled` is false |

## Supported formats

### Input

Any format MagickWand can decode (BMP, PNG, JPEG, GIF, TIFF, WEBP, SVG, ICO, …).
Format is auto-detected from the blob.

### Output

| Format | MIME | Notes |
|--------|------|-------|
| `jpg` | `image/jpeg` | Alpha flattened onto white before encode |
| `png` | `image/png` | Alpha preserved when present |
| `bmp` | `image/bmp` | Alpha flattened onto white |
| `svg` | `image/svg+xml` | Viewport scaling via Magick SVG coder |
| `ico` | `image/x-icon` | First frame / single resolution path |
| `webp` | `image/webp` | Alpha preserved when present |
| `xo` | `application/x-xo` | PDF XObject intermediate (see below) |

Additional Magick coders (e.g. `tiff`, `gif`) work when not blocked by `AllowedFormats`.

## Processing behavior

- **SVG input**: Density and resolution are set from `dpi` before read so rasterization honors DPI.
- **EXIF**: `MagickAutoOrientImage` runs after read.
- **Multi-frame** (animated GIF/WebP, multi-res ICO): **first frame only**.
- **ICC/EXIF strip**: Applied for web formats (`jpg`, `png`, `webp`, `gif`, `ico`); not for `bmp` / `svg` / `xo`.
- **JPEG/BMP transparency**: Flatten onto white background before encode.
- **Resource limits** (on Reporting init): Magick memory/map 512 MiB; width/height/area from `MaxImageSize`; operation time 60 s.

## XO format (PDF XObject data)

Base64-decoded binary layout:

```text
[4 bytes: width (big-endian uint32)]
[4 bytes: height (big-endian uint32)]
[1 byte:  color_space  (0=Gray, 1=RGB, 2=CMYK)]
[1 byte:  has_alpha    (0 or 1)]
[1 byte:  reserved     (0)]
[1 byte:  reserved     (0)]
[N bytes: pixel data]              — zlib/deflate (FlateDecode)
[N bytes: alpha data (if has_alpha)] — zlib/deflate (FlateDecode)
```

Uncompressed pixel size is `width * height * channels` (1/3/4). Alpha is
`width * height` bytes. Pixel order is top-to-bottom, left-to-right (PDF default).
PDF builders insert the compressed streams into Image XObjects without re-compressing.

## Point units example

Two inches square at 300 DPI → 600×600 px:

```json
{
  "image": "<base64>",
  "format": "png",
  "width": 144,
  "height": 144,
  "units": "pt",
  "dpi": 300
}
```

(`144 pt = 2 in`; do not send `width: 2` for inches.)

## Swagger

Interactive docs: `/swagger/`. Operation appears under **Reporting Service** with
`bearerAuth`. Annotations live in thin headers under
`src/api/reporting/`; implementation is under `src/reporting/`.

## Related

- Blackbox: [test_27_reporting_image_scale](/docs/H/tests/test_27_reporting_image_scale.md)
- Plan: [IMAGE_PLAN_COMPLETE](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md)
- [API Overview](/docs/H/core/API_OVERVIEW.md)
