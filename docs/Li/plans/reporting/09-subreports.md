<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Subreports

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 83–85
**Stop when:** A Subreport element picks a saved child report, binds
parent fields to child parameters, opens the child in its own tab,
and Preview HTML places the child's output in the element box
(fixed clip first; then grow/shrink and page split).

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation.

**Entry:** Element review Phases 72–82 need not all be complete;
Subreport work starts when Andrew names Phase 83. Reuse lookup 053
key **56** (`SubReport`). Child reports are ordinary standalone
reports (they run on their own). **No new band type** — a "package"
of statements is a parent report with its own header and the
existing Statement report as a Subreport in Detail. Lookup 053 key
10 (`Package`) remains a legacy band Kind and is not required for
this design.

**Naming:** **Bind** is confirmed for the parent→child map. It is
**not** Link (Phase 82 URI, 054 key 29).

## Shared design

### Category

Subreport gets its **own palette Category** (new lookup **074**
row; next free key is **8** after Structural/Static/Field/
Calculated/Aggregate/System/Bands/CrossTab). It is **not** under
Bands, Static, or Field. Lookup 053 key 56 (`SubReport`) moves its
`value_int` to that Category when Phase 83 writes the lookup
packet. Leave room for **multiple Subreport Kinds** later under
the same Category. **Do not put Subreport lookups in
`acuranzo_1481.lua`** (Phase 75 is Units/Background/Overflow only).

### Properties

| Property | Role |
| --- | --- |
| Report | Picker of saved reports (child definition) |
| Bind Datasources | Parent→child field map (see below). Name is **Bind** (confirmed). Not Link (Phase 82 URI). |
| Can Grow | Child output may expand the element / band (Phase 85) |
| Can Shrink | Child output may shrink (Phase 85) |
| Show child headers/footers | When false, omit child report/page headers and footers |

### Bind Datasources (mirror report Datasources)

Model Bindings like the existing report **Datasources** editor
(Phase 19b: `datasourceRows` / `DS_COUNT_ROW_ID` /
`datasourceSectionId` in `report-properties.js`).

- Section **Bind Datasources** with **Bind Count** (integer,
  0…N; **0 = no binding**, allowed).
- Count adds or trims **Bind 1**, **Bind 2**, … from the end
  (same as Datasource Count → Datasource N).
- Each Bind N has four rows:
  - **Parent Datasource**
  - **Parent Field**
  - **Sub Datasource**
  - **Sub Field**
- Client section ids (like `DATASOURCE_SECTION_ID` / 9100+N) —
  no 075 rows required unless Phase 83 chooses lookup-backed
  labels. Store on the item as a `binds` (or equivalent) array;
  blank/zero count omits binding at Render.

### Two usage modes

1. **Unbound / fixed-scope** — Bind Count **0**. The child runs
   its own datasources once (no parent field map). Typical
   placement: report header or footer for a static table or
   legend so the parent does not need an extra query. Render
   runs an unbound child **once per placement** and **may cache**
   that output for reuse on later pages/bands that reference the
   same unbound Subreport.

2. **Bound** — Bind Count ≥ 1. For each parent row, apply Bind
   Datasources (Parent Datasource/Field → Sub Datasource/Field),
   then run the child (e.g. line-item details under an invoice
   header row). Caching is per bind-key set, not global.

**Designer:** labelled box showing the child report name and an
**Open** action that opens the child in its own Reports tab.

**Render:** new lazy `Reports.Render.Subreport` module (rule 12).
Unbound: one child run (cacheable) offset into the element box.
Bound: per parent row, apply each Bind row, run the child
pipeline, return placed items offset into the element box.
**Depth limit** and **cycle detection** (A embeds B embeds A).
Later optimization for bound mode: batch the child query with
an IN-list of parent keys.

**Use case:** all-customers "package" run — parent report header +
Detail band contains the customer Statement report as a Subreport;
one Preview/PDF run emits every statement without a new band Kind.

---

## Phase 83 — Subreport designer element

### Goal

Place a Subreport on a band. Pick the child report, edit Bind
Datasources, and Open the child in its own tab. No Render change
required for Done means (canvas shows a labelled placeholder box).

### Entry gate

Andrew names Phase 83. Confirm 053 key 56 still exists. Next free
Acuranzo file re-checked for the 074 Category packet and any new
054 keys (Can Grow / Can Shrink / Show headers / Report picker).

### Work items

- [ ] 83.1 Lookup packet (**not** 1481): 074 key **8** Category
      Subreport (icon); set 053 key 56 `value_int` to 8; leave
      headroom for more Subreport Kinds. Reverse restores.
- [ ] 83.2 Palette / drop under the Subreport Category. Default
      box; labelled chrome with child name (or "Subreport").
- [ ] 83.3 Report picker Property: list saved reports; store child
      `report_id` (and rev policy as agreed).
- [ ] 83.4 **Bind Datasources** UI mirroring report Datasources:
      Bind Count (0 = none), then Bind N with Parent Datasource,
      Parent Field, Sub Datasource, Sub Field. Bind confirmed
      (not Link).
- [ ] 83.5 Can Grow, Can Shrink, Show child headers/footers
      (booleans; grow/shrink behaviour is Phase 85).
- [ ] 83.6 **Open** opens the child report in its own manager tab.
- [ ] 83.7 Vitest for Count trim/add and bind rows; lint;
      `templates:copy` as touched.
- [ ] 83.8 Browser: Category, drop, pick, Bind Count 0 and 2,
      Open, Undo.

### Done means

Designer places a Subreport under its own Category, stores Report
+ binds, and Open shows the child. Preview may still ignore the
child (Phase 84).

### Exit gate

Lint/tests green; Andrew confirms the browser check.

### Status

**not started**

### Accomplished

_(empty until the phase closes)_

### Lessons learned

_(empty until the phase closes)_

### Handoff

Phase 84. Bind naming is confirmed.

---

## Phase 84 — Subreport Render (fixed box)

### Goal

`stage=html` (and compose) places the child's output inside the
Subreport element box at a **fixed** size with **clipping**. No
grow/shrink yet.

### Entry gate

Phase 83 Status complete.

### Work items

- [ ] 84.1 Insert lazy `Reports.Render.Subreport` (rule 12: one
      migration for the new module; loader registers lazy load).
- [ ] 84.2 Unbound (Count 0): run child once per placement (cache
      allowed); Bound: per parent row resolve Bind → child params,
      run child; offset placed items into the element box; clip to
      width/height.
- [ ] 84.3 Depth limit + cycle detection; clear log lines on skip /
      fail.
- [ ] 84.4 Respect Show child headers/footers.
- [ ] 84.5 `render_check.sh` fixture: parent Detail + Statement
      child; clipped overflow. Browser Preview HTML check.

### Done means

Preview shows the child inside the box; overflow is clipped; cycles
are refused safely.

### Exit gate

Packets applied; checks green; Andrew confirms.

### Status

**not started**

### Accomplished

_(empty until the phase closes)_

### Lessons learned

_(empty until the phase closes)_

### Handoff

Phase 85.

---

## Phase 85 — Subreport grow/shrink and page split

### Goal

Can Grow / Can Shrink change the used element (and band) height from
the child's used height. Long child output can split across pages
with the same rules as other growing content.

### Entry gate

Phase 84 Status complete.

### Work items

- [ ] 85.1 Measure child used height; apply grow/shrink to the
      placed box and band layout (Layout / Paging / Subreport as
      needed — rule 12 per module).
- [ ] 85.2 Page split: continue child output on the next page when
      it does not fit; headers/footers policy unchanged from 84.4.
- [ ] 85.3 Optional: batch child query with IN-list of parent keys
      (optimization; may ship later if timeboxed).
- [ ] 85.4 Fixtures for grow, shrink, and multi-page child.
      Browser check.

### Done means

Grow/shrink and page split match the fixed design; package-of-
statements Preview paginates sensibly.

### Exit gate

Packets applied; checks green; Andrew confirms.

### Status

**not started**

### Accomplished

_(empty until the phase closes)_

### Lessons learned

_(empty until the phase closes)_

### Handoff

Next Kind review or Bind rename cleanup, as Andrew names.
