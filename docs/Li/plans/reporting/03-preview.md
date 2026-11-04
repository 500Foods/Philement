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

**packet handed over** (acuranzo_1454, 2026-11-03; next file 1455)

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

