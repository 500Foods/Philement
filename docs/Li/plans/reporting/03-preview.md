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

- [x] 19b.1 Report property panel: a Datasources section. Add and
       remove. Each entry is its own group (Datasource 1,
       Datasource 2, ...) with Name and Kind.
- [x] 19b.2 Kind comes from a new datasource-kind lookup: JSON,
       Query, CSV. JSON is live. Query is listed and disabled until
       Phase 28. CSV is listed and ignored; a later importer turns a
       CSV (then XLS, XML, YAML) into JSON rows.
- [x] 19b.3 Helium packet: one Acuranzo file for the kind lookup
       (next file after 1445). The definition stores the lookup key,
       matching the other report lookups. Query status `Reporting`
       stays in Phase 28.
- [x] 19b.4 A `kind` `json` datasource stores `rows` in the
       definition, permanently: saved, revisioned, exported. Load
       Sample writes into the chosen datasource's `rows`. Fixed JSON
       and lookup tables are first-class.
- [x] 19b.5 The first datasource drives iteration. Detail and group
       bands default to it. Grouping assumes, or enforces, a sort on
       the group key. (Default to first DS done; grouping assumes the
       sort — enforcing it is deferred.)
- [x] 19b.6 The Field dropdown lists the columns of the band's
       datasource (the first one by default), not one global sample.
- [x] 19b.7 Field items pick Field only. Item Datasource stays
       reserved for joins and lookups; not required here.
- [x] 19b.8 `composeSources` builds `sources[name]` from each `json`
       datasource's `rows`. No copying one array under every name.
- [x] 19b.9 `queryRef` and `params` are reserved keys on a `query`
       datasource. Documented only; Phase 28 fills them.
- [x] 19b.10 Amend [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md):
       `json` rows live in the definition. Drops the Phase 15.1
       session-only rule for this path.
- [x] 19b.11 Each datasource gets its own panel section
       "Datasource N" (client section ids 9100+, max 20) with Name,
       Kind and Rows; the "Datasources" section keeps only Count.

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

**Complete** (2026-11-03). Andrew accepted the designer check:
Compose printed `1,250.50` / `89.99` / `4,300.00` from an embedded
`json` datasource with `"log": []`. Andrew accepted designer check;
confirm 1446 applied. He noted the Datasources UI "will need work but
ok for now" — polish follows later.

### Accomplished

- Report panel: **Datasources** section (Count) plus one
  "Datasource N" section each (ids 9100+, max 20) with Name, Kind,
  Rows. Kind stores `kindKey` (lookup 076) plus `kind` code.
- `json` rows live in the definition (saved, revisioned, exported).
  Load Sample writes into the band's datasource; session `_sample` gone.
- Field dropdown lists the band datasource's columns. `composeSources`
  sends each datasource its own rows; Compose strips `rows` from the
  definition it sends.
- acuranzo_1446 (lookup 076 Report Datasource Kinds) applied.
- LITHIUM-RPT.md amended (rows in the definition).

### Lessons learned

- Decide datasource ownership up front (Report owns them, first one
  drives iteration); it removed a whole class of session-state bugs.
- Keep fields/params metadata out of the kind phase; Phase 28 needed
  it shaped by the real query, not guessed early.
- Every Compose call ships rows through conduit (256 KiB param cap);
  big datasets belong on the Phase 29 server-side path.

### Handoff

- Datasources panel needs UI work later (band DS picker, layout).
- Grouping assumes a sort on the group key; enforcing it is deferred.
- Query kind shipped in Phase 28. CSV waits on a later importer.
- Lua load, filter, and custom fields are not yet designed.
- Superseded: next Acuranzo file is now **1450** (see Phase 21).

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
- [x] 20.3 Hand the amendment over. Do not edit the applied Phase 19
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

**Complete** (2026-11-03). acuranzo_1445 handed over; all six
fixtures match the local Lua 5.5 harness. Andrew's Compose run
2026-11-03 showed masked totals `1,250.50` / `89.99` / `4,300.00`
under `#,##0.00` and `"log": []`.

### Accomplished

- acuranzo_1445 amends `Reports.Render` compose (1444 untouched):
  field items go through the Value format table; null prints blank;
  bad mask/value warns once per item per band and prints raw.
- Fixtures: `detail.intermediate.formatted.json` and the `masks.*`
  set. Static, grouped, bands, warnings unchanged.

### Lessons learned

- Hydrogen maps JSON null to nil: absent and null columns are
  indistinguishable in Lua. Design warnings around that.
- Amend in a new migration file; never edit an applied one. The
  local Lua 5.5 harness against the fixtures again caught every bug
  before hand-over.

### Handoff

- Masks do not change band height, so Phase 21 paging can ignore them.
- Superseded: next Acuranzo file is now **1450** (1446–1449 used by
  19b and 28).

---

## Phase 21 — Paging

### Goal

Fixed-height paging keeps a group header with its first detail row.

### Entry gate

Phase 20 Status complete. Masks do not change band height.

### Work items

- [x] 21.1 Page breaks use the stored band heights against the content
       box. If the group header plus one detail row do not fit in the
       remaining box, both move to the next page. If they do not fit
       on an empty page either, compose warns and places them anyway.
       The header is not repeated on a later page. Its summary is
       every row of the group.
- [x] 21.2 Add an overflow fixture beside the Phase 1 files. The
       header moves with its first detail row.
- [x] 21.3 Hand the amendment over. After apply, the overflow fixture
       matches.

### Done means

The overflow fixture's page list keeps the group header with its
first detail row.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**Complete** (2026-11-03 PDT). Andrew applied acuranzo_1450 and
confirmed paging in Compose: increasing band heights pushed content
past the page limit and produced more than one page.

### Accomplished

- acuranzo_1450 (does not edit 1449): `Reports.Render` compose splits
  bands into pages. Content box = `page.height` less top and bottom
  margins (lengths in in/mm/cm/pt/px). Each page opens with the page
  headers (report header first on page 1) and reserves the page
  footers. A band that does not fit breaks the page. Group headers
  at a break need room for themselves plus the row's detail bands, or
  all move together. Too big for an empty page: warn and place.
  Group headers do not repeat. Report footer must fit on the last
  page and still prints after the page footer, as before.
- Fixture set `overflow.*` (3 in page): North header and three rows
  plus footer fill page 1; the South header would fit alone but moves
  to page 2 with its first row. All seven sets match.

### Lessons learned

- The definition already stores page height and margins (copied from
  lookup 057 by the designer), so compose reads no lookup.

### Handoff

- Phase 21 closed. Next: Phase 22 (System datetime and system user).
- Phase 23 adds page number/count on top of the `pages` list.
- Next Acuranzo file **1451**; QueryRef 175 still unused.

### Start here (fresh agent) — historical, phase closed

- **Machine:** angrin. Repo `/mnt/extra/Projects/Philement`.
- **Scope:** 21.1–21.3 only. Fixed-height paging in
  `Reports.Render` `stage=compose`: break pages on stored band heights
  against the page content box (page size lookup 057, margins); keep a
  group header with its first detail row; warn and place anyway when
  they do not fit an empty page; no repeated group headers. Page
  number / page count is Phase 23, not here.
- **Code:** `Reports.Render` lives in Acuranzo migrations
  `elements/002-helium/acuranzo/migrations/` (seed 1444, amendments
  1445 masks, 1449 Parameter item). Write a **new** amendment
  **acuranzo_1450.lua**; do not edit applied files.
- **Fixtures:** `elements/003-lithium/tests/fixtures/reporting/`
  (`*.definition/sources/intermediate.json`). Add an `overflow.*` set;
  all existing sets must still match. Check with a local Lua 5.5
  harness before hand-over.
- **Designer check:** Compose tab (`reports-compose.js`) on Reporting
  Demo (QueryRef 174, `ROW_LIMIT` param) gives an easy many-row source.
- **Datasources:** settled; read Phase 19b and Phase 28 (04-pdf.md)
  Status/Handoff only if needed. Do not redesign.
- **Constraints:** Andrew applies migrations (no SchemaTool apply by
  the agent); no commit unless asked; do not reopen Phase 14/14b;
  flatpickr stays parked on Phase 22; new palette items must be
  canvas-draggable (rule from Phase 28).
- **Next caller-facing QueryRef:** 175.

---

## Phase 22 — System datetime and system user

### Goal

The palette can place a system datetime and a system user, each
datetime carries a Timezone property, and compose prints them.

### Entry gate

Phase 21 Status complete.

### Locks (2026-11-03, Andrew)

- Mask language everywhere is the **Luxon token subset** (see Value
  format in REPORTING_PLAN.md). Phase 22 may print with the existing
  Phase 20 formatter as an interim; the dialect switch for every
  datetime item is Phase 22b. Fixtures written here use masks that
  read the same in both dialects where possible, and 22b rewrites them.
- Timezone property: `User` | `UTC` | `Explicit`. Explicit stores an
  IANA name on the item (e.g. `America/Vancouver`).
- Report-level default zone (IANA string) for scheduled / JWT-less
  runs and for `User` when there is no claim.
- User zone comes from the JWT `tz` claim via `params._hydrogen.tz`
  (login posts `Intl…resolvedOptions().timeZone`). Mid-session
  `tzoffset` drift is accepted; do not engineer around it.

### Work items

- [x] 22.1 The palette adds lookup 053 key 5 (System datetime) and key
       6 (System user); both are canvas-draggable. A new datetime item
       stores the omitted-mask default and `timezone` `User`. A new
       user item stores no mask.
- [x] 22.2 System user prints `params._hydrogen.username`. An empty
       username prints blank and adds a warn log entry.
- [x] 22.3 Property panel: Timezone select `User` | `UTC` | `Explicit`;
       Explicit shows an IANA text field. Report properties add a
       default zone (IANA string, empty = none). Datetime items that
       format an instant (system, unix-number field/calculated/
       aggregate) get the same property; civil-string field values
       keep no-shift.
- [x] 22.4 Resolution in compose, one generation instant shared by
       all items (`params.now` or `H.system.now()`):
       `UTC` → UTC. `User` → `_hydrogen.tz` if present, else report
       default, else UTC + info log. `Explicit` → the item zone.
       Until 22a lands, an IANA zone formats with
       `_hydrogen.tzoffset` when the zone equals `_hydrogen.tz`;
       otherwise (Explicit / report default with no luatz) UTC + info
       log naming the zone. After 22a, all IANA zones resolve through
       luatz at the generation instant.
- [x] 22.5 Fixture: username `ada` / empty + warn; `params.now`
       `1772719629`; `User` with `tz` `America/Vancouver`, `tzoffset`
       `-420` prints `2026-03-05 07:07`; `UTC` prints
       `2026-03-05 14:07`; `User` with no claim and no default prints
       `2026-03-05 14:07` + info. Ship as the next free Acuranzo file
       (start from **1451**). Hand the amendment over.

### Done means

The fixture prints the username and the zoned system datetime for
User and UTC; Explicit is stored and either resolves (22a applied) or
falls back with an info entry.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**Complete** (2026-11-03). Andrew applied acuranzo_1451–1453 and confirmed User time, UTC time, mask presets, and System User username in the designer. Phase 22 closed with 22a and 22b.

### Note (2026-11-03)

When this phase opens, swap upstream `flatpickr@4.6.13` to the community
edition with the npm alias `"flatpickr": "npm:flatpickr-ce@4.9.0"` (pin
exact, no `^`). Smoke `shared/flatpickr-picker.js`, the property Date
editor, and Profile date-formats. Upstream has had no releases since
2022; `flatpickr-ce` claims drop-in and started shipping updates
2026-10-27. Do not chase every CE bump for a week after the swap.

---

## Phase 22a — Seed luatz (Helium library pack)

### Goal

`require("luatz.init")` works inside `Reports.Render`, reading the
host's tzdata, so Explicit and report-default IANA zones format
correctly (DST included) with or without a JWT.

### Entry gate

Phase 22 Status complete (or Andrew waives to run in parallel). Must
land before Explicit IANA is relied on and before 22b fixtures that
use a named zone.

### Work items

- [x] 22a.1 Seed luatz modules as Helium script rows like lua-pdfkit /
       lua-zint: `luatz.*`, group `luatz`, `invokable=0`, loaded by
       `group.script`. Pure Lua, no C. Pin the upstream version.
- [x] 22a.2 zoneinfo comes from the **host OS package** (`tzdata`,
       `/usr/share/zoneinfo`) via `io.open`. Do **not** bundle
       zoneinfo in the migration. If the sandbox blocks that path,
       allow read-only `io.open` under the zoneinfo root rather than
       ship data. DOKS images (Ubuntu/Fedora-shaped) carry tzdata.
- [x] 22a.3 A compose helper resolves (IANA, instant) → offset; an
       unknown zone or unreadable tzfile falls back to UTC + warn log.
- [x] 22a.4 Fixture: `America/Vancouver` at `1772719629` prints
       `2026-03-05 06:07` (PST, -480) and a summer instant prints at
       -420; bad zone → UTC + warn. Ship as the next free Acuranzo
       file. Hand over.

### Update / refresh

- **zoneinfo data** is not owned by the migration. It refreshes when
  the host/container image updates its `tzdata` package; cadence
  follows the base image rebuild (TBD Andrew: image rebuild policy,
  e.g. monthly or on IANA release).
- **luatz code** is versioned: the seed records the upstream version
  /commit in script metadata (TBD: column or comment header, e.g.
  `-- luatz <version> <commit>` plus a `luatz.version` module
  returning the string). An update is a new Acuranzo migration that
  replaces the `luatz.*` rows and bumps that version; never edit the
  applied seed.
- **Config hooks (env, read by Render in 1452):** `REPORTS_TZ_ZONEINFO_PATH` and `REPORTS_TZ_DEFAULT_ZONE`; were TBD as `reports.tz.zoneinfo_path` (default
  `/usr/share/zoneinfo`), `reports.tz.default_zone` (fallback when
  report has none), optional `reports.tz.tzdata_version` reported in
  the generation log (read from `/usr/share/zoneinfo/tzdata.zi`
  `# version` line when present). Where config lives (Helium config
  table vs server JSON) is TBD.
- No cron system in this phase; refresh is image rebuild + a later
  migration when the luatz pin moves.

### Done means

Named zones format at the generation instant from host tzdata.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**Complete** (2026-11-03). Andrew applied acuranzo_1452 with 1451/1453; User and UTC confirmed in the designer. Named-zone luatz path still needs Sandbox.AllowIo (or a read-only zoneinfo allow) for Explicit/defaultZone against host tzdata.

---

## Phase 22b — Luxon masks and format presets Lookup

### Goal

Every report datetime mask uses the Luxon token subset (same language
as Lithium / Profile), and authors pick masks from an editable Lookup
of presets.

### Entry gate

Phase 22 complete; Phase 22a complete if any fixture uses a named zone.

### Work items

- [x] 22b.1 Replace the Phase 20 date dialect in the compose
       formatter with the Luxon subset: `yyyy yy MMMM MMM MM M dd d
       EEEE EEE HH H mm m ss s SSS`, `W`/`WW` ISO week (first week
       containing a Thursday), literals in `'...'`. `n`/`nn` local
       week is deferred (Sunday first-day assumed until a Profile
       option). Locale macros (`D`/`t`/`f`…) and zone-name tokens are
       out for v1. Omitted-mask defaults become `yyyy-MM-dd` and
       `yyyy-MM-dd HH:mm`.
- [x] 22b.2 Applies to all datetime items: field, calculated,
       aggregate, parameter, system. Migrate stored masks on existing
       definitions (old `mm`/`nn`/`hh` → `MM`/`mm`/`HH`) in the same
       amendment or document a one-shot conversion.
- [x] 22b.3 New Lookup 077 (Andrew picked) of format presets, rows
       `{ caption, mask, kind }`. Seed:
       `EEEE MMMM d yyyy HH:mm:ss`, `yyyy-MM-dd HH:mm:ss`,
       `yyyy-MM-dd HH:mm:ss.SSS`, `yyyy-MMM-dd HH:mm`,
       `yyyy-MMM-dd (EEE) HH:mm`, `yyyy-MMM-dd ('Wk' W)`.
- [x] 22b.4 Property panel preset picker shows caption + live sample;
       picking writes the mask string onto the item. Custom edit stays.
- [x] 22b.5 Rewrite `masks` / datetime fixtures to Luxon; all existing
       sets still match. Ship as the next free Acuranzo file(s).

### Done means

One mask language across Lithium and compose; presets editable in
Lookups.

### Exit gate

Andrew's apply report and the fixture calls.

### Status

**Complete** (2026-11-03). Andrew applied acuranzo_1453 and confirmed mask presets in the property panel. Luxon dialect and Lookup 077 are live.

---

## Phase 23 — Page number and page count

### Goal

Page number, page count and a Page label print on the page that owns them.

### Entry gate

Phase 22 Status complete.

### Work items

- [x] 23.1 The System palette adds lookup 053 key 61 (Page number) and key 62 (Page count), click + drag. Both use the number mask. An omitted mask prints a plain integer.
- [x] 23.2 New lookup 053 key 64 (Page label, Number group, value_int 7) is one text item with a freeform `template` string: `{n}` = page number, `{c}` = page count. New items start with `Page {n} of {c}`; a missing or empty template prints that default. Other `{...}` text is left as-is; no brace escape in v1. One item keeps alignment right when n and c grow (static text plus separate numbers does not). No presets: the property panel has one Template text field (Andrew 2026-11-03).
- [x] 23.3 Compose (acuranzo_1454) writes the page number, page count and label into 61/62/64 after the page list exists; every band on a page gets the same values.
- [x] 23.4 Fixture `pagenum.*`: two pages print `1` and `2`, count `02` (mask `00`) on both, Page label `Page 1 of 2` / `Page 2 of 2` (right aligned) and `Pg 1/2` / `Pg 2/2`. Hand the amendment over.

### Done means

The two-page fixture shows the page number, the page count and the Page label.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**Complete** (2026-11-04). Andrew applied acuranzo_1454 and confirmed Page Number, Page Count and Page Label in the designer.

### Accomplished

- Lookup 053 keys 61 (Page number), 62 (Page count), 64 (Page label) in acuranzo_1454.
- Compose fills 61/62/64 after paging; every band on a page gets the same values.
- Lithium System palette: Page Number / Page Count / Page Label (click + drag); Template row in the property panel.
- Fixture `pagenum.*` and unit test `reports-pagelabel.test.js`.

### Lessons learned

- Page Label is one freeform template string with `{n}`/`{c}`. No coded presets.
- A new item key must land in every Lithium registry. `ITEM_TYPES` in `model/document.js` omitted 64 while palette, drop, sample and properties had it, so drop threw `Item type 64 is not a v1 item.` Fix: add 64 to `ITEM_TYPES`. `MASK_TYPES` stays without 64 (template, not mask). Page Number/Count were already in `ITEM_TYPES`, so they worked first. Check `ITEM_TYPES` first when a new key fails on drop.
- The fix was Lithium-only; acuranzo_1454 and Helium compose were correct. No follow-up migration.
- Andrew applies migrations. Next Acuranzo file is 1455.

### Handoff

- Phase 24 (HTML emit) is next. `Reports.Render` in acuranzo_1454 still rejects any stage other than `compose`; Phase 24 adds `html` in acuranzo_1455.

---

## Phase 24 — HTML emit

### Goal

`stage=html` emits one section per page of the composed document.

### Entry gate

Phase 23 Status complete.

### Work items

- [x] 24.1 `stage=html` returns `{ "html", "log" }`. One section per
       page, at the definition page size, with `box-sizing: border-box`
       and padding equal to the four margins. `break-after: page` is
       on the section. Positions keep the stored unit and drop the
       space (`0 in` becomes `0in`). Align, font family, size, and
       style come from the item, then the definition font. Text is
       escaped and clips to the item box. A call that omits `stage`
       runs `compose` until Phase 31.
- [x] 24.2 The expected HTML skeleton is
       [`/elements/003-lithium/tests/fixtures/reporting/detail.html`](/elements/003-lithium/tests/fixtures/reporting/detail.html).
       It is the `html` value for the detail definition and the
       detail sources, including the final newline.
- [x] 24.3 Hand the amendment over. Andrew confirmed the rendered
       Preview HTML page on 2026-11-04 ("That works pretty well") and
       the Editor-matching chrome ("Excellent work"). The Lua 5.5.1
       probe matched `detail.html`. He did not send a separate byte
       dump of the live result.

### Done means

The detail fixture's HTML matches the expected skeleton.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**Complete** (2026-11-04). Andrew confirmed the rendered Preview HTML page and the Editor-matching chrome. That is the apply report for `acuranzo_1455.lua`: the previous script rejects `stage=html`, and the page rendered. Variance: the skeleton match is the Lua 5.5.1 probe against `detail.html`. He did not send a separate byte dump of the live result. No lookup. No QueryRef. Next free Acuranzo file is **1456**. QueryRef **175** stays unused.

### Accomplished

- `acuranzo_1455.lua` copies the 1454 `Reports.Render` body and adds `stage=html`. Reverse restores the 1454 body. No lookup. No QueryRef. Leave the file as written.
- [`/elements/003-lithium/tests/fixtures/reporting/detail.html`](/elements/003-lithium/tests/fixtures/reporting/detail.html) is the detail skeleton.
- The Emit lock in [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md) and the HTML section of [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md) record `{ html, log }` and the page section.
- Lua 5.5.1 ran the new body with a mock `H.set_result_json`. Compose of the detail definition and detail sources matched `detail.intermediate.formatted.json`. `stage=html` matched `detail.html`. An omitted stage returned pages. `stage=pdf` was rejected. A field value `A<B>&` was escaped. The static fixture produced one section containing `Static`. `luacheck` on `acuranzo_1455.lua` reported 0 warnings.
- Preview HTML's Run button calls `stage=html` through the existing `invokeScript`. `sanitizeHtml` runs first. The page goes to a sandboxed iframe `srcdoc`. The parent does not assign report HTML to `innerHTML`. An error toasts once and leaves the previous page. `log` is kept and not shown. Font settings stay off that tab. There is no auto-render. The code is [`/elements/003-lithium/src/managers/reports/reports-preview.js`](/elements/003-lithium/src/managers/reports/reports-preview.js).
- The paper sits in the Editor stage (`.reports-stage` / `.reports-stage-body`). The iframe is page-sized (`--reports-preview-width`, `--reports-preview-height`), with the sheet's corner and shadow plus a page edge. Rules are in [`/elements/003-lithium/src/managers/reports/reports-preview.css`](/elements/003-lithium/src/managers/reports/reports-preview.css). Definition and Compose use the same inset: `.reports-json-wrap` padding is `var(--space-2) var(--space-3)`.
- `npx vitest run tests/unit/reports-preview.test.js tests/unit/reports-chrome.test.js` — 2 files, 22 passed. `npm run lint:css` passed after the chrome. ESLint on `reports-preview.js`, `reports-pages.js`, and those two test files passed. `npm run templates:copy` copied 138 templates. `npm run lint` passed after the Run-button wiring. `reports.js` is 1000 lines.

### Lessons learned

- A stored length is `0 in`. A CSS length is `0in`. The emitter strips that space and keeps the unit.
- `box-sizing: border-box` is what puts the margin padding inside the page width and height.
- The HTML field is `html`. `log` stays beside it. `pdf_base64` remains the PDF name for Phase 31.
- `luacheck` sees the migration, and the script body is a string. The probe is what runs the body.
- The reverse body is the 1454 script, so a rollback keeps page number, page count, and Page label.
- Chrome's DOMPurify keeps the page `section`, including `break-after` and the page box, and drops a `script`. happy-dom's DOMPurify reads an empty tag name and unwraps that section. The unit test asserts the item styles and that a script does not survive. Do not change the emitter from `section` to `div` to satisfy happy-dom.
- An iframe with no `srcdoc` makes happy-dom log a DOMException about `about:blank`. `srcdoc=""` takes the srcdoc path and stays quiet.
- `sandbox=""` cannot be measured, so several pages use `calc(page height * section count)` in one frame. The frame is `content-box`, so the 1px edge sits outside the paper and does not shrink the section.
- The right-pane toolbar zeros its bottom margin. The gap under the toolbar is the pane's top padding. The Editor already had that on `.reports-stage-body`. Definition and Compose needed the same top padding on `.reports-json-wrap`.
- `reports.js` is at the 1000-line cap. The Run wiring stayed inside it by collapsing `_onComposeRun` to one line. New preview CSS belongs in `reports-preview.css`. `reports.css` is already over the cap.

### Handoff

- Leave `acuranzo_1455.lua` as written. Do not apply another migration for this phase. Next free Acuranzo file is **1456**. QueryRef **175** stays unused.
- A call that omits `stage` still returns `{ pages, log }` until Phase 31. `stage=pdf` is still rejected.
- Phase 25 is next, and it stays in progress. The Run path, the sanitizer, the error toast, and the page chrome are already in. Do not rebuild them. Font settings stay off Preview HTML.
- Auto-render (25.3b) is undecided. Ask before adding it. If it is chosen, it debounces, and it never fires while the pointer is dragging or resizing.
- The open browser check (25.5) is two sample rows producing two detail bands. The missing-sample warn is not on screen. That half moves to Phase 26. Do not build the generation log in the Phase 25 sitting. Do not start Phase 26 until Phase 25 is complete.
- `reports.js` is 1000 lines. Do not add a line without collapsing one. New preview CSS stays in `reports-preview.css`.

---

## Phase 25 — Preview

### Goal

The designer runs `Reports.Render` at `stage=html` and shows that
HTML in a sandboxed iframe.

### Entry gate

Phase 24 Status complete. Andrew asked for the page during the Phase
24 sitting, so the Run path and the page chrome landed before this
phase's own sitting. The open definition's datasources are the session
sample from Phase 15, or an empty object for a static report.

### Work items

- [x] 25.1 `invokeScript` was already in `src/shared/conduit.js`.
       Preview calls that function. No new client.
- [x] 25.2 Preview requests `stage=html` with the current definition
       and the session sample. `sanitizeHtml` runs on the HTML. The
       result is assigned to a sandboxed iframe `srcdoc`. The parent
       page does not assign report HTML to `innerHTML`. Landed in
       the Phase 24 sitting (`reports-preview.js`).
- [x] 25.3 The Run button (`#reports-preview-btn`) renders the whole
       report. It shows only on Preview HTML. Landed in the Phase 24
       sitting.
- [x] 25.3b Auto-render on change is declined (Andrew, 2026-11-04).
       Run is the only trigger. The cited browser trial "23.5" does
       not exist.
- [x] 25.4 A result that carries `error` toasts once (`Preview failed`)
       and leaves the previous iframe page as it was. `log` stays on
       the result and is not shown. The generation-log phase shows
       the entries. Landed in the Phase 24 sitting.
- [x] 25.5 Browser: Andrew previewed the detail fixture with two
       sample rows on 2026-11-04. The second detail band rendered.
- [~] 25.5b A missing sample writes one warn on `log`. The screen
       does not list `log`. That check moves to Phase 26.3.

### Done means

The operator can preview the HTML the script produced for the detail
fixture.

### Exit gate

Lint, the conduit unit test, and the browser check against an applied
`Reports.Render`.

### Status

**Complete** (2026-11-04). Andrew declined auto-render. Run is the only trigger. He previewed two detail rows and the second band rendered. No source change this sitting. The missing-sample warn stays in Phase 26. No new file. No new QueryRef. Next free Acuranzo file is **1456**. QueryRef **175** stays unused.

### Accomplished

- Run (`#reports-preview-btn`) calls `Reports.Render` at `stage=html` with the open definition and the session sample, through the existing `invokeScript`. `sanitizeHtml` runs first. The page goes to a sandboxed iframe `srcdoc`. The parent does not assign report HTML to `innerHTML`. An error toasts `Preview failed` once and leaves the previous page. Font settings stay off that tab. The code is [`/elements/003-lithium/src/managers/reports/reports-preview.js`](/elements/003-lithium/src/managers/reports/reports-preview.js).
- Auto-render was not built. Andrew declined it on 2026-11-04.
- Andrew's browser check: two sample rows, and the second detail band rendered.
- The Phase 24 sitting's checks still cover this code: `npx vitest run tests/unit/reports-preview.test.js tests/unit/reports-chrome.test.js` (2 files, 22 passed), `npm run lint`, `npm run lint:css`, and ESLint on the preview files. This sitting did not change source and did not re-run them.

### Lessons learned

- The work item's "browser trial in 23.5" does not exist. Phase 23 has no such item. The auto-render choice had to be asked, and the answer was Run only.
- A second detail row repeats the detail band in the emitted HTML. That is the preview check. It is not a second page by itself.
- `log` is already on the result. Showing it is a different phase. The missing-sample warn cannot be confirmed until that panel exists.

### Handoff

- Phase 26 (Generation log) is next. Andrew chose a Log tab (2026-11-04): a list, not a JSON viewer. The last Run fills it, whether that Run was Compose or Preview HTML. The three counts sit on that tab. "Report header" is a band, so the flag is the worst level shown with those counts, not a mark on the report-header band and not a mark on the report list.
- The pinned rules: `log` is `{ level, message }` entries; levels are `info`, `warn`, and `error`; Lithium counts them and does not expect a second counts object; the counts are session state and are not columns on `reports`; any error flags the open report as an error, otherwise any warn flags it as a warning, otherwise any info flags it as info, and zero entries show no flag.
- The missing-sample check is 26.3: one warn entry, a warn count of 1, and the warn flag. A clean detail preview shows an empty log and no flag.
- Leave `acuranzo_1455.lua` as written. Next free Acuranzo file is **1456**. QueryRef **175** stays unused.

---

## Phase 26 — Generation log

### Goal

A generation shows its info, warn, and error messages, and the open report shows the three counts.

### Entry gate

Phase 25 Status complete. Andrew chose the panel on 2026-11-04.

### Decided

A **Log** tab on the main page bar, after Preview HTML and before Preview PDF. It uses the Definition page's CodeMirror host and footer. The buffer is aligned text, not JSON, in Vanadium Mono. The first two lines are the counts, in the order info, warn, error, with a flag column only when a level fired. Then each entry is `level` and `message` in those two columns. The last Run fills the buffer, whether that Run was Compose or Preview HTML. The flag is the worst level, shown on the count line and as the tab's color: any error, otherwise any warn, otherwise any info. An empty log shows no flag. Switching reports clears the buffer and the counts. A new report clears them too. The counts are session state. They are not columns on `reports`. Compose still shows `log` inside its JSON. Preview HTML still shows the page.

### Work items

- [x] 26.1 The preview keeps the script's `log` array. The Log tab lists each entry's level and message as aligned CodeMirror text. Lithium counts `info`, `warn`, and `error`, in that order, on that tab. The last Run fills the list, from Compose or from Preview HTML.
- [x] 26.2 A non-zero error count flags the open report as an error. A warn count with zero errors flags it as a warning. Info only flags it as info. Zero entries show no flag. The flag is the Log tab's color and the flag column. The counts are session state. They are not columns on `reports`.
- [x] 26.3 Browser: Andrew said "Ok, I think that is working" (2026-11-04). That covers the missing-sample warn and a clean detail preview. He did not quote the warn line, a warn count of 1, or a zero dump.

### Done means

The operator can read the generation messages and see the three counts on the open report.

### Exit gate

Lint and the browser check.

### Status

**Complete** (2026-11-04). Andrew said "Ok, I think that is working." That is the browser check for 26.3. He did not quote the warn line, a warn count of 1, or a clean-detail zero dump. No new file. No new QueryRef. Next free Acuranzo file is **1456**. QueryRef **175** stays unused.

### Accomplished

- Log tab after Preview HTML. The page reuses `.reports-json-wrap` and `.reports-json-host`, with the editor footer. The buffer is aligned text from [`/elements/003-lithium/src/managers/reports/reports-log.js`](/elements/003-lithium/src/managers/reports/reports-log.js). The face is Vanadium Mono. Word wrap is off.
- Compose Run and Preview HTML Run both call `showGeneration`. A failed run leaves the previous log. Switching to another report, and New, call `clearGeneration`. Saving a new report keeps the log and adopts the new id.
- The flag is `data-generation-flag` on `#reports-log-tab` (`error`, `warn`, or `info`) and a flag column on the count line. An empty log has neither.
- `npx vitest run tests/unit/reports-log.test.js tests/unit/reports-chrome.test.js tests/unit/reports-preview.test.js` — 3 files, 26 passed. ESLint on the touched files passed. `npm run lint:css` passed. `npm run templates:copy` copied 139 templates. `reports.js` is 1000 lines.

### Lessons learned

- The log is one array. Compose already shows it inside the JSON. The Log tab is the same entries as aligned columns, not a second log and not a second JSON viewer.
- `language: 'log'` in CodeMirror is SQL highlighting. Aligned text uses `plain`, which adds no syntax colors.
- `reports.js` is at the 1000-line cap. The Log page lives in `reports-log.js`. The CSS lives in `reports-log.css`. The in-progress note said 999 lines. Restoring the header comment put it back at 1000.
- Andrew's close did not quote the missing-sample line. The unit test covers the column shape. The live warn text stays the Lua string in `acuranzo_1455.lua`: `datasource NAME has no array in sources; no rows`.

### Handoff

- Phase 27 (Over-cap preview) is next and has not started. Its entry gate is this phase complete. Phase 18 is already applied.
- When the inline params would exceed `ClientInvokeMaxParamsBytes` (default 256 KiB), save the definition through the existing save (QueryRef 168) and call `Reports.Render` with `report_id` and `rev_id` instead of `definition`. One toast names that path. A failed save leaves the canvas unchanged. Do not send the session JWT. Do not send `params._hydrogen`.
- The live script still requires `definition`. `acuranzo_1455.lua` reads `P.definition` and fails when it is not a table. The call-shape lock says a stored report may send `report_id` and `rev_id` and the script loads the row. That load is not in 1455. Phase 27's body does not name a migration. The next sitting has to put the load in a new file or the `report_id` call fails. Do not edit `acuranzo_1455.lua`. Next free Acuranzo file is **1456**.
- Sources stay inline. A datasource that itself exceeds the cap is a later stored-blob phase. Raising the params cap to 10 MiB is operator `hydrogen.json` and is not Phase 27. The result cap stays 1 MiB.
- The phase text names preview. Compose sends the same definition and hits the same cap. The toast sentence is not pinned.
- QueryRef **175** stays unused. No new lookup.

---

## Phase 27 — Over-cap preview

### Goal

A definition that does not fit in the params cap still previews, by
saving first and rendering by `report_id`.

### Entry gate

Phase 26 Status complete. Phase 18 is already applied.

### Decided

Andrew chose the toast and the scope on 2026-11-04. The title is
`Saved to preview`. The description is `This report is larger than the
256 KiB script limit, so it was saved and previewed from the stored
revision.` Preview HTML is the only Run button that takes this path.
Compose of an over-cap definition still fails with `Compose failed`.

Every over-cap Preview Run saves, including a report that is already
saved and not dirty. QueryRef 168 appends a revision. That revision
is the one Render loads. A failed save does not call Render. The
canvas stays as it was. The existing save error toast is the only
toast. Sources stay inline. If the sources alone still exceed the
cap, the save still happens and the preview then fails with
`Preview failed`.

The client measures the compact UTF-8 JSON of the inline params
against 256 KiB minus a 4 KiB reserve, so the server-merged
`_hydrogen` claims still fit. It does not read `hydrogen.json`. An
operator-raised cap still takes this path at 256 KiB.

`acuranzo_1456.lua` is a new file. It does not edit
`acuranzo_1455.lua`. When the call sends `report_id` and `rev_id`
and no definition, the script loads QueryRef 167 through
`H.altquery_sync` on the session database and parses the report
text. An inline definition still wins. A loaded definition whose
detail or group band omits `datasource` takes the first declared
datasource name, the same fill `composeDefinition` applies before
an inline call. The saved file is not rewritten.

### Work items

- [x] 27.1 When the inline params would exceed
       `ClientInvokeMaxParamsBytes`, save the definition and preview
       by `report_id`. One toast names that path. Do not send the
       session JWT as a script param. Landed in
       [`/elements/003-lithium/src/managers/reports/reports-preview.js`](/elements/003-lithium/src/managers/reports/reports-preview.js)
       and [`/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua).
- [ ] 27.2 Browser: an over-cap definition previews after that toast,
       and the canvas is unchanged when the save fails. Waits until
       Andrew applies `acuranzo_1456.lua` and checks
       https://lithium.500courses.com.

### Done means

An over-cap definition previews by `report_id` after one toast.

### Exit gate

Lint and the browser check.

### Status

**In progress** (2026-11-04). 27.1 is in. The browser check is 27.2. It waits until Andrew applies `acuranzo_1456.lua` and previews an over-cap definition. This phase stays in progress until he reports that check. Next free Acuranzo file is **1457**. QueryRef **175** stays unused.

### Accomplished

- Preview HTML measures the inline params. Over 256 KiB minus 4 KiB, it saves through `saveListedReport` (QueryRef 168) and calls `Reports.Render` with `stage=html`, `report_id`, `rev_id`, and `sources`. The call has no `definition`, no `params._hydrogen`, and no session JWT. The code is [`/elements/003-lithium/src/managers/reports/reports-preview.js`](/elements/003-lithium/src/managers/reports/reports-preview.js).
- The toast is `Saved to preview`, shown after the save succeeds and before the script call. A failed save toasts the save error and does not call Render. With no list row, the toast is `Save is not available`. Edit mode stays on. The edit snapshot is retaken so the footer matches the saved row.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua) copies the 1455 body. Reverse restores that body. The new body loads QueryRef 167 and parses `reports.report`. An inline definition table still wins, so a preview that fits is unchanged.
- `npm test` was 92 files, 1464 passed, 12 skipped. The 12 skipped are `tests/integration/auth.integration.test.js`: the check of https://lithium.500courses.com aborted, so those cases did not run. `npm run lint` and `npm run lint:css` exited 0. ESLint on `reports-preview.js` and `reports-preview.test.js` exited 0. `reports-preview.test.js` has 9 tests. `luacheck` on `acuranzo_1456.lua` reported 0 warnings. Lua 5.5.1 ran the new body: inline `stage=html` matched `detail.html`, the `report_id` load matched that same page, a bad report text failed with `definition is not valid JSON`, an empty session database failed before the query, an inline definition won when ids were also present, and a detail band that omits `datasource` was filled only on the load path.

### Lessons learned

- QueryRef 168 appends a revision on every call. An over-cap preview saves even when the row is clean, because that new `rev_id` is what Render loads.
- `LithiumTable.handleSave` refuses a clean row and toasts `Changes Saved`. The preview path calls `saveListedReport` and leaves edit mode on.
- `reports.report` arrives as text. The script parses it. There is no JSON decoder in the sandbox.
- The saved definition keeps datasource rows and does not fill a band that omits `datasource`. The inline call fills that band in `composeDefinition` and drops the rows. The load path fills the band inside the script so the stored preview matches the designer preview. The saved file stays the export.
- Sources stay on the call. A later phase can store those rows when they alone exceed the cap.

### Handoff

- Apply [`/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1456.lua) before the 500 Courses check. Until that script is the live `Reports.Render`, a `report_id` call fails with `definition must be a JSON object`. Leave `acuranzo_1455.lua` as written. Reverse 1456 restores the 1455 body.
- The check is Preview HTML on a definition whose inline params exceed 256 KiB minus 4 KiB. The toast title is `Saved to preview`. The description is `This report is larger than the 256 KiB script limit, so it was saved and previewed from the stored revision.` The page then renders from the new revision. A failed save leaves the canvas as it was and does not show that toast. A report that was already clean still gains a revision. The list row has to be selected. Compose of an over-cap definition still fails with `Compose failed`.
- Next free Acuranzo file is **1457**. QueryRef **175** stays unused. No new lookup. Phase 28 is already complete. Phase 29 has not started. The next phase starts in a new conversation, after this browser check is reported.

