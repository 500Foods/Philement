<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Graphics

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 50–67
**Stop when:** The author can place images, barcodes, and charts, and can export SVG and PNG.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 49 in [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md) is complete. Phase 57 may run alongside earlier phases, as its entry gate says.
**Next file:** [`/docs/Li/plans/reporting/07-native-pdf.md`](/docs/Li/plans/reporting/07-native-pdf.md) after Phase 67.

## Phase 50 — Embedded image

### Goal

An embedded image shows in the designer and in the HTML preview.

### Entry gate

Phase 49 Status complete.

### Work items

- [ ] 50.1 Palette shows lookup 053 keys 2 (Static Image) and 22
       (Field Image). A new item stores `dpi` `72`. `format` is
       `png`, or `jpeg` when the stored bytes are JPEG. Reuse is the
       rule, not a switch. There is no downsample-cap property.
- [ ] 50.2 The image browser loads a local file or takes a clipboard
       paste. A designer panel shows the suggested pixel size from
       the physical box and the item dpi, and lets the author change
       the width and the height. Each side is at least 1 pixel. A
       side that rounds to zero, or that the author sets below 1,
       is kept at 1. The panel states that confirming stores those
       pixels and does not keep the original file.
       Confirm calls `image_scale` unless the chosen size is the
       source's own pixel size, in which case the original bytes are
       stored. Embedded bytes dedupe into the definition's
       content-hash registry. Changing the box later does not
       resample.
- [ ] 50.3 `stage=html` repeats the data URI of the stored bytes in
       each placement. DOMPurify keeps that data URI. The script
       does not call `image_scale`.
- [ ] 50.4 Browser: the panel shows a suggested size, an edited size
       is what gets stored, and the same PNG on two detail rows
       shows in both rows of the preview, at the box size.

### Done means

The preview shows the same embedded PNG on two detail rows.

### Exit gate

The script update applied, lint, and the browser check.

### Status

**not started**

---

## Phase 51 — Reference and URL images

### Goal

A reference image and a URL image each resolve to bytes and render.

### Entry gate

Phase 50 Status complete. The Phase 17 document QueryRef is applied.

### Work items

- [ ] 51.1 The reference picker lists documents through the Phase 17
       read QueryRef. A URL item stores the URL.
- [ ] 51.2 `Reports.Render` resolves both kinds to bytes before any
       item is placed. References use `H.altquery_sync` on
       `params._hydrogen.database` and the Phase 17 QueryRef. A
       missing or unconfigured database name adds a warn entry and
       leaves the box empty. URLs use `H.http.request_sync` against
       that URL, with no Hydrogen JWT. A failed reference or an
       unreachable URL is a warn entry and an empty box.
- [ ] 51.3 Browser: a reference source and a URL source each render
       once their bytes resolve.

### Done means

A reference image and a URL image each render once.

### Exit gate

The script update applied, and the browser check. The Phase 17
document QueryRef was applied before that check.

### Status

**not started**

---

## Phase 52 — PDF image stored once

### Goal

A repeated image is one PDF image object, and each placement
references it.

### Entry gate

Phase 51 Status complete.

### Work items

- [ ] 52.1 `stage=pdf` calls `doc:image` with the same resolved bytes
       for every placement of that image. The port stores one image
       object and each placement references it. JPEG sources stay
       JPEG. Everything else stores as PNG. There is no XO embedder.
       The script does not call `image_scale`.
- [ ] 52.2 Fixture: the same PNG on two detail rows. The PDF contains
       one image object and two drawing references. A second,
       different PNG is a second object. The fixture stays under the
       1 MiB result cap after base64. Say in the Working Log that the
       port registry supplied the reuse.
- [ ] 52.3 Hand the amendment over. After apply, the fixture matches.

### Done means

The repeated PNG is one PDF image object with two drawing references.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 53 — Seed lua-zint

### Goal

Put the pure-Lua zint port into `scripts` so `require` loads it. No
report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–52 may still be open. Phases 54 and 69
wait for the apply.

### Work items

- [ ] 53.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `zint.*` module (`init`, `barcode`,
       `common`, …). `invokable = 0`, `mcp_access = 0`. Group name
       `zint`. The script name is the require path after the first
       dot. One library per file. Reverse deletes only those rows.
       The packet records the commit. If neither the port repository
       nor the task-tree path is present, stop and ask.
- [ ] 53.2 The port writes a file when `opts.output` is set, and the
       sandbox leaves `io` nil. The probe leaves that field unset and
       uses the returned SVG string.
- [ ] 53.3 Hand the packet over. After apply, Andrew runs a probe:
       `require("zint.barcode")` for a Code 128 and a QR code. Each
       probe returns an SVG that contains the data string.
       `AllowDBModuleLoad` is on for that call.

### Done means

Andrew reports a Code 128 SVG and a QR SVG from inside Hydrogen, with
`io` left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 54 — Barcode SVG

### Goal

Static and field barcodes render as SVG in the HTML preview.

### Entry gate

Phase 53 and Phase 25 Status complete.

### Work items

- [ ] 54.1 Palette shows lookup 053 keys 43 (Static Barcode) and 45
       (Field Barcode). A new barcode writes raster `dpi` `300`.
       The item stores `symbology` as one of `qr`, `code39`,
       `code128`, `upca`, `gs1-128`, plus the data string or the
       field name. Other symbologies are refused with a warning.
- [ ] 54.2 Compose calls `zint.barcode` with no `output` path. The
       SVG inlines into the item box in the HTML preview and the
       HTML export.
- [ ] 54.3 DOMPurify is configured so this SVG survives the preview
       sandbox. A stripped SVG is a failed gate.
- [ ] 54.4 Browser: a Code 128 of a known string and a QR of a known
       string appear in the preview. A field barcode follows the
       sample row.

### Done means

The Code 128 and the QR survive the preview sanitizer, and the field
barcode follows the sample row.

### Exit gate

Applied script update, lint, and the browser check.

### Status

**not started**

---

## Phase 55 — Barcode PDF raster

### Goal

Until Phase 71, a barcode in the PDF is a PNG Lithium rasterized and
passed in `params.rasters`.

### Entry gate

Phase 54 and Phase 31 Status complete.

### Work items

- [ ] 55.1 `stage=pdf` embeds a barcode PNG only when Lithium passes
       it in `params.rasters` under the item id. Lithium rasterizes
       the barcode SVG through `image_scale` at the item's dpi
       (default 300). A missing raster omits the symbol and adds a
       warning. The Working Log names this raster path.
- [ ] 55.2 A Code 128 of a known string appears in the PDF.

### Done means

A Code 128 appears in the PDF as a raster.

### Exit gate

The browser check, or Andrew's fixture PDF if the browser cannot
open it. Lint passes.

### Status

**not started**

---

## Phase 56 — Seed d3.lua

### Goal

Put the pure-Lua D3 port into `scripts` so `require` loads it. No
report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–55 may still be open. Phase 57 may
run alongside this one. Phases 59 and 70 wait for the apply.

### Work items

- [ ] 56.1 Copy the module named in the library lock into one
       migration. `invokable = 0`, `mcp_access = 0`. Phase 0 locked
       `require("d3.lua")`: group `d3`, script name `lua`. Reverse
       deletes only those rows. The packet records the commit. If
       neither the port repository nor the task-tree path is present,
       stop and ask.
- [ ] 56.2 The demo writes its SVG with `io.open`, and the sandbox
       leaves `io` nil. Confirm the module returns its SVG as a
       string with no file written.
- [ ] 56.3 The regression oracle is the port's reference pair, the
       D3.js chart and the Lua chart, whose SVGs are byte-identical.
       After apply, Andrew's probe reproduces the reference SVG from
       inside Hydrogen. The probe stops at that SVG.
- [ ] 56.4 Hand the packet over. Per-kind charts wait for Phase 57
       and the draw phases.

### Done means

Andrew reports the reference SVG reproduced byte-for-byte inside
Hydrogen, with `io` left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 57 — Chart design lock

### Goal

Agree the chart contract before the designer and the Lua grow around
it. No source edits.

### Entry gate

Phase 0 Status complete. This phase may run alongside Phases 1–56.
Phase 58 waits for this phase.

### Work items

- [ ] 57.1 Lock the chart kinds for v1: bar, stacked bar, line,
       dual-axis, combinations, and pie. The series-first model in
       Charts is the rule. `kind` stays optional shorthand.
- [ ] 57.2 Lock the options hierarchy: chart options, then one or
       more series. Adding a series picks that series' format.
       Record the option defaults in the Defaults table. Chart `dpi`
       stays 150.
- [ ] 57.3 Lock the four scopes as written in Charts: `band`,
       `group`, `record`, and `custom`.
- [ ] 57.4 Record the arc the port will add. Pie is in v1. This
       phase does not edit the port. Phase 63 adds the arc and
       previews one pie.
- [ ] 57.5 Name the fixtures: one bar, one grouped-scope bar, one
       record-scope bar, one line, one dual-axis line, one stacked
       bar with an overlaid line, and one pie. These are new files.
       Phase 1 did not write them. The Phase 1 set is the static,
       detail, and grouped trio in
       [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).

### Done means

The chart contract in [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md) is the one Andrew approved.

### Exit gate

Andrew's explicit approval. No source edited.

### Status

**not started**

---

## Phase 58 — Charts in the designer

### Goal

Place a chart, add its series, and round-trip the bindings through
the definition file.

### Entry gate

Phase 57 Status complete.

### Work items

- [ ] 58.1 Palette shows lookup 053 keys 42 (Static Chart) and 44
       (Field Chart). The item stores a `chart` object per the locked
       contract, plus its box. A new chart writes `dpi` `150` and the
       option defaults Phase 57 added.
- [ ] 58.2 `chart-panel.js` walks the options hierarchy: the chart's
       options first, then an add-series row. Each series names its
       datasource, field, role, stack, axis, and label. A kind
       shorthand fills the common cases.
- [ ] 58.3 The panel follows the property panel's commit rules.
       Options are designer state until commit. The browser does not
       draw the chart.
- [ ] 58.4 An empty series binding blocks export with a toast.
- [ ] 58.5 Browser: place a chart, add a bar series and a line series
       over the session sample, set the dual axis, export, reload,
       and see the bindings.

### Done means

A chart item round-trips through the definition file with its series
and options.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 59 — Bar chart

### Goal

A band-scope bar chart renders in the HTML preview.

### Entry gate

Phase 19, Phase 56, and Phase 58 Status complete.

### Work items

- [ ] 59.1 Resolve scope `band` as every row of the chart's
       datasource. A `custom` scope warns and draws nothing.
       Phase 64 draws that scope.
- [ ] 59.2 d3.lua draws the bar with the resolved rows, the
       `categoryField`, the series, and the box in points. The
       returned SVG is stored on the intermediate. The intermediate
       does not re-derive the chart.
- [ ] 59.3 The HTML export and the preview inline that SVG.
       DOMPurify keeps it. A stripped SVG is a failed gate.
- [ ] 59.4 One band-scope bar fixture from Phase 57. Hand the
       amendment over. After apply, the fixture matches and the
       preview shows the bar.

### Done means

The band-scope bar fixture matches the expected intermediate and
renders in the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 60 — Group and record chart scopes

### Goal

The same bar draws for a group window and for a record window.

### Entry gate

Phase 59 Status complete.

### Work items

- [ ] 60.1 Scope `group` uses the rows of the current group. Scope
       `record` filters the named datasource on the current record's
       `keyField`. A chart on a detail band draws once per page, on
       the first detail of that page, not once per row.
- [ ] 60.2 One fixture for `group` and one for `record`, using the
       bar from Phase 59.
- [ ] 60.3 Hand the amendment over. After apply, both fixtures match
       and both preview.

### Done means

The group fixture and the record fixture both match and both preview.

### Exit gate

Applied script update, the two fixture calls, and the browser preview.

### Status

**not started**

---

## Phase 61 — Line charts

### Goal

A line chart and a dual-axis line chart render in the preview.

### Entry gate

Phase 60 Status complete.

### Work items

- [ ] 61.1 Draw the Phase 57 line fixture and the dual-axis line
       fixture with the same d3 path Phase 59 uses. Roles `line` on
       one axis, then `line` on two axes.
- [ ] 61.2 Hand the amendment over. After apply, both fixtures match
       and both preview.

### Done means

The line fixture and the dual-axis fixture both match and both
preview.

### Exit gate

Applied script update, the two fixture calls, and the browser preview.

### Status

**not started**

---

## Phase 62 — Combo chart

### Goal

A stacked bar with an overlaid line renders in the preview.

### Entry gate

Phase 61 Status complete.

### Work items

- [ ] 62.1 Draw the Phase 57 combo fixture: bar series sharing a
       `stack`, plus a line series. This is the shape of the port's
       demo chart, drawn with the report's series, not by pasting
       the demo.
- [ ] 62.2 Hand the amendment over. After apply, the fixture matches
       and the preview shows the combo.

### Done means

The combo fixture matches the expected intermediate and renders in
the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 63 — Pie chart

### Goal

Add the pie arc to the port and preview one pie.

### Entry gate

Phase 62 Status complete.

### Work items

- [ ] 63.1 Add the arc in the port's repository, record the commit,
       and preview one pie fixture.
- [ ] 63.2 Hand the script amendment over. After apply, the fixture
       matches and the preview shows the pie.

### Done means

The pie fixture matches the expected intermediate and renders in
the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---
## Phase 64 — Custom chart

### Goal

A chart scope of `custom` plots the rows its function returns.

### Entry gate

Phase 63 Status complete. Phase 33 Status complete.

### Work items

- [ ] 64.1 A `custom` scope names a function whose `returns` is
       `rows`. Compose calls it with the band context, using the
       Phase 33 compiler, and plots the rows it returns.
- [ ] 64.2 The Phase 57 custom fixture renders in the preview. Hand
       the amendment over.

### Done means

The custom-chart fixture renders in the preview from the Phase 33
compiler.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 65 — Chart PDF raster

### Goal

Until Phase 71, a chart in the PDF is a PNG Lithium rasterized and
passed in `params.rasters`.

### Entry gate

Phase 64 Status complete.

### Work items

- [ ] 65.1 Lithium rasterizes the chart SVG through `image_scale` at
       the chart `dpi` (default 150) and passes the PNG in
       `params.rasters` under the item id. A missing raster omits the
       chart and adds a warning. HTML, preview, and SVG keep the
       vector.
- [ ] 65.2 The band-scope bar from Phase 59 appears in the PDF.

### Done means

The band-scope bar appears in the PDF as a raster.

### Exit gate

The browser check, or Andrew's fixture PDF. Lint passes.

### Status

**not started**

---

## Phase 66 — SVG emit

### Goal

`stage=svg` returns one SVG document of the composed pages.

### Entry gate

Phase 65 Status complete. The fixture is the text detail. Chart and
barcode SVG inline from the intermediate.

### Work items

- [ ] 66.1 Amend `Reports.Render` with `stage=svg`. One page per
       section, positions in physical CSS units, the definition's
       font face, and page breaks marked per the Emit contract. A
       label-sized report is this file with the page size set to the
       label and the margins set to zero.
- [ ] 66.2 Chart and barcode items inline their SVG from the
       intermediate. Images inline as data URIs at their scaled size.
- [ ] 66.3 Add the expected SVG skeleton for the detail fixture. Hand
       the packet over. Andrew's call matches, and a browser check
       embeds the SVG and shows the pages.

### Done means

The detail fixture's SVG matches the expected skeleton and embeds in
a page with the page breaks marked.

### Exit gate

Applied script update, the fixture call, and the browser check.

### Status

**not started**

---

## Phase 67 — PNG export

### Goal

The manager turns a `stage=svg` result into one PNG per page.

### Entry gate

Phase 66 Status complete. `Reporting.Enabled` is true on the server
used for the gate.

### Work items

- [ ] 67.1 The export action requests `stage=svg`, then posts each
       page's SVG to `image_scale` with `units=px`, the page box,
       `dpi` 150, and `format=png`. The script does not call that
       endpoint.
- [ ] 67.2 A label downloads as one PNG. More than one page downloads
       as a zip of PNGs. The script result stays the SVG, so the
       1 MiB result cap applies to that SVG.
- [ ] 67.3 Browser: the label fixture downloads one PNG at the
       label's physical size.

### Done means

The label fixture becomes one PNG, rasterized by `image_scale` from
the SVG the script returned.

### Exit gate

Lint and the browser check. Reporting was enabled for that check.

### Status

**not started**

