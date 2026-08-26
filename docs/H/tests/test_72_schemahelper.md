# Test 72: SchemaHelper Integration

## Overview

Test 72 validates the SchemaHelper interactive TUI front-end and its
underlying queue/packet/apply/connect modules against checked-in fixture
data (no live database required). It exercises the full module stack:
`schemahelper_queue`, `schemahelper_packet`, `schemahelper_apply`,
`schemahelper_connect`, and `schemahelper_qutil`.

## Purpose

- **Module coverage**: Verify all Lua submodule functions exist and operate
  correctly on fixture JSON artifacts.
- **Queue build**: Validate migration totals (total, perfect, accepted,
  subject) and per-field finding IDs (e.g. `meta:drift:1148:1003:name`).
- **Packet workflow**: Test `--ref` collision detection and packet directory
  creation (MANIFEST.json, PACKET.md, FINDING.json, DETAIL.txt, SUGGESTED.sql).
- **Apply logic**: Validate refuse_reason / can_apply / confirm_token /
  build_sql / build_catalog_sql logic for metadata, orphan, and catalog findings.
- **Connect probe**: Verify wrapper CLI flag parsing and resolve() for
  SQLite (env export and exec-line parsing).
- **Decode embedded**: Confirm brotli/base64 decoding works across all
  dialects (SQLite, MySQL upper+lower, DB2, PostgreSQL).
- **Luacheck**: All 20 `schemahelper*.lua` files must pass luacheck clean.
- **CLI sanity**: `schemahelper.sh --help` / `--version` and
  `schemahelper.lua --version` produce correct output.
- **Renderer**: Dashboard content renders without error against the fixture
  (headless terminal stubs).

## Test Configuration

- **Test Name**: SchemaHelper
- **Test Abbreviation**: SCH
- **Test Number**: 72
- **Version**: 1.1.0

## What It Does

1. **Fixture verification**: Ensures all required fixture files exist in
   `extras/schematool/test/fixtures/sample_project/` (findings.json,
   catalog_findings.json, state sidecar, migrations, detail files,
   wrapper script).

2. **Lua availability**: Confirms Lua 5.5 and `terminal.lua` are available.

3. **Queue module**: Validates `build`, `load_state`, `create_state`,
   `default_state_path`, `artifacts_present`, `load_metadata`,
   `load_detail_section`, `note_for`, `explore_lines`, `save_decision`,
   `save_cursor`, `jq_update_state`, `build_dashboard_lines`,
   `build_review_lines`, `find_finding`, `json_subobj`, `payload_text`.

4. **Decision persistence**: Verifies that `save_decision` + `save_cursor`
   round-trips through jq and that accepted findings persist across a
   rebuild of the queue.

5. **Pure module checks**: Runs luacheck-clean assertions on `qutil`,
   `packet`, `apply`, and `connect` submodules (function existence +
   behavioral checks).

6. **Decode patterns**: Tests `decode_embedded` and `has_embed` across
   all engine-specific brotli/base64 wrapper patterns.

7. **CLI/version**: Runs `schemahelper.sh --help` / `--version` and
   `schemahelper.lua --version`.

8. **Terminal dependency**: Loads `terminal.ui.panel.text` (TextPanel)
   from the terminal.lua rock.

9. **Dashboard renderer**: Renders `dashboard_content` against the fixture
   with headless terminal stubs (no TTY required).

## Fixture Layout

```directory
extras/schematool/test/fixtures/sample_project/
  findings.json          — SchemaTool metadata audit output (copy of what --work-dir produces)
  catalog_findings.json  — SchemaTool catalog audit output
  schemahelper_acuranzo_sqlite.json  — state sidecar (cursor + decisions)
  migrations/
    design_1000.lua      — perfect migration
    design_0100.lua
    design_1148.lua      — known drift (meta:drift:1148:1003:name)
    design_1200.lua
    design_1290.lua      — orphan ref 1290
  schemas/queries.sql
  finding_detail_meta_drift_1148.txt
  catalog_finding_detail_nullable.txt
  schematool_sqlite_fixture.sh  — wrapper script (env exports)
```

## Work-Dir / Out-Dir Contract

The smoke test (`smoke_schemahelper_queue.sh`) and this integration test
both pass the fixture directory as `--work-dir` (where JSON intermediates
reside) and `--out-dir` (where the state sidecar lives). In the real
SchemaHelper TUI:

- **`--work-dir`** (auto-generated `/tmp/schemahelper-<timestamp>-<rand>`):
  holds `findings.json`, `catalog_findings.json`, `catalog_expected.json`,
  detail text files, and SchemaTool log/exit files. Cleaned up on exit
  unless `--keep-work-dir` is passed.

- **`--out-dir`** (default: wrapper directory): holds the state sidecar
  (`schemahelper_<design>_<engine>.json`), final `.sql` remediation, and
  `.mig` orphan capture. Persists across sessions for `--reuse`.

## Output Structure

The smoke test writes packets to a `mktemp -d /tmp/schemahelper_smoke_XXXXXX`
directory (or `--packet-dir DIR`). Packet directories follow the pattern:

```text
<packet-dir>/schemahelper_acuranzo_sqlite_1291/
  MANIFEST.json
  PACKET.md
  FINDING.json
  DETAIL.txt
  SUGGESTED.sql
```

## Dependencies

- Lua 5.5 with `terminal.lua` rock and `lua-brotli` C module
- `jq`, `shellcheck`, `luacheck`
- SchemaTool Lua scripts under `extras/schematool/lua/`
- Checked-in fixtures under `extras/schematool/test/fixtures/sample_project/`

## Integration

```bash
# Run this specific test
./tests/test_72_schemahelper.sh

# Or via the full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](/docs/H/tests/TESTING.md)
- [SchemaHelper Tool Guide](/docs/H/tools/SCHEMAHELPER.md)
- [SchemaTool Tool Guide](/docs/H/tools/SCHEMATOOL.md)
