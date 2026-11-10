<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Element review (Static/Text)

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 72–82
**Stop when:** Static Text (lookup 053 key 0) has the reviewed Object,
Position, Fonts, Formatting, and Background properties; the designer
shows borders and eight-handle resize; Overflow replaces Fit Height;
Justified alignment works; font metrics no longer force Helvetica;
and Element Link is a URI anchor in HTML (and PDF when that stage lands).

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

Use the Terminology block in the parent: Category, Kind, Section,
Element, Property. Docs use those words. Code keeps its own names
until that code changes for another reason.

**Entry:** The 2026-11-09 Render split is on disk
(`acuranzo_1467.lua`–`acuranzo_1479.lua`, lookups restored in
`acuranzo_1480.lua`). Andrew recreates the databases. The next free
Acuranzo file is **1481**. QueryRef **175** stays unused. Next free
lookup id **080**. Next free lookup 054 key **53**. Next free lookup
075 key **17**. Phases 53 and 54 may still be in progress; this
milestone does not wait on them. Naming Phase 72 starts the review.
**Next file:** None yet. Later Kind reviews get their own milestone
files when Andrew opens them.

## Shared context (read before Phase 72)

Andrew's Static/Text decisions (2026-11-09):

- Justified is yes, late Phase 81.
- Overflow replaces Fit Height (and Fit comes off Static Text).
- A Units Section on the Report Element holds Position Units, Font
  Units, and Page Units (054 key 34 moves into it).
- Shared Background Color on every Kind, including shapes; migrate
  saved shape fill colours onto it; Fill Style stays shape-specific
  (pinned — see Pinned decision below; Phases 75/77).
- Font Family, Font Size, and Font Style stay three separate
  dropdowns. Each list previews only its own value (Family shows
  each name in that face; Size shows each option at that size; Style
  shows each in that style). Size/Style do not use the chosen
  family; Family does not use size/style.
- **Custom dropdowns (2026-11-09).** Properties Panel lists use the
  shared `customDropdownEditor` — **never** a native `<select>`
  (see parent Property panel lock). Pick-only lists of 2–7 options
  double-click cycle without opening the menu.
- No generated width tables. Phase 79 measures at runtime; server
  TTF/OTF live on the filesystem under a configurable root, with
  Helvetica AFM fallback.

Bug root causes already traced (confirm in a browser before closing
the phase that cites them):

| Bug | Where | Cause |
| --- | --- | --- |
| Home/End in single-line Text | `lithium-properties.js` default `input` editor (fixed Phase 72: same `stay` Set as memo ~909–926). Was bubbling to table nav (`lithium-table-ops.js` Home/End) | Home/End reached the table before stopPropagation |
| V. Align ignored on default text | Designer: `reports-static.css` only styles `.reports-item-wrapped[data-valign=…]`. Render: `acuranzo_1478.lua` (`Render.Html`) applies `offsetPt` only on the `o.lines` branch; un-measured text skips it. Layout only builds lines when fit is not `clip` (`acuranzo_1469.lua` / Lithium `model/wrap.js`) | Default Fit Height is `clip`, so no lines, so no valign |
| Fit Height switches font to Helvetica | Lithium `reports-static.js` ~486–507 and `model/wrap.js` (~460); Render `acuranzo_1472.lua` (`Render.Paging`) sets `copy.font.family = "Helvetica"` when `lay.grown`; `acuranzo_1469.lua` (`Render.Layout`) is Helvetica AFM only | Only Helvetica advances exist today |

Lookup snapshot for Phase 75:

- Lookup 075 keys 1–16 are taken (1–9 from 1440, 10–12 Stroke/Fill/Corners
  from 1460, 13–16 Barcode from 1465). Next free **17** (Units), then
  **18** (Background).
- Lookup 054 next free key **53**. Key 4 is Align (Position today;
  List Left/Center/Right from 1437). Key 5 is Fit. Key 34 is Page
  Units (Page Layout section 2 today). Keys 46/47 are Fill Style /
  Fill Color (Fill section 11).
- Static Text (053 key 0) attributes still include Fit (5) from
  1436. 1480 updated 053 keys 1–4 only.

### Tentative Render migrations (rule 12)

One module per migration. `sql_old` is that module's current body
from the newest migration that wrote it. Re-check the next free
number on disk before writing. Numbers below assume 1481 is the
lookup packet and nothing else lands first.

| File | Module | First needed by |
| --- | --- | --- |
| `acuranzo_1481.lua` | Lookups only (075/054/053). No Render | Phase 75 |
| `acuranzo_1482.lua` | `Render.Items` | Phase 77 (fore/back/background on item_out) |
| `acuranzo_1483.lua` | `Render.Html` | Phase 77 (CSS colours); Phase 78 may amend Html again |
| `acuranzo_1484.lua` | `Render.Shapes` | Phase 77 (read shared Background Color; Fill Style patterns stay) |
| `acuranzo_1485.lua` | `Reports.RewriteBackgroundColor` | Phase 77 (one-shot reports.report rewrite; invoke once) |
| `acuranzo_1486.lua`–`acuranzo_1500.lua` | Lookup 080 Named Colours | Phase 77 (CSS + xkcd seed; 15 batches) |
| `acuranzo_1501.lua` | `Render.Items` | Phase 78 (`item_out` always carries `valign`) — **applied** |
| `acuranzo_1502.lua` | `Render.Html` | Phase 78 (flexbox V. Align on non-lines text) — **applied** |
| `acuranzo_1503.lua` | Lookup 054 key 4 | Phase 78 (Label Align → H. Align) — **applied** |
| `acuranzo_1504.lua` | `Render.Metrics` **insert** (lazy) | Phase 79 |
| `acuranzo_1505.lua` | `Render.Layout` | Phase 79 (measure_family → Metrics) |
| `acuranzo_1506.lua` | `Render.Paging` | Phase 79 (drop grown → Helvetica) |
| Phase 80 lookup | 054 Overflow list + Min/Max Font Size | Next free after Phase 79 (**1507**) |
| Phase 80 Render | `Layout` / `Paging` / `Items` / `Html` as needed | Overflow Clip/Ellipsis/Shrink/Grow/**Fit**; rule 12 |
| later | as needed | Phase 81 Justified |
| TBD after 75–81 | `Render.Core` (`safe_uri`) | Phase 82 |
| TBD | `Render.Items` (carry `link` on placed item) | Phase 82 |
| TBD | `Render.Html` (`<a href>`) | Phase 82 |
| TBD | PDF emit module (`doc:link`) | Phase 82 when `stage=pdf` exists |

If a phase touches fewer modules, skip the unused numbers and keep
the sequence dense. Never edit an applied Render body in place.
Phase 82 file numbers are assigned only after Phases 75–81 have
taken their slots; re-check the next free Acuranzo file on disk.

### Pinned decision (Background / Fill) — Andrew 2026-11-09

A **new shared Background Color** Property (new lookup 054 key) is
on **every** Element Kind, including shapes. It paints the element
box. Fill Style (054 key 46) and gradient/pattern details stay
**shape-specific**. Fill Color (054 key 47) is retired from the
panel once Background Color lands; shapes no longer have a separate
fill-colour row.

Text Back Color (glyph / highlight) and Background Color (full box)
are two Properties. Fore Color is text colour.

**Saved-report migration** (detail in Phases 75 and 77): each shape
item's fill colour moves onto `backgroundColor`. `fillStyle` and
gradient stops/angle stay. Where the colour lived:

| Today (shape item JSON) | After |
| --- | --- |
| `fillPaint.color` (solid) | `backgroundColor` (hex string) |
| `fillPaint.stops[0].color` (linear/radial) | `backgroundColor`; stops keep their own colours for the gradient ramp unless a later pass unifies them — v1 copies stop 0 into `backgroundColor` and leaves `fillPaint` stops as stored |
| `fillStyle` | unchanged |
| no `fillPaint` | omit `backgroundColor` (designer default) |

Definitions live in `reports.report` (JSON text) per
`report_id`/`rev_id`. See Phase 77 for load-time normalize vs
one-shot rewrite.

---

## Phase 72 — Home/End in the single-line editor

### Goal

Home and End move the caret inside a single-line Properties Panel
editor (Static Text Text, and every other single-line text Property).
They do not jump the report table or the outline.

### Entry gate

Andrew names Phase 72. No migration. Lookup and Render packets are
unchanged this phase.

### Work items

- [x] 72.1 In `lithium-properties.js`, give the default single-line
      `input` editor the same navigation `stopPropagation` set the
      memo editor already uses (`Arrow*`, `Home`, `End`, `PageUp`,
      `PageDown`). Keep Enter = commit and Escape = cancel.
- [~] 72.2 Vitest for Home/End stopPropagation — deferred; smallest
      change this sitting was the editor fix. Add with Phase 73 or
      when Andrew asks.
- [x] 72.3 ESLint on `src/properties/lithium-properties.js` (exit 0).
      Full `npm test` not re-run this sitting.
- [x] 72.4 Browser: select a Static Text Element, edit Text, press
      Home and End. Caret moves in the field. The table selection
      does not jump. Andrew confirmed 2026-11-09 ("That looks good").

### Done means

Home and End move the caret in every single-line PP text editor.

### Exit gate

Lint on the touched file is green. Andrew confirmed the browser
check (72.4) on 2026-11-09.

### Status

**complete** (2026-11-09)

### Accomplished

- Single-line `input` editor in
  `elements/003-lithium/src/properties/lithium-properties.js` now
  `stopPropagation`s the same navigation keys as the memo editor
  (`Arrow*`, `Home`, `End`, `PageUp`, `PageDown`), plus Enter and
  Escape so the table/outline does not take them.
- `npx eslint src/properties/lithium-properties.js --no-warn-ignored`
  exited 0 (no pre-existing errors on this file).
- Andrew validated Home/End in the browser (2026-11-09).

### Lessons learned

- The memo path already had the right set; copying that `stay` Set
  onto the default input is enough. No properties-panel or table
  change was required.

### Handoff

Phase 73. No packet. 72.2 Vitest stays deferred.

---

## Phase 73 — Borders toggle

### Goal

A Borders toolbar toggle (default on) draws a light silver outline
around every Element. The selected Element keeps its blue outline.
The toggle sits with Grid, Snap, Ruler, and Crosshair and is
session-only like them.

### Entry gate

Phase 72 Status complete.

### Work items

- [x] 73.1 Add a Borders button after Crosshair in `reports.html`
      (`fa-border-outer`, title "Show element borders"). Wire
      `bordersBtn` in `reports.js`; `_bindBorders` /
      `_syncBorders` / `_unbindBorders` in `reports-pointer.js`
      (session flag `_bordersOn` default true, `aria-pressed`,
      stage class `reports-borders-off` when off — same pattern as
      Grid/Crosshair).
- [x] 73.2 CSS: `.reports-stage:not(.reports-borders-off)
      .reports-item:not(.reports-item-selected)` gets
      `outline: 1px solid silver` (offset -1px). Selected keeps
      `var(--accent-info)`. Outline only — no layout change. Off
      removes silver only.
- [x] 73.3 Default on. No document field. No migration. Session
      only, like Grid/Snap/Ruler/Crosshair (not localStorage).
- [x] 73.4 ESLint on `reports.js` and `reports-pointer.js` (exit 0).
      `npm run templates:copy` ran (143 templates). Full suite /
      lint:css not required this sitting beyond the touched files.
- [x] 73.5 Browser: Borders on shows silver on unselected items;
      selected stays blue. Toggle off hides silver. Grid/Snap/Ruler/
      Crosshair still work. Andrew confirmed 2026-11-09.

### Done means

Borders toggle defaults on; silver on every Element; selected stays
blue.

### Exit gate

Lint green. Andrew confirms the browser check (73.5). Phase stays
in progress until he does.

### Status

**Complete (2026-11-09)**

### Accomplished

- Borders toolbar button after Crosshair; session toggle defaults
  on; stage class `reports-borders-off` when off.
- Silver `outline` on unselected items; selected blue outline wins.
  Does not affect box size or Render output.

### Lessons learned

- Outline (not border) keeps the box size unchanged for Render and
  snap.

### Handoff

Phase 74. No packet.

---

## Phase 74 — Resize from all sides and corners

### Goal

An Element resizes from eight handles (four sides, four corners).
The old bottom-right-only square is gone. A drag that changes top or
left is one undo step with the size change.

### Entry gate

Phase 73 Status complete.

### Work items

- [x] 74.1 Replace `.reports-item-corner` with eight handles in the
      item chrome (`reports.css`, `appendResizeHandles` in
      `items.js` / `reports-static.js`). Cursors: ns, ew, nesw,
      nwse per edge.
- [x] 74.2 `reports-pointer.js` `_beginItemDrag(event, itemEl,
      edges)` via `edgesFromHandle`. `dragItemBox` /
      `snapItemBox` take `{n,s,e,w}`. Top/left+size commit through
      `moveItem` then `resizeItem`.
- [x] 74.3 One `_commitDocument` on pointerup for the whole resize,
      including origin moves. Snap still applies when Snap is on.
- [x] 74.4 Dropped `.reports-item-corner`. Handles are 8×8 px
      (same as the old corner), centered on each edge/corner.
- [x] 74.5 Vitest covers `edgesFromHandle`, north/west
      `dragItemBox`, and north/west `snapItemBox`. ESLint on
      touched files; `templates:copy` when HTML/CSS public sync
      needed.
- [x] 74.6 Browser: resize from each handle; Undo restores the
      prior box in one step; Snap still feels right.
      Andrew confirmed sizing and undo 2026-11-09.
- [x] 74.7 Triangle chrome (CSS only): `clip-path` on
      `::before`; full 8×8 hit target stays transparent.
      Corners keep the outward half; edges base on the
      element-edge midline with apex on the inward side.
      Blue and cursors unchanged. Geometry noted in
      `reports.css` and Accomplished.
- [x] 74.8 Browser: triangle handles look right; grab still
      easy. Andrew confirmed 2026-11-09.

### Done means

Eight handles resize the Element; top/left drags undo as one step;
the old corner square is gone.

### Exit gate

Tests, lint, and Andrew's browser check.

### Status

**Complete (2026-11-09)**

### Accomplished

- Eight AABB handles on every painted item; old corner gone.
- Edge-aware drag/snap; n/w resize is one undo step.
- Rotation: handles stay on the axis-aligned box; content
  rotation (image CSS / shape SVG) paints inside that box and
  does not reorient the handles.
- Triangle chrome: handles are still 8×8 hit squares centred
  on the edge/corner (`-4px` offset → centre on the edge).
  Visible fill is `::before` + `clip-path` (handle background
  transparent so the full square stays grabable). Corners keep
  the outward diagonal half (nw `polygon(0 0,100% 0,0 100%)`,
  ne / sw / se likewise). Edges: base on the handle midline
  (element edge) and apex at the inward mid-side (n
  `polygon(0 50%,100% 50%,50% 100%)`, and mirrors for s/e/w).
  Corner triangles were invisible until
  `.reports-item-selected { overflow: visible }` — item
  `overflow:hidden` clipped the outward halves; edge triangles
  face inward so they already showed. `.reports-item-text`
  still clips label overflow.

### Lessons learned

- Corner chrome must use inward clip-paths (right angle at the
  handle centre). Outward halves are clipped by item
  `overflow:hidden`. Cascade: prefer clip-path-only on corners
  so `.reports-item-selected .reports-item-handle::before
  { inset: 0 }` does not fight inset overrides.

### Handoff

Phase 75. Packet `acuranzo_1481.lua`. No Render module.

---

## Phase 75 — Lookup migration 1481 (Units, Background, Overflow)

### Goal

Hand Andrew one lookup packet that adds the Units and Background
Sections, the new Properties, moves H. Align and Page Units, removes
Fit from Static Text, and adds Overflow. No Render body change in
this packet.

### Entry gate

Phase 74 Status complete. Next free Acuranzo file on disk is 1481
(re-check). Background/Fill is pinned (shared Background Color;
see Pinned decision above).

### Work items

- [x] 75.1 Lookup 075: insert key **17** Units and key **18**
      Background (icons in `collection`). Place Units where the
      Report Element's unit Properties will group; Background after
      Formatting or as agreed in the sort_seq.
- [x] 75.2 Lookup 054: add Position Units and Font Units (new keys
      from **53** upward). Move Page Units (key 34) into the Units
      Section (`section` 17, Group Units). Defaults: Position `in`,
      Font `pt`, Page Units keeps its list.
- [x] 75.3 Lookup 054 key 4 (Align): `section` → Formatting (7);
      List becomes Left, Center, Right, Justified. Justified is
      stored now; emit is Phase 81.
- [x] 75.4 New 054 keys: Fore Color, Back Color, Background Color
      (Color editors). Background Color is on **every** Kind,
      including shapes (new key from 53 upward). Retire Fill Color
      (key 47) from shape `attributes` lists once Background Color
      is listed; Fill Style (key 46) stays shape-specific. Put
      Background Color in the Background Section (075 key 18).
- [x] 75.5 New 054 Overflow Property. List: Clip, Ellipsis, Shrink,
      Grow. Default Clip. Section Formatting.
- [x] 75.6 Lookup 053 key 0 (Static Text): remove Fit (key 5) from
      `attributes`; add the new Property keys; keep ID, Text, Link,
      Position, Fonts, Align, Rotation, etc. Do not strip Fit from
      image Kinds that still use 054 key 5.
- [x] 75.7 Packet `acuranzo_1481.lua` only. Reverse restores prior
      collections and deletes new 075/054 keys. QueryRef 175 unused.
      No Render module.
- [x] 75.8 Hand over. Andrew applies. `tests/test_31_migrations.sh`
      when he asks. Lithium may still ignore unknown keys until
      later phases.

### Done means

After apply, lookups show Units and Background Sections, Static Text
has no Fit, Align lists Justified, Overflow exists, Page Units sits
under Units, and Background Color is on every Kind's attribute list
(shapes included; Fill Color dropped from those lists).

### Exit gate

Andrew reports 1481 applied. Test 31 when he asks.

### Status

**Complete (2026-11-09)**

### Accomplished

- Wrote `acuranzo_1481.lua` (lookups only). test_31 SQL gen + sqruff OK for 1481.
  Fill Color retired from shape attribute lists; saved-shape colour
  rewrite remains Phase 77.
- Andrew applied 1481 (2026-11-09); new rows visible and inert as
  expected until Lithium Phase 76+.

### Lessons learned

- Lookup-only packets can land before Lithium knows the keys; the
  panel simply ignores unknown editors until the matching phase.

### Handoff

Phase 76. Shape fillPaint → `backgroundColor` rewrite is Phase 77.
Next free Acuranzo after 1481.

---

## Phase 76 — Units defaults, display format, font dropdown previews

### Goal

Position lengths default to the report Position Units (`in`). Font
Size defaults to Font Units (`pt`). The PP shows lengths and sizes
as `#0.## unit` with a space before the unit; stored values keep
full precision. Font Family, Font Size, and Font Style stay three
separate dropdowns with independent previews.

### Entry gate

Phase 75 complete (1481 applied).

### Work items

- [x] 76.1 Report Element: Units Section shows Position Units, Font
      Units, and Page Units. Changing Position Units does not
      rewrite stored item strings; it only changes the default for
      bare numbers and the display unit when a value has no unit.
- [x] 76.2 Display-only format `#0.## unit` for Top, Left, Width,
      Height, Font Size, and other length Properties that use the
      unit helpers in `report-properties.js`. Parsing still accepts
      a bare number (report/position/font unit as appropriate) and
      keeps precision in the definition JSON.
- [x] 76.3 Font Size: editable combo. List 8, 10, 12, 14, 16, 18,
      20, 22, 24, 28, 32, 40, 48, 64. Each option's label renders at
      that size. Default unit `pt`.
- [x] 76.4 Font Family list: each option's label uses that family
      face. Font Style list: each option uses that style
      (Normal/Bold/Italic/Bold Italic). Previews are independent —
      Size/Style do not adopt the selected Family; Family does not
      adopt Size/Style.
- [x] 76.5 Not a single combo widget. Three Properties, three
      editors (`lithium-properties.js` list/select paths as needed).
- [x] 76.6 Vitest for format/parse helpers. `npm test`, lint.
- [x] 76.7 Browser: units display, bare-number commit, and the
      three dropdown previews.

### Done means

PP shows `#0.## in` / `#0.## pt` as appropriate; the three font
dropdowns preview independently; stored JSON is unchanged in
precision.

### Exit gate

Tests, lint, Andrew's browser check. No Render packet required
unless a helper must move server-side (prefer Lithium-only).

### Status

**Complete (2026-11-09)**

### Accomplished

- Units Section on Report: Position Units (default `in`), Font Units
  (default `pt`), Page Units (key 34) bound. Bare Top/Left/Width/Height
  use Position Units; Font Size uses Font Units; page paper lengths use
  Page Units. Changing units does not rewrite stored strings.
- PP display-only `#0.## unit` via `displayLength` / row `display`;
  `formatLength` for save untouched.
- Font Family / Size / Style: three separate editors with independent
  previews. Font Size is an editable combo (preset list + free entry);
  Home/End stay in the field (Phase 72).
- Vitest for displayLength and units resolution; ESLint on touched
  files.
- Browser fix (2026-11-09): FontFamily/FontStyle now keep lookup
  `List` (editor names are not `List`); Font Size menu mounts on
  `document.body` (tableholder was clipping); combo input gets the
  same `color: inherit` rule as direct-child editors.
- All PP List/PageSize/Boolean rows use `customDropdownEditor`
  (pick-only; plain option text). Pick-only lists of 2–7 options double-click
  cycle. Standing rule recorded in parent + this Shared context.

### Lessons learned

- Custom dropdowns belong in `lithium-properties.js` once, not
  per Kind. Pick-only lists of 2–7 options cycle on double-click;
  editable lists (Font Size) do not.
- Nested PP editors need the same `color: inherit` rule as
  direct-child inputs; body-mounted menus avoid Tabulator clip.

### Handoff

Phase 77. Lookups 054 keys 55–57 already from 1481. Next free
Acuranzo **1482** (Render.Items).

---

## Phase 77 — Fore, Back, and Background colours

### Goal

Fore Color (text), Back Color (text background), and Background
Color (element box) paint in the designer and in `stage=html`.
Background Color is shared by every Kind, including shapes. Fill
Style patterns stay shape-specific and read the shared background
colour. Saved shape fill colours are migrated onto
`backgroundColor`.

### Entry gate

Phase 76 Status complete. 1481 applied.

### Where saved definitions live

- Table `reports`, column `report`: the definition JSON text for
  one revision (`report_id` + `rev_id`). Phase 17 QueryRefs
  read/write that column. See parent lock `reports` columns.
- On disk fixtures under
  `elements/003-lithium/tests/fixtures/reporting/*.definition.json`
  and any exported `.json` files authors keep locally.
- In memory: the open designer `doc` (bands → items). Shape items
  today store `fillStyle` and `fillPaint`
  (`{ kind, color? }` or `{ kind, angle?, stops:[{at,color}] }`).

### Migration approach (shape fill colour → Background Color)

Do **both** of the following in this phase so old rows and new
edits agree:

1. **Load-time normalize (Lithium, required).** When opening a
   definition (import, open revision, paste JSON), for every shape
   item: if `backgroundColor` is missing and `fillPaint` has a
   colour, set `backgroundColor` from `fillPaint.color` (solid) or
   from `fillPaint.stops[0].color` (gradient). Leave `fillStyle`
   and the rest of `fillPaint` (kind, angle, stops) in place.
   Saving then persists `backgroundColor` on the next revision.
2. **One-shot rewrite (Helium packet, required).** A migration
   walks `reports.report` for every revision, parses the JSON, runs
   the same per-item map, and writes the text back. Reverse is a
   restore from a pre-image only if Andrew wants it; otherwise
   document that reverse does not un-migrate colours (forward is
   additive: it only adds `backgroundColor`). Tentative file after
   the Render colour packets, or bundled with the Shapes amendment
   — pin the number when writing. Fixtures in the Lithium tree are
   updated in the same sitting.
3. **Render fallback (same sitting).** `Render.Shapes` /
   `Render.Items` read `item.backgroundColor`, and if absent fall
   back to the old `fillPaint.color` / stop-0 colour so an
   un-migrated row still paints until the rewrite lands.

After migrate, the PP shows Background Color (not Fill Color).
Editing Background Color updates `backgroundColor`; shape pattern
emit uses that hex with `fillStyle` / gradient stops as today.

### Work items

- [x] 77.1 Lithium: wire Fore / Back / Background Color in
      `report-properties.js` / `reports-static.js` / shape panel.
      Designer paints Fore as text colour, Back as text
      highlight, Background as the item box. Shapes use
      Background Color + Fill Style (no Fill Color row).
- [x] 77.2 Lithium load-time normalize as above (open, import).
      Update fixture definition JSON that has shape fills.
- [x] 77.3 Rule 12: amend `Render.Items` and `Render.Html` for the
      three colours. Tentative `acuranzo_1482.lua` Items,
      `acuranzo_1483.lua` Html. `sql_old` = current body.
- [x] 77.4 Amend `Render.Shapes` (`acuranzo_1484.lua`): fill paint
      uses `backgroundColor` with fallback to `fillPaint`; Fill
      Style patterns unchanged.
- [x] 77.5 Helium packet that rewrites `reports.report` JSON for
      every revision (map above). Hand over with the Render
      packets. QueryRef 175 unused.
- [x] 77.6 `render_check.sh`, `npm test`, lint. Andrew applies.
- [x] 77.7 Browser: Fore/Back/Background on Static Text; open an
      old shape report and confirm the colour landed on Background
      Color; Fill Style still patterns. Andrew confirmed 2026-11-09
      after applying 1482–1500.


### Named colour lookup (same phase)

Lookup **080** Named Colours. Seeded with 148 CSS named colours plus
xkcd colour-survey names (CC0, title-cased); CSS wins on duplicate
names. Each row: `value_txt` = name, `code` = `#rrggbb`,
`value_int` = swatch flag (0 seeded default). Migrations
`acuranzo_1486.lua`–`acuranzo_1500.lua` (15 batches; reverse
deletes the batch, 1486 also drops the header). Designer loads via
`PANEL_LOOKUPS.namedColours` (QueryRef 26). Colour PP editor
type-ahead resolves a name to hex; Coloris gains a searchable named
dropdown and flagged rows join the swatch row (theme → named → user).

### Done means

Colours show in the designer and HTML preview; shapes use shared
Background Color with Fill Style; saved shape fills have
`backgroundColor` after open or after the rewrite packet.

### Exit gate

Applied packets, `render_check.sh`, Andrew's browser check.

### Status

**Complete** — Andrew applied `1482`–`1500` and confirmed the
browser check 2026-11-09.

### Accomplished

- Lithium: Fore/Back/Background Color (keys 55–57) in PP; designer
  paints fore/background on the item box and Back Color on inline
  `.reports-item-text-ink` (matches Render `item_back_span`);
  shapes drop Fill Color (47); load-time
  `normalizeDefinitionColours`; fixtures; vitest
  (`reports-item-colours`).
- Canonical stored `none`; Coloris ✓/✕ / none (`fa-ban`) / Name
  tab; PP caret→names; stock Close removed.
- Render: `acuranzo_1482` Items, `1483` Html, `1484` Shapes
  (1.0.1 none); `1485` `Reports.RewriteBackgroundColor`; lookup
  080 `1486`–`1500` (1002 names).

### Lessons learned

- Shared `backgroundColor` replaces shape Fill Color; keep
  `fillPaint` for patterns/gradients and sync stop 0 / solid colour
  when Background Color is edited.
- Named-colour seed is too large for one DB2 literal; split across
  migrations (~70 rows each).
- One-shot `reports.report` rewrite is an invokable script (SQL
  cannot portably walk JSON); Render fallback + load normalize
  cover until invoke.
- Canvas: confirm the paint path sets CSS vars after property
  commit; Back Color must be inline (glyph highlight), not block.
- Coloris cannot parse `none` — keep pending none out of its
  change handler; ✓ writes the linked field.

### Handoff

Phase 78 — V. Align for un-measured (Clip) text. Next free
Acuranzo **1501**. Map row for `1501` Html still accurate; Items
may also need a packet (see Phase 78 plan).

---

## Phase 78 — V. Align for un-measured text

### Goal

Vertical align top/middle/bottom works when text is not broken into
measured lines (default Clip / no `o.lines`), in both the designer
and `Render.Html`.

### Entry gate

Phase 77 Status complete.

### Work items

- [x] 78.1 Designer: simple path sets `data-valign`; CSS flexes any
      `.reports-item[data-valign]` (not only wrapped); text face
      `width:100%` so H. Align positions the ink span. Labels
      **H. Align** / **V. Align**.
- [x] 78.2 `Render.Html` non-lines: flexbox `justify-content` from
      `o.valign`; lines branch keeps `offsetPt`. Font CSS on both
      branches unchanged (uses `o.font`).
- [x] 78.3 Rule 12: `acuranzo_1501` Items (`valign` on `item_out`),
      `acuranzo_1502` Html (flex), `acuranzo_1503` lookup 054 key 4
      → H. Align. Unapplied.
- [x] 78.4 Vitest (`reports-valign-font`); fixture `valign9`
      (9 H×V Clip cells + frame); `render_check.sh` run.
- [x] 78.5 Font bug: non-Clip canvas no longer forces Helvetica for
      display (`fontOf(..., false)`); AFM still used only inside
      `wrapText` until Phase 79.

### Done means

A Clip Static Text with V. Align middle or bottom is centred or
bottom-aligned in the designer and in Preview HTML. Nine H×V
positions work with Back Color on glyphs and Background on the box.

### Exit gate

Packets applied, checks green, Andrew confirms.

### Status

**Complete (2026-11-09)** — Andrew applied `1501`–`1503` and moved on
(wrapping close enough).

### Accomplished

- Designer Clip path `data-valign` + flex CSS; ink span H. Align;
  V. Align / H. Align labels (client + lookup 1503).
- Font: chosen family on canvas for Clip and non-Clip.
- Render: Items carries `valign`; Html non-lines flexbox.
- Fixture `valign9`; vitest; sizecheck OK.

### Lessons learned

- Non-Clip canvas font flip was `fontOf(doc, item, helvetica=true)`
  when Fit Height ≠ Clip — display used Helvetica AFM family.
  Measure can stay Helvetica AFM; paint must use the item font.
- Clip `item_out` lacked `valign` because Paging only merges layout
  fields when `lines` exist; Items must copy `valign` itself.

### Handoff

Phase 79 next. Packets `1504`–`1506` followed.

---

## Phase 79 — Runtime font metrics (no width tables)

### Goal

Designer and Render measure text with the real font. No generated
per-font width tables are committed. Helvetica forcing on grow /
measure paths is removed. Helvetica AFM remains the fallback when a
font file cannot be opened.

### Entry gate

Phase 78 Status complete. Font-file prerequisite is agreed (see
below).

### Design (Andrew, 2026-11-09)

- **Do not** generate or commit width tables. Tables make adding a
  font a rebuild step.
- **Lithium:** measure at runtime with canvas `measureText` using
  the Element's family, style, and size.
- **Render:** new lazily loaded `Reports.Render.Metrics` reads
  advances from the actual font bytes (TTF/OTF hmtx/cmap via
  pdfkit's fontkit), ideally calling pdfkit helpers directly.
- **Fallback:** existing Helvetica AFM path in `Render.Layout` /
  Lithium `model/afm-helvetica.js` when the font cannot be loaded.

### Prerequisite — font files on both sides

Every selectable Font Family needs bytes Lithium can load (browser
`@font-face`) and bytes the server can parse.

Today Lithium only ships:

- `elements/003-lithium/public/assets/fonts/VanadiumSans-SemiExtended.woff2`
- `elements/003-lithium/public/assets/fonts/VanadiumMono-SemiExtended.woff2`

pdfkit does **not** parse WOFF2 (brotli). Server-side needs **TTF,
OTF, or WOFF1** for each face (and each style if they are separate
files). Lithium may keep woff2 for display if matching TTF/OTF are
on the server.

**Server location (Andrew, 2026-11-09): filesystem.** Render.Metrics
resolves a face to a file under a **configurable directory** (Hydrogen
/ host setting or script param — pin the exact knob when writing the
packet; do not hard-code a developer path). Example layout:
`<fontRoot>/VanadiumSans-SemiExtended.ttf`. If the file is missing or
unreadable, fall back to Helvetica AFM. Do not load fonts from the
DB or from the Lua payload for v1 metrics.

### pdfkit findings (read-only, 2026-11-09)

Source of truth for the port: `/mnt/extra/Projects/lua-pdfkit`
(same code is vendored at
`elements/001-hydrogen/hydrogen/payloads/lua/pdfkit`).

**Formats parsed** (`pdfkit.fontkit`):

| Format | Module | Notes |
| --- | --- | --- |
| `.ttf` / `.otf` | `pdfkit.fontkit.TTFFont` | SFNT; OTF may be CFF outlines |
| WOFF1 | `pdfkit.fontkit.WOFFFont` | zlib-wrapped SFNT; inflated then TTF/CFF path |
| `.ttc` | `pdfkit.fontkit.TrueTypeCollection` | pick face via `postscriptName` |
| WOFF2 | — | **Not supported** (explicit in fontkit `init.lua` and README) |
| Standard 14 | `pdfkit.font.standard` + `pdfkit.font.afm` + `pdfkit.font.generated.*` | Generated AFM metrics; no file |

**Without creating a PDF document (preferred for Metrics):**

- `local fontkit = require("pdfkit.fontkit")`
- `fontkit.create(buffer, postscriptName?)` → font
- `fontkit.openSync(filename, postscriptName?)` → font
- `font:layout(str, userFeatures?, script?, language?, direction?)`
  → GlyphRun; `run:advanceWidth()` (font units)
- `font:ascent()`, `font:descent()`, `font:lineGap()`,
  `font:unitsPerEm()`
- `font:glyphForCodePoint(cp)` → Glyph; `glyph:advanceWidth()`
- Standard 14 without a doc:
  `local AFM = require("pdfkit.font.afm")`
  `local metrics = AFM.new(require("pdfkit.font.generated.Helvetica"))`
  then `metrics:glyphsForString(str)`, `metrics:widthOfGlyph(g)`,
  `metrics:advancesForGlyphs(glyphs)` (widths in 1/1000 em).
  `require("pdfkit.font.standard").isStandardFont(name)` tests the
  fourteen names.

**Document-tied helpers (work, but need a `PDFDocument`):**

- `PDFDocument.new(options)` → `doc`
- `doc:font(src, family?, size?)` — `src` is a standard name, a
  filesystem path, or raw bytes (`pdfkit.font_factory.open`)
- `doc:registerFont(name, src, family?)`
- `doc:fontSize(size)`
- `doc:widthOfString(str, options?)` —
  `mixins/text.lua` `M.widthOfString(doc, str, options)`; uses
  `doc._font:widthOfString(str, doc._fontSize, options.features)`
- `doc:currentLineHeight(includeGap?)` →
  `doc._font:lineHeight(doc._fontSize, includeGap)`
- `FontFactory.open(document, src, family, id)` →
  `StandardFont` or `EmbeddedFont`
- `StandardFont:widthOfString(str, size)`
- `EmbeddedFont:widthOfString(str, size, features?)`
- `Font:lineHeight(size, includeGap)` uses `ascender`,
  `descender`, `lineGap` (thousandths of an em)

`StandardFont.new` / `EmbeddedFont.new` / `FontFactory.open` all
take a `document`. Measurement math itself does not need pages, but
those constructors expect one. Prefer fontkit + AFM for
`Render.Metrics` so compose can measure without allocating a PDF.

### Work items

- [x] 79.1 Server font root: default `/usr/share/fonts/vanadium`
      (`REPORTS_FONT_ROOT` / `params._hydrogen.fontRoot`). Option B:
      Vanadium Sans/Mono → full `*SemiExtended*.ttf` (same width as
      designer `@font-face`). Italic → upright SemiExtended (no
      italic file). Bold / Bold+Italic → SemiExtendedMedium.
      Mail SemiExtended = future one-line map swap (option C).
- [x] 79.2 Lithium `model/wrap.js`: canvas `measureText` with
      family/style/size; AFM Helvetica fallback (no canvas /
      Helvetica family). `ensureFontsReady`. Drop grown→Helvetica.
- [x] 79.3 Insert `Reports.Render.Metrics` (`acuranzo_1504`).
      cmap `glyphForCodePoint` + `advanceWidth` (not `font:layout` —
      Vanadium GSUB errors in lua-pdfkit). Ligatures not applied.
      Per-process face cache. AFM fallback.
- [x] 79.4 Loader: **no amend**. Layout calls `need_module("Metrics")`
      on first non-Helvetica measure (lazy).
- [x] 79.5 Amend Layout (`1505`) + Paging (`1506`): measure_family;
      drop grown→Helvetica.
- [x] 79.6 Fixture `wrap-vanadium`; harness lists Metrics;
      vitest; ESLint. Packets unapplied.
- [ ] 79.7 Browser: Vanadium grow no longer flips to Helvetica;
      Preview HTML line breaks match designer for SemiExtended faces.

### Done means

Measure uses the real font in the designer and in Render; Helvetica
is only the fallback; no width-table files are in the repo.

### Exit gate

Applied Metrics + Layout/Paging packets, `render_check.sh`,
Andrew's browser check.

### Status

**Complete (2026-11-09)** — Andrew applied through `1511` (Metrics +
`scripts.options` + `Reports.Render` sandbox.io); wrapping close
enough.

### Accomplished

- Benchmark gate: full SemiExtended TTFs; Andrew chose option B.
- `Render.Metrics` insert; Layout/Paging amend; designer canvas
  measure; `wrap-vanadium` fixture.
- `scripts.options` (1507–1511); Hydrogen `create_context_ex`;
  harness `options.json` for sandbox.io.

### Lessons learned

- `font:layout` fails on Vanadium GSUB; use cmap + `advanceWidth`.
- Production sandbox nils `io` unless `scripts.options.sandbox.io`
  (or global AllowIo). Fontkit `openSync` needs `io`. Harness must
  mirror that (not "keep io when Metrics is present").
- Orchestrator does not source per-script limits; options.limits
  feeds ScoreboardJobLimits.

### Handoff

Phase 80 Overflow. Next free Acuranzo **1512**.

---

## Phase 80 — Overflow replaces Fit Height

### Goal

Overflow replaces the Lithium-only Fit Height row on Static Text.
**Wrap** decides what must fit the box; **Overflow** decides how
text that does not fit is handled. Measure uses Phase 79 metrics
(real font widths).

### Overflow values (Andrew, 2026-11-09 — locked)

| Value | Behaviour |
| --- | --- |
| **Clip** (default) | Font as set; excess clipped. |
| **Ellipsis** | Font as set; truncated with `…`. |
| **Grow** | Font as set; box height grows (old Fit Height `grow`). |
| **Shrink** | Font reduced only when text is too big; never above the set size. |
| **Fit** | Font scaled up or down until text fills the box (like image `contain`); proportional; only font size changes. |

**Wrap** decides the fit target:

- **nowrap** — a single line must fit the width.
- **wrapped** — all lines must fit the width and height.

**Min Font Size** / **Max Font Size** (new 054 properties): Min
applies to Shrink and Fit; Max applies to Fit.

### Pinned load mapping (`fitHeight` → Overflow)

| Old `fitHeight` | Overflow |
| --- | --- |
| `clip` | Clip |
| `grow` | Grow |
| `shrink` | Shrink |
| `both` | **Grow** (pinned; Andrew did not object) |

Stop writing `fitHeight` on Static Text after this phase. Wrap
stays. Friendlier labels for Wrap's CSS values are an optional
small add-on (not yet decided).

### Entry gate

Phase 79 Status complete (Shrink/Fit need real widths from Metrics).

### Work items

- [ ] 80.1 Lookup: amend 054 Overflow list to
      Clip, Ellipsis, Shrink, Grow, **Fit** (was Clip, Ellipsis,
      Shrink, Grow from Phase 75). Add **Min Font Size** and
      **Max Font Size** (054 keys; next free after Phase 79
      packets — next free **1507** after Phase 79 1504–1506).
      Reversible. Wire on Static Text (and other Kinds that show
      Overflow).
- [ ] 80.2 Lithium: remove Fit Height from layout rows; bind
      Overflow + Min/Max Font Size. Load-map `fitHeight` as
      pinned above; stop writing `fitHeight`.
- [ ] 80.3 Implement Overflow behaviours (designer + Render) using
      Phase 79 metrics. Wrap selects nowrap vs multi-line fit
      target. Shrink/Fit respect Min/Max Font Size.
- [ ] 80.4 Rule 12: amend Layout / Paging / Items / Html as needed
      (modules not finished in 79). One module per migration.
      `render_check.sh`.
- [ ] 80.5 Browser: each Overflow value × Wrap mode; Preview HTML
      matches. Optional: friendlier Wrap labels if Andrew decides.
- [ ] 80.6 (Optional add-on) Friendlier PP labels for Wrap CSS
      values — only if Andrew picks captions this sitting.

### Done means

Static Text has Overflow (five values) and no Fit Height; Wrap
selects the fit target; Min/Max Font Size drive Shrink/Fit; load
mapping applied; designer and Preview HTML agree.

### Exit gate

Lookup + Render packets applied, checks green, Andrew confirms.

### Status

**Complete (2026-11-09)** — Andrew tested; good enough (Fit × No Wrap).

### Accomplished

- Lookup Fit + Min/Max Font Size; `ITEM_FIELDS` 58/59/60.
- Fit Height removed; fitHeight→overflow load map; Overflow behaviours
  in designer + Layout/Paging.
- Fixtures overflow-*; vitest + ESLint clean.

### Lessons learned

- Overflow blank: 054 key 58 listed but no `ITEM_FIELDS` binder.
- Fit target depends on Wrap (wrap = height at box width; no-wrap =
  width only).
- Grow keeps the item font (no Helvetica force on grown lines).

### Handoff

Phase 81 Justified. Next free **1515**.

---

## Phase 81 — Justified alignment

### Goal

H. Align Justified spreads word spacing on full lines. The last line
of a paragraph is not justified (stays start-aligned). Designer and
`stage=html` match.

### Entry gate

Phase 80 Status complete. Lookup already lists Justified (Phase 75).

### Work items

- [x] 81.1 Lithium designer: `justifyWordSpacings` + per-line
      `word-spacing` on measured paint; Clip+Justified uses CSS
      `text-align: justify; text-align-last: left`. No Wrap stays
      left (gap 0).
- [x] 81.2 Render: Layout returns `wordSpacings`; Paging copies;
      Html emits per-line `word-spacing` (pt). PDF can reuse the
      same field (lua-pdfkit `align` justify / `wordSpacing` —
      docs only). Rule 12: `1515` Layout, `1516` Paging, `1517` Html.
- [x] 81.3 Fixtures `justify-*`; vitest; ESLint; `render_check`
      (new fixtures DIFF vs HEAD as expected). Browser check left
      for Andrew.

### Done means

Justified Static Text shows even word spacing on full lines; the last
line is not stretched.

### Exit gate

Packets (if any) applied, checks green, Andrew confirms.

### Status

**In progress** — packets `1515`–`1517` written (unapplied). Awaiting
Andrew apply + browser check.

### Accomplished

- Stored align spelling stays `Justified` (lookup Phase 75).
- `justifyWordSpacings` (Lithium + Layout): stretch full soft-wrapped
  lines; last line of each hard-newline paragraph, Ellipsis last
  line, single-word, and No Wrap → 0.
- Designer measured paint: one ink span per line with CSS
  `word-spacing`; Clip path keeps `text-align: justify` +
  `text-align-last: left`.
- Html: per-line `word-spacing` when `wordSpacings` present;
  non-lines Justified uses justify + text-align-last. Escaped
  `width:100%%` in the Clip format string (Lua 5.5).
- Fixtures `justify-basic`, `justify-ellipsis`; unit tests.

### Lessons learned

- Fit/Overflow interaction: Shrink/Fit size first, then justify at
  the resolved size. Grow keeps family; justify uses Layout lines.
- Back Color stays on the per-line ink span.
- No Wrap + Justified = left (documented).

### Handoff

Andrew applies `1515`–`1517`, browser-checks Justified × Wrap /
Overflow / V. Align / Back Color / No Wrap, Preview HTML vs designer.
Next free **1518**. Phase 82 Link next. Post-81 phases left: **4**
(82–85).

---

## Phase 82 — Element Link as URI anchor

### Goal

Reuse lookup 054 key 29 (Link) as a URI on any Element. A safe URI
becomes an HTML anchor and, when `stage=pdf` exists, a PDF link
annotation. Empty or disallowed schemes emit no anchor.

### Entry gate

Phase 81 Status complete (Static/Text review). Re-check the next
free Acuranzo file after Phases 75–81 migrations. Key 29 is already
on almost every Kind's 053 `attributes` list.

### Pinned decisions (Andrew 2026-11-09)

- **Allowlist** (case-insensitive schemes): `http`, `https`,
  `mailto`, `tel`, `sms`, `geo`. Fax is omitted — use `tel`.
- One table in **`Render.Core`** (`safe_uri` helper). Adding a
  scheme is one line in that table.
- Block everything else. Empty or invalid → no anchor.
- **HTML:** `Render.Items` carries validated `link` on the placed
  item; `Render.Html` wraps the box in `<a href="…">` with the
  href escaped, `rel="noopener noreferrer"`, and
  `target="_blank"` for `http`/`https` only (`mailto`/`tel`/
  `sms`/`geo` stay same-tab / no blank).
- **PDF:** `doc:link(x, y, w, h, url)` in the PDF emit module when
  `stage=pdf` lands (lua-pdfkit URI action). Until then, Items
  still carries `link` for compose.
- Applies to **any** Element Kind (054 key 29 / `item.link`).
  Report-level `doc.link` may follow the same helper later; not
  required for Done means.
- **Field per-row URI override** is deferred.
- **Optional lookup:** set 054 key 29 `Editor` to `"Text"` (closes
  the Phase 14 deferral of `Editor: None`). No new 054 key.

### Work items

- [ ] 82.1 Optional: lookup packet sets 054 key 29
      `Editor: "Text"`. No attribute-list churn unless a Kind is
      missing 29 (re-check 053).
- [ ] 82.2 Lithium: keep the Object Link text row. Optional client
      allowlist hint on commit (server/`safe_uri` remains
      authoritative). Blank still omits. No canvas chrome required
      for v1.
- [ ] 82.3 `Render.Core`: `safe_uri(s)` — trim, parse scheme
      case-insensitively, allowlist table only; return URI or nil.
      Rule 12: one migration for Core.
- [ ] 82.4 `Render.Items`: copy `safe_uri(item.link)` onto the
      placed item from `item_out`. Rule 12: one migration for
      Items.
- [ ] 82.5 `Render.Html`: when placed item has `link`, wrap the
      absolute box in `<a href="…">` (escaped),
      `rel="noopener noreferrer"`, `target="_blank"` only for
      http(s). Rule 12: one migration for Html.
- [ ] 82.6 PDF emit (when `stage=pdf` exists): for each placed
      item with `link`, `doc:link(x, y, w, h, url)`. Same
      allowlist. Rule 12: one migration for that module. If PDF
      stage is still absent at Phase 82 start, ship 82.3–82.5 and
      leave 82.6 open or fold into the PDF-stage phase — note in
      Handoff.
- [ ] 82.7 `render_check.sh` / fixture: Link on static text (and
      one non-text Kind); bad scheme (`javascript:`) produces no
      anchor; http(s) has `rel` + `target`. Browser check on
      Preview HTML.

### Done means

Setting Link to an allowlisted URI on any Element produces a real
HTML anchor in Preview; disallowed schemes produce none. PDF
annotations follow when that stage exists (or Handoff names the
follow-up). Field override is not in this phase.

### Exit gate

Rule 12 packets applied (numbers assigned after 75–81), checks
green, Andrew confirms HTML. PDF item closed or explicitly deferred
in Handoff.

### Status

**not started**

### Accomplished

_(empty until the phase closes)_

### Lessons learned

_(empty until the phase closes)_

### Handoff

Next Kind review or the deferred PDF Link step, as Andrew names.
No packet until Phase 82 starts.
