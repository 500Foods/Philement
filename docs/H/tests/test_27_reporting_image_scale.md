# Test 27: Reporting Image Scale Endpoint

## Overview

The [`test_27_reporting_image_scale.sh`](/elements/001-hydrogen/hydrogen/tests/test_27_reporting_image_scale.sh)
script exercises `POST /api/reporting/image_scale` against a live Hydrogen server
with ImageMagick, JWT auth (SQLite demo), and sample images under
`tests/artifacts/images/`.

## Purpose

- Format conversion and scaling (PNG, BMP, JPEG, WebP, SVG, XO)
- Point units and DPI conversion
- Scale algorithms
- Auth required (401 without JWT)
- Validation and limit errors (400 / 413)
- Alpha flatten on JPEG; SVG density path

## Test configuration

| Field | Value |
|-------|-------|
| Test name | Reporting Image Scale |
| Abbreviation | RIS |
| Number | 27 |
| Version | 1.2.0 |
| Config | [`hydrogen_test_27_reporting.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_27_reporting.json) |
| Port | 5270 (`5` + `27` + `0`) |

Reporting is enabled with an explicit `AllowedFormats` list (gif omitted so
unsupported-format returns 400). `MaxInputBytes` / `MaxOutputBytes` are 9e6 so
handler 413 is reachable under `API_MAX_POST_SIZE` (10 MiB).

## Artifacts

| File | Role |
|------|------|
| `sample.png` | PNG→PNG, algorithms, alpha→JPEG |
| `sample.bmp` | Large BMP→PNG |
| `sample.jpg` | JPEG→WebP |
| `sample.webp` | WebP coverage (artifacts) |
| `sample.ico` | ICO coverage (artifacts) |
| `sample.svg` | SVG→PNG |

## Image scale cases

1. PNG to PNG (128×128)
2. BMP to PNG (200×200)
3. JPEG to WebP
4. Point units (144 pt @ 300 DPI → 600×600)
5. XO format (header width/height big-endian)
6. Invalid image data → 400
7. Missing `format` → 400
8. Invalid format → 400
9. Unsupported format (`gif` not in allow-list) → 400
10. No JWT → 401
11. Scale algorithms: nearest, bilinear, bicubic, lanczos
12. SVG input → PNG
13. PNG with alpha → JPEG (identify alpha false/undefined)
14. Oversized dimensions → 400
15. Oversized input base64 → 413

## Flow

1. Validate config and image artifacts
2. Locate Hydrogen binary; start with test config
3. Wait for HTTP ready and READY FOR REQUESTS
4. Acquire JWT via `/api/auth/login` (demo SQLite user)
5. Run image scale cases
6. Stop server

## Related

- API docs: [reporting_endpoints.md](/docs/H/api/reporting/reporting_endpoints.md)
- Plan: [IMAGE_PLAN_COMPLETE](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md)
