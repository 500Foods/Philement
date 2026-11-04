<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Preview

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 19–27
**Stop when:** The author can preview HTML, with pages, masks, and the generation log.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 18 in [`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md) is complete. A phase whose own entry gate allows an earlier phase to stay open still follows that gate.
**Next file:** [`/docs/Li/plans/reporting/04-pdf.md`](/docs/Li/plans/reporting/04-pdf.md) after Phase 27.

## Phase 19 — Compose skeleton

### Goal

Seed `Reports.Render` and return the structural intermediate for a
static report, a detail list, and groups. Scope amended 2026-11-03 (Andrew):
compose takes every band role the designer places after Phase 16, in
any count: report header, page header, group headers (multi-level,
nested in header order), detail (one or more), group footers, page
footer, report footer. Order RH → PH… → GH… → Detail → GF… → PF… → RF.

### Entry gate

Phase 1 fixtures exist. Phase 17 is applied if the script loads a
stored id. The fixture call, which passes `definition` inline, can be
written before Phase 17. Re-check the next file number.

### Work items

- [x] 19.1 Copy the `scripts` insert shape from
       [`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua).
       Group `Reports`, name `Render`, `invokable = 1`,
       `mcp_access = 0`.
- [x] 19.2 `stage=compose` emits the structural intermediate. A static
       report renders when `sources` is empty and the detail band has
       no datasource. The detail band repeats once per row. Field text
       is the plain value, so `1234.5` prints `1234.5` even when a
       mask is stored. Groups open and close at every level (amended from
       one level), on one page. Rows are taken in the order given; compose
       does not sort. The first detail with a datasource drives the rows;
       every detail band repeats per row. Group footers pair with headers
       by suffix (gh2/gf2), else by nesting order.
       The result includes `log`. A query datasource with no
       array adds a warn entry and contributes no rows. The fixture calls
       pass `stage=compose`. Phase 31 switches the default stage to
       `pdf`. The script skips top-level `history`. The intermediate
       and `log` leave that key out.
- [x] 19.3 A missing datasource, an unknown `typeKey`, and an empty
       `field` become warn entries on `log`. The script still returns a document.
       A `format` other than 1 returns an error through
       `H.set_result_json` and does not throw out of the worker.
- [x] 19.4 Hand the packet over. After apply, the three structural
       fixture calls match. Each call passes `stage` `compose`, the
       definition file as `definition`, and the sources file as
       `sources`. The contract is
       [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).

       | Case | Definition | Sources | Expected |
       | --- | --- | --- | --- |
       | static | [`static.definition.json`](/elements/003-lithium/tests/fixtures/reporting/static.definition.json) | [`static.sources.json`](/elements/003-lithium/tests/fixtures/reporting/static.sources.json) | [`static.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/static.intermediate.json) |
       | detail | [`detail.definition.json`](/elements/003-lithium/tests/fixtures/reporting/detail.definition.json) | [`detail.sources.json`](/elements/003-lithium/tests/fixtures/reporting/detail.sources.json) | [`detail.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/detail.intermediate.json) |
       | grouped | [`grouped.definition.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.definition.json) | [`grouped.sources.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.sources.json) | [`grouped.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.intermediate.json) |

       | bands (added) | [`bands.definition.json`](/elements/003-lithium/tests/fixtures/reporting/bands.definition.json) | [`bands.sources.json`](/elements/003-lithium/tests/fixtures/reporting/bands.sources.json) | [`bands.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/bands.intermediate.json) |
       | warnings (added) | [`warnings.definition.json`](/elements/003-lithium/tests/fixtures/reporting/warnings.definition.json) | [`warnings.sources.json`](/elements/003-lithium/tests/fixtures/reporting/warnings.sources.json) | [`warnings.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/warnings.intermediate.json) |

       The formatted `1,234.50` file is Phase 20. Packet:
       [`acuranzo_1444.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1444.lua).
- [x] 19.5 (added) Designer tabs. The JSON tab is renamed **Definition**
       (page id stays `json`). After Revisions come **Compose**,
       **Preview HTML**, and **Preview PDF**. Compose has a Run button on
       the right of the toolbar. It calls `Reports.Render` through
       `invokeScript` with `stage` `compose`, the open definition
       (history stripped), and the session sample under every declared
       datasource name (empty `sources` when there is no sample or no
       datasource). The result shows read-only in the same JSON editor as
       Definition. Empty `{}` becomes `[]` for `pages`, `log`, `bands`,
       and `items`. An error toasts once and keeps the last result. The
       two Preview tabs are placeholders: HTML is Phases 24 and 25; PDF is
       later. Code: [`reports-compose.js`](/elements/003-lithium/src/managers/reports/reports-compose.js).
       On Definition and Compose, Import and Export hide; Collapse all,
       Expand all, and the Query Manager font popup show (saved per page
       as `lithium_reports_font_json` / `_compose`). Run sits first in
       the right-hand cluster.
- [x] 19.6 (added) Browser check. Andrew enabled Scripting in
       `hydrogen-pintura.json`, applied 1444, and ran Compose from the
       designer: Definition, Compose, the Run button, the editor tools,
       and the Preview HTML/PDF placeholders all behave as listed.

### Done means

Andrew reports the five structural fixture calls matched, and the
script is `Reports.Render`.

### Exit gate

Andrew's apply report and those three calls. Test 98 on the new
migration if he ran it.

### Status

**Complete** (2026-11-03). Andrew applied `acuranzo_1444`, enabled
Scripting in `hydrogen-pintura.json`, and confirmed the Compose tab
live in the designer.

### Accomplished

- `Reports.Render` seeded (group `Reports`, invokable 1, mcp_access 0).
  `stage=compose` returns `{ pages, log }` for every band role in any
  count, with multi-level groups (scope widened from one level).
- Warnings for an unknown typeKey, a missing datasource, and an empty
  field. Format ≠ 1 and other stages return an error body.
- Fixtures: static, detail, grouped, plus added bands and warnings.
  All five match.
- Designer: JSON tab renamed **Definition**. New **Compose** tab (Run,
  read-only JSON, `{}`→`[]` normalising, one toast on error, last
  result kept). **Preview HTML** and **Preview PDF** placeholder tabs.
  Definition and Compose share Collapse all, Expand all, and the font
  popup; Import/Export hide there.

### Lessons learned

- Hydrogen encodes an empty Lua table as `{}`. Normalise list keys in
  the client until the encoder can tell arrays from objects.
- An invokable script needs Scripting enabled in the server config, or
  every call fails before the script runs. Check config before the
  migration when a phase adds a script.
- A local Lua harness against the fixtures caught every compose bug
  before the packet went over. Keep it for Phase 20.

### Handoff

- Phase 20 amends `Reports.Render` in a new file (next Acuranzo file
  1445). Do not edit 1444.
- The Compose tab is the quickest check for Phase 20: load the detail
  sample and Run. `amount` should read `1,234.50` once masks apply.
- Preview HTML is Phases 24/25. Preview PDF follows the PDF milestone.

---

## Phase 19b — Datasources editor and embedded JSON

### Goal

The Report owns its datasources. A `json` datasource keeps its rows
in the definition. Compose reads each datasource from the definition.

### Entry gate

Phase 19 Status complete. Runs beside Phase 20; it does not wait for
20 to close.

### Work items

- [ ] 19b.1 Report property panel: a Datasources section. Add and
       remove. Each entry is its own group (Datasource 1,
       Datasource 2, ...) with Name and Kind.
- [ ] 19b.2 Kind comes from a new datasource-kind lookup: JSON,
       Query, CSV. JSON is live. Query is listed and disabled until
       Phase 28. CSV is listed and ignored; a later importer turns a
       CSV (then XLS, XML, YAML) into JSON rows.
- [ ] 19b.3 Helium packet: one Acuranzo file for the kind lookup
       (next file after 1445). The definition stores the lookup key,
       matching the other report lookups. Query status `Reporting`
       stays in Phase 28.
- [ ] 19b.4 A `kind` `json` datasource stores `rows` in the
       definition, permanently: saved, revisioned, exported. Load
       Sample writes into the chosen datasource's `rows`. Fixed JSON
       and lookup tables are first-class.
- [ ] 19b.5 The first datasource drives iteration. Detail and group
       bands default to it. Grouping assumes, or enforces, a sort on
       the group key.
- [ ] 19b.6 The Field dropdown lists the columns of the band's
       datasource (the first one by default), not one global sample.
- [ ] 19b.7 Field items pick Field only. Item Datasource stays
       reserved for joins and lookups; not required here.
- [ ] 19b.8 `composeSources` builds `sources[name]` from each `json`
       datasource's `rows`. No copying one array under every name.
- [ ] 19b.9 `queryRef` and `params` are reserved keys on a `query`
       datasource. Documented only; Phase 28 fills them.
- [ ] 19b.10 Amend [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md):
       `json` rows live in the definition. Drops the Phase 15.1
       session-only rule for this path.

### Done means

Add two `json` datasources in the Report panel, load rows into the
first, bind a field, save, reload, and Compose prints the rows. The
rows survive export and import.

### Exit gate

Andrew's check in the designer, plus the kind lookup apply report.

### Notes

- Every Compose and Render call ships the rows through conduit.
  Params are capped at 256 KiB.
- Phase 28 owns the QueryRef picker (lookup 28 status `Reporting`,
  sorted by name, refresh control), the synthetic demo query, and
  fields and params (with datatypes and `calculated`) on the query
  definition.
- Lookup joins (Status 0/1 to True/False) and the palette buckets
  (Static, Fields, Aggregates, Lookups) follow later.
- Lua load, filter, and custom fields are not yet designed.

### Status

**Not started** — decisions locked by Andrew 2026-11-03.

---

## Phase 20 — Value format

### Goal

Compose applies the mask table. Null prints blank.

### Entry gate

Phase 19 Status complete.

### Work items

- [x] 20.1 Apply the Value format table. A number rounds half away
       from zero. An omitted number mask prints a plain decimal. A
       mask that does not parse warns once, and the raw value is
       printed. Dates follow the same table.
- [x] 20.2 The formatted detail fixture
       [`detail.intermediate.formatted.json`](/elements/003-lithium/tests/fixtures/reporting/detail.intermediate.formatted.json)
       prints `1,234.50` for `amount` and a blank `note`. The
       definition and sources are the Phase 19 detail pair. The
       contract is [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).
       A second fixture walks the rest of the mask table:
       [`masks.definition.json`](/elements/003-lithium/tests/fixtures/reporting/masks.definition.json),
       [`masks.sources.json`](/elements/003-lithium/tests/fixtures/reporting/masks.sources.json),
       [`masks.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/masks.intermediate.json).
- [ ] 20.3 Hand the amendment over. Do not edit the applied Phase 19
       file. After apply, both fixture calls match.

### Done means

The formatted detail fixture and the mask-table fixture both match.

### Exit gate

Andrew's apply report and the two fixture calls.

### Variances

- Hydrogen turns JSON null into nil, so a column absent from the row
  and a null value look the same. Both print blank; the
  absent-column warning is not raised.
- A unix-seconds value has a time when it is not exactly midnight UTC.
  With no mask, midnight prints `yyyy-mm-dd`, otherwise
  `yyyy-mm-dd hh:nn`.
- A negative under a mask without `( )` prints a leading minus before
  any `$` (`-$1.24`).
- A bad mask or a value that is not a number or date warns once per
  item per band.

### Status

**In progress** — acuranzo_1445 written; all six fixtures match the
local Lua 5.5 harness. Waiting on Andrew to apply 1445.

---

## Phase 21 — Paging

### Goal

Fixed-height paging keeps a group header with its first detail row.

### Entry gate

Phase 20 Status complete. Masks do not change band height.

### Work items

- [ ] 21.1 Page breaks use the stored band heights against the content
       box. If the group header plus one detail row do not fit in the
       remaining box, both move to the next page. If they do not fit
       on an empty page either, compose warns and places them anyway.
       The header is not repeated on a later page. Its summary is
       every row of the group.
- [ ] 21.2 Add an overflow fixture beside the Phase 1 files. The
       header moves with its first detail row.
- [ ] 21.3 Hand the amendment over. After apply, the overflow fixture
       matches.

### Done means

The overflow fixture's page list keeps the group header with its
first detail row.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 22 — System datetime and system user

### Goal

The palette can place a system datetime and a system user, and compose prints them.

### Entry gate

Phase 21 Status complete.

### Work items

- [ ] 22.1 The palette adds lookup 053 key 5 (System datetime) and key 6 (System user). A new datetime item stores the omitted-mask default. A new user item stores no mask.
- [ ] 22.2 System user prints `params._hydrogen.username`. An empty username prints blank and adds a warn log entry.
- [ ] 22.3 System datetime uses the one generation instant from
       Value format. Two items on the same report share it. It adds
       `params._hydrogen.tzoffset` minutes, where a negative offset
       is west of UTC, and formats that civil time with the item
       mask. The mask grammar does not change. `tz` is the IANA name
       Hydrogen stored on the claim, and compose does not look it up.
       A missing `tzoffset` formats the instant in UTC and adds an
       info log entry. Field datetimes keep the Value format rules.
- [ ] 22.4 Fixture: username `ada` prints `ada`. An empty username
       prints blank and adds a warn entry. The call sends
       `params.now` `1772719629` and does not read the live clock.
       With `tzoffset` `-420` and mask `yyyy-mm-dd hh:nn` it prints
       `2026-03-05 07:07`. The same instant with no `tzoffset` prints
       `2026-03-05 14:07` and adds an info entry. Hand the amendment
       over.

### Done means

The fixture prints the username and the timezone-shifted system datetime.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

### Note (2026-11-03)

When this phase opens, swap upstream `flatpickr@4.6.13` to the community
edition with the npm alias `"flatpickr": "npm:flatpickr-ce@4.9.0"` (pin
exact, no `^`). Smoke `shared/flatpickr-picker.js`, the property Date
editor, and Profile date-formats. Upstream has had no releases since
2022; `flatpickr-ce` claims drop-in and started shipping updates
2026-10-27. Do not chase every CE bump for a week after the swap.

---

## Phase 23 — Page number and page count

### Goal

Page number and page count print on the page that owns them.

### Entry gate

Phase 22 Status complete.

### Work items

- [ ] 23.1 The palette adds lookup 053 key 61 (Page number) and key 62 (Page count). Both use the number mask. An omitted mask prints a plain integer.
- [ ] 23.2 Compose writes the page's number and the report's page count into those items after the page list exists.
- [ ] 23.3 Fixture: a two-page report prints `1` and `2`, and page count `2` on both pages. Hand the amendment over.

### Done means

The two-page fixture shows the page number and the page count.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 24 — HTML emit

### Goal

`stage=html` emits one section per page of the composed document.

### Entry gate

Phase 23 Status complete.

### Work items

- [ ] 24.1 `stage=html` emits one section per page with
       `break-after: page`, the composed text, and positions in
       physical CSS units. Align, font family, size, and style come
       from the item, then the definition font. Text clips to the
       item box. A call that omits `stage` still runs `compose`
       until Phase 31.
- [ ] 24.2 Add the expected HTML skeleton for the detail fixture.
- [ ] 24.3 Hand the amendment over. After apply, the HTML fixture
       matches.

### Done means

The detail fixture's HTML matches the expected skeleton.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 25 — Preview

### Goal

The designer runs `Reports.Render` at `stage=html` and shows that
HTML in a sandboxed iframe.

### Entry gate

Phase 24 Status complete. The open definition's datasources are the
session sample from Phase 15, or an empty object for a static report.

### Work items

- [ ] 25.1 If `src/shared/conduit.js` still has no `invokeScript`,
       add it for `POST /api/conduit/script` (JWT-bearer, params shape
       per the call contract) and cover it with a unit test. If
       CATCHUP already added it, call that function.
- [ ] 25.2 Preview requests `stage=html` with the current definition
       and the session sample. DOMPurify runs on the HTML. The result
       is assigned to a sandboxed iframe `srcdoc`. The parent page
       does not assign report HTML to `innerHTML`.
- [ ] 25.3 A render button renders the whole report. Auto-render on
       change is decided by the browser trial in 23.5. If it wins, it
       debounces, and it never fires while the pointer is dragging
       or resizing.
- [ ] 25.4 A result that carries `error` toasts once and leaves
       the canvas as it was. `log` stays on the result. This phase
       does not toast each entry. The generation-log phase shows them.
- [ ] 25.5 Browser: preview the detail fixture with two sample rows
       and see two detail bands. Preview a report whose sample is
       missing and see one warn entry on `log`.

### Done means

The operator can preview the HTML the script produced for the detail
fixture.

### Exit gate

Lint, the conduit unit test, and the browser check against an applied
`Reports.Render`.

### Status

**not started**

---

## Phase 26 — Generation log

### Goal

A generation shows its info, warn, and error messages, and the open report shows the three counts.

### Entry gate

Phase 25 Status complete.

### Work items

- [ ] 26.1 The preview keeps the script's `log` array. The panel lists each entry's level and message. Lithium counts `info`, `warn`, and `error`. The report header shows those three counts for the last generation.
- [ ] 26.2 A non-zero error count flags the open report as an error. A warn count with zero errors flags it as a warning. Info only flags it as info. Zero entries show no flag. The counts are session state. They are not columns on `reports`.
- [ ] 26.3 Browser: the missing-sample preview shows one warn entry, a warn count of 1, and the warn flag. A clean detail preview shows an empty log and no flag.

### Done means

The operator can read the generation messages and see the three counts on the open report.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 27 — Over-cap preview

### Goal

A definition that does not fit in the params cap still previews, by
saving first and rendering by `report_id`.

### Entry gate

Phase 26 Status complete. Phase 18 is already applied.

### Work items

- [ ] 27.1 When the inline params would exceed
       `ClientInvokeMaxParamsBytes`, save the definition and preview
       by `report_id`. One toast names that path. Do not send the
       session JWT as a script param.
- [ ] 27.2 Browser: an over-cap definition previews after that toast,
       and the canvas is unchanged when the save fails.

### Done means

An over-cap definition previews by `report_id` after one toast.

### Exit gate

Lint and the browser check.

### Status

**not started**

