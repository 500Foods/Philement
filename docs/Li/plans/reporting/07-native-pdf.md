<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Native PDF

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 68–71
**Stop when:** Charts and barcodes in the PDF are vectors.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 67 in [`/docs/Li/plans/reporting/06-graphics.md`](/docs/Li/plans/reporting/06-graphics.md) is complete. Phases 69 and 70 follow their own entry gates.
**Next file:** None. The backlog and the Working Log stay in the parent.

## Phase 68 — Native PDF contract

### Goal

Agree the payload lua-zint and d3.lua will return, before either
port grows a PDF writer. No port code in this phase.

### Entry gate

Phase 67 Status complete. Phase 54 Status complete. The barcode SVG
and the chart SVG are what this payload replaces.

### Work items

- [ ] 68.1 Agree the payload both ports will return: a bounding box,
       the resources it needs, and a content stream that lua-pdfkit
       can place. Pin the entry name in the contract note. Do not
       guess the table in this plan. Ask when a field is still open.
- [ ] 68.2 Andrew approves that note. No migration is written.

### Done means

The contract note names the payload Andrew approved.

### Exit gate

Andrew's explicit approval. No source edited.

### Status

**not started**

---

## Phase 69 — Zint PDF payload

### Goal

lua-zint returns the Phase 68 payload for a Code 128 and a QR code.

### Entry gate

Phase 68 Status complete. The work is in the zint repository.

### Work items

- [ ] 69.1 Extend lua-zint so a Code 128 probe and a QR probe return
       the Phase 68 payload. Record the commit.
- [ ] 69.2 No Helium packet in this phase. Phase 71 ships the
       modules.

### Done means

Andrew reports the two zint probes from the port's tests, and each
probe matches the Phase 68 payload.

### Exit gate

The port probe. No migration is applied in this phase.

### Status

**not started**

---

## Phase 70 — D3 PDF payload

### Goal

d3.lua returns the Phase 68 payload for the port's reference chart.

### Entry gate

Phase 68 and Phase 56 Status complete. The work is in the d3
repository. This phase may run before Phase 69.

### Work items

- [ ] 70.1 Implement the chart PDF emit against the Phase 68 payload,
       including the gradient and the standard face the demo chart
       uses.
- [ ] 70.2 Probe with the port's reference chart. Record the commit.
       No Helium packet in this phase.

### Done means

Andrew reports the d3 probe payload from the port's tests.

### Exit gate

The port probe. No migration is applied in this phase.

### Status

**not started**

---

## Phase 71 — Place native PDF objects

### Goal

`stage=pdf` places the payloads from Phases 69 and 70. Charts and
barcodes in the PDF are vectors.

### Entry gate

Phases 31, 55, 65, 69, and 70 are complete.

### Work items

- [ ] 71.1 Extend lua-pdfkit where the payload needs a placeable
       vector resource, a matrix, and the shared resources the two
       payloads reference. Record the commit. Ship the changed
       modules in a new migration. Do not edit an applied seed in
       place.
- [ ] 71.2 Amend `Reports.Render`. Chart and barcode items in
       `stage=pdf` call the native emits and place the objects.
       `params.rasters` is not required for these items. The SVG
       paths keep their SVG from the same compose.
- [ ] 71.3 A chart fixture and a barcode fixture, rendered as PDFs.
       Andrew opens each one. Axis text is selectable. Bars stay
       crisp when the page is zoomed.
- [ ] 71.4 Record the port commits the packet pins.

### Done means

A chart PDF and a barcode PDF open as vectors the ports emitted.

### Exit gate

Applied script update, the fixture PDFs, and Andrew's confirmation
that the objects are vectors.

### Status

**not started**

