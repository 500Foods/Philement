# Reports.Render tools

Dev tools for the split `Reports.Render` (loader `acuranzo_1479.lua`, modules
`Reports.Render.<Module>` in `acuranzo_1467.lua` through `acuranzo_1478.lua`).
They run the real Helium generator from [`helium/`](../../helium/). They never
touch a database. They are not a Hydrogen `test_NN` script.

| Tool | Usage | What it does |
| --- | --- | --- |
| [render_check.sh](render_check.sh) | `./render_check.sh [rev] [empty\|error]` | Captures the stored bodies at `rev` (default `HEAD`) and in the working tree, then runs the harness. Exit 1 on any difference |
| [harness.lua](harness.lua) | `lua harness.lua <old_dir> <new_dir> [empty\|error]` | Runs both builds on every fixture in `elements/003-lithium/tests/fixtures/reporting/` plus a synthetic barcode, image, shape, and timezone case, at `stage=compose` and `stage=html`. Prints each case, the modules fetched, and the diff count |
| [capture.lua](capture.lua) | `lua capture.lua <migration.lua> <out_dir>` | Writes the forward `Reports.Render` body exactly as stored (after the runner strips leading spaces) to `<out_dir>/<Module>.lua` or `Render.lua` |
| [sizecheck.lua](sizecheck.lua) | `lua sizecheck.lua <migration.lua> [engine]` | Prints statement count, longest statement, longest literal, and largest raw Render body. Defaults to `db2` |
| [jsoncheck.lua](jsoncheck.lua) | `lua jsoncheck.lua <migration.lua> [...]` | Decodes the DB2 SQL and fails when a `JSON_INGEST` literal is not a JSON object |

Size ceiling for one Render body: under 1000 lines, encoded DB2 literal under
about 24 KB (DB2 rejects a string constant over 32,672 bytes), and raw body
under 60 KB (MariaDB and MySQL `text` holds 65,535 bytes).

`8b524425f^` is the last revision with the single-body `Reports.Render`.
`./render_check.sh 8b524425f^` shows the split matched it: 44 cases, 0 diffs.
