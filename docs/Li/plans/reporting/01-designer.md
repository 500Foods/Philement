<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Designer

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 1–10
**Stop when:** The author can place static text and export the definition file.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 0 in the parent is complete.
**Next file:** [`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md) after Phase 10.

## Phase 1 — Contracts and fixtures

### Goal

Write the format version 1 contracts down as documents and checked-in
JSON fixtures, so later phases test against files.

### Entry gate

Phase 0 Status complete.

### Work items

- [x] 1.1 Add a contract note under `docs/Li/` that restates the
      approved definition, intermediate, HTML, and call shapes,
      and the Defaults table. Absolute links. No line-number
      references.
- [x] 1.2 Add `elements/003-lithium/tests/fixtures/reporting/` with
      one static definition, one detail definition, one grouped
      definition, and matching source JSON. Each case has an expected
      structural intermediate: plain text, one page. The detail
      definition includes `1234.5` with mask `#,##0.00` and a null.
      A second expected file for that detail case is the formatted
      intermediate, `1,234.50` and a blank null, matching the
      value-format table. HTML, page-split, PDF, and chart fixtures
      are written by the phase that owns that gate.
- [x] 1.3 Point this plan's Phases 2, 19, and 20 at those paths.
       Chart fixtures land in Phase 57.

### Done means

The three fixtures exist and the contract note matches Phase 0.

### Exit gate

Andrew has read the contract note. `npm test` still passes (no new
test required in this phase).

### Status

**complete — 2026-10-30.** Andrew read
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md) and accepted it.
`npm test` had exited 0 (59 files, 1216 passed) before that read.
No variance.

### Accomplished

- Contract note:
  [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md). It restates
  the definition, the intermediate, the HTML emit, the call, and
  the Defaults table.
- Fixtures under
  [`/elements/003-lithium/tests/fixtures/reporting/`](/elements/003-lithium/tests/fixtures/reporting/):
  static, detail, and grouped definitions, each with sources and a
  one-page structural intermediate. The detail pair also has
  [`detail.intermediate.formatted.json`](/elements/003-lithium/tests/fixtures/reporting/detail.intermediate.formatted.json).
  `amount` is `1234.5` in the structural file and `1,234.50` in the
  formatted file. `note` is an empty string in both.
- Phases 2, 19, and 20 name those paths. Phase 57 still owns the
  chart fixtures.
- `npm test` exited 0. No new Vitest file. This phase did not
  require one.

### Lessons learned

- The lock says pages of bands of items and shows an item. It does
  not name the keys `pages`, `bands`, or a sibling `log`. This phase
  pinned them in the note and the fixtures. The read confirms them.
- A null prints `""` in both intermediate files. The formatted
  file's distinct proof is the mask on `1234.5`.
- `npm test` was fully green, including
  `tests/integration/auth.integration.test.js` against
  `https://lithium.500courses.com`. The older HTTP 503 variance did
  not appear on this run.

### Handoff

Read [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md). Confirm
the keys in Fixtures and Intermediate: top-level `pages` and `log`,
a page object with `bands` only, a band of `id`, `typeKey`,
`height`, and `items`, repeated detail ids, omitted align resolving
to `Left`, null text as `""`, `field` on the group header only, and
static sources `{}`.

Andrew accepted the note on 2026-10-30 and said to continue, so
Phase 2 ran in that same conversation. The pinned keys stand.
Chart fixtures stay in Phase 57. Phase 2 parses `8.5 in` by
`11 in` and the `0.25 in` margins from the three definition files.

---

## Phase 2 — Units and geometry

### Goal

Pure functions for length strings and the content box. No DOM.

### Entry gate

Phase 1 Status complete.

### Work items

- [x] 2.1 `model/units.js`: parse and format `in`, `mm`, `cm`, `pt`,
      `px`. Convert to CSS pixels at 96 per inch. Reject a bare
      number and an unknown unit with a thrown error the caller can
      show.
- [x] 2.2 Content-box helper: page size minus margins, in the
      document unit and in CSS pixels.
- [x] 2.3 Vitest covers the page size on the Phase 1 definitions
      [`static.definition.json`](/elements/003-lithium/tests/fixtures/reporting/static.definition.json),
      [`detail.definition.json`](/elements/003-lithium/tests/fixtures/reporting/detail.definition.json),
      and [`grouped.definition.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.definition.json).
      Each is `8.5 in` by `11 in`, with `0.25 in` margins. The contract
      note is [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).
      Also a bad string and a zero margin.

### Done means

`npm test` and `npm run lint` pass, and the new tests fail if 1 inch
is not 96 CSS pixels.

### Exit gate

Those two commands, run from `elements/003-lithium`, exit 0.

### Status

**complete — 2026-10-30.** `npm test` exited 0 (60 files, 1223
passed). `npm run lint` exited 0. No variance. The new tests expect
`toCssPx('1 in')` to be 96, and the fixture content box to be
`8 * 96` by `10.5 * 96` CSS pixels.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/model/units.js`](/elements/003-lithium/src/managers/reports/model/units.js)
  exports `parseLength`, `formatLength`, `toCssPx`, and `contentBox`.
  `CSS_PX_PER_INCH` is 96. Units are `in`, `mm`, `cm`, `pt`, and `px`.
  A bare number and an unknown unit throw an `Error` whose message
  a later toast can show.
- `contentBox` subtracts the margins and returns `width` and `height`
  as strings in the document unit, plus `widthPx` and `heightPx`.
  An omitted margin side is `0.25 in`. An omitted document unit is
  `in`. The three Phase 1 pages measure `8 in` by `10.5 in`, which
  is 768 by 1008 CSS pixels. A zero margin keeps `8.5 in` by `11 in`.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-units.test.js`](/elements/003-lithium/tests/unit/reports-units.test.js),
  7 cases. `npm test` and `npm run lint` from `elements/003-lithium`
  exited 0. `npx eslint` on `units.js` also exited 0.

### Lessons learned

- `npm run lint` globs one directory level, so
  `src/managers/reports/model/units.js` is outside that command.
  The test file is inside it. Lint the model file directly until
  the script changes.
- `8.5`, `0.25`, and `11` are exact in binary floating point, so the
  fixture subtraction prints `8 in` and `10.5 in` after rounding to
  six decimal places and stripping trailing zeros.
- An omitted margin is the Defaults-table `0.25 in`, including when
  the whole `margin` object is absent. A zero margin has to be written
  as `0 in`.

### Handoff

Phase 3 is next in this file. It loads the Phase 1 fixtures.
`contentBox` of each fixture page is `8 in` by `10.5 in`. Do not
change `CSS_PX_PER_INCH`. A thrown length error's `message` is the
text a later toast shows. There is no DOM in this module.

---

## Phase 3 — Document model and undo

### Goal

Pure create, import, export, and edit of a definition, with undo.

### Entry gate

Phase 2 Status complete.

### Work items

- [ ] 3.1 `model/document.js` loads a fixture, rejects `format` other
      than 1, and exports canonical JSON (stable key order is not
      required; semantic equality is).
- [ ] 3.2 Add, move, resize, and delete a band and an item. Ids are
      stable across undo.
- [ ] 3.3 `model/history.js` snapshots the document. Undo and redo
      restore it. A new edit drops the redo tail.
- [ ] 3.4 Vitest uses the Phase 1 fixtures.
- [ ] 3.5 `createBlank()` returns the new-report document from the
      Defaults table: Letter portrait, the grid and font rows, and
      page header, detail, and page footer at the table heights.
      Vitest checks those written keys. Detail has no datasource.

### Done means

A test imports the detail fixture, moves a field, undoes the move,
and the export matches the fixture again.

### Exit gate

`npm test` and `npm run lint` exit 0.

### Status

**not started**

---

## Phase 4 — Manager shell, import, export

### Goal

Replace the placeholder with a manager that starts a blank report,
loads a fixture, and downloads the definition file.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 `reports.html` / `reports.css` / `reports.js`. Fetch the
      HTML template. `templates:copy`. Stay under 1000 lines.
- [ ] 4.2 `init`, `destroy`, and `cleanup`. CATCHUP Phase 1
      (2026-10-16) calls `destroy`, otherwise `cleanup`, otherwise
      `teardown`. `destroy` releases the listeners. `cleanup`
      releases the same set when `destroy` is absent. Drop the
      "under development" copy.
- [ ] 4.3 Import picks a local JSON file, runs it through the
      document model, and shows the report name. A bad file toasts
      the model error and leaves the previous document in place.
- [ ] 4.4 Export downloads the current definition as
      `<name>.report.json`.
- [ ] 4.5 Correct the Reports id sentence in
      [`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
      from 12 to 24.
- [ ] 4.6 Browser: import the detail fixture, export it, import the
      export, and confirm the name survived. Confirm a bad file
      toasts and the name stays.
- [ ] 4.7 New calls `createBlank()`. Export of that document contains
      the written defaults, including `page.sizeKey` 0,
      `font.style` `Normal`, `grid.style` `dots`, and the three
      bands. An imported file that omits a key still omits it on
      export.

### Done means

Manager 24 round-trips the detail fixture through a file, and New
exports the written defaults. Lint and unit tests pass.

### Exit gate

`npm test`, `npm run lint`, `npm run lint:css`, `templates:copy`, and
the browser check in 4.6.

### Status

**not started**

---

## Phase 5 — Page surface

### Goal

Show one page at the definition's size, with margins.

### Entry gate

Phase 4 Status complete.

### Work items

- [ ] 5.1 Draw the page from the document model. Unzoomed scale is
      96 CSS pixels per inch. Margins are guides.
- [ ] 5.2 Changing the page in the model (test hook or a temporary
      control that Phase 14 will replace) redraws the page.
- [ ] 5.3 Browser: Letter portrait is larger than a 4 in custom page,
      and the margin guides sit inset by 0.25 in on the fixture.

### Done means

The open fixture is visible as a page, and the unit tests still pass.

### Exit gate

Phase 4 commands plus the browser check.

### Status

**not started**

---

## Phase 6 — Bands and resize

### Goal

Render the v1 bands and let the operator change a band's height.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 Paint each band as a labeled strip and a body of the stored
      height. Order is the array order.
- [ ] 6.2 Drag the bottom edge. Write the new height back in the
      document unit. `element.style.height` is the drag preview;
      mouseup commits through the model and pushes history.
- [ ] 6.3 Minimum height 0.15 in. Undo restores the previous height.
- [ ] 6.4 Browser: resize Detail, export, and confirm the file's
      height changed. Undo restores the canvas and a second export.

### Done means

Band height round-trips through the definition file.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 7 — Section grid

### Goal

Recreate the dot, line, dotted, and dashed backgrounds.

### Entry gate

Phase 6 Status complete.

### Work items

- [ ] 7.1 `grid.js` draws an SVG pattern from the definition's grid
      object: dots, grid, dotted, dashed, and none.
- [ ] 7.2 A `ResizeObserver` redraws when the band size changes.
      Disconnect it in `cleanup` / `destroy`.
- [ ] 7.3 Spacing follows the unit string. The default is the
      Defaults table: `0.125 in`, style `dots`.
- [ ] 7.4 Browser: switch the fixture grid style across the five
      values and resize a band. The pattern stays aligned to the
      band origin.

### Done means

The five grid styles render, and cleanup removes the observer.

### Exit gate

Lint, unit tests for the pattern geometry that can run without a
browser, and the browser check.

### Status

**not started**

---

## Phase 8 — Rulers and the mouse line

### Goal

Top and left rulers in the document unit, with a hairline that
follows the pointer.

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 `rulers.js` builds an SVG ruler for `in` and `cm`. Major
      ticks labeled, minor ticks at eighths of an inch or millimetres
      of a centimetre. Match the retired ruler's density closely
      enough that a 1 in mark lands on 96 CSS pixels when zoom is 1.
- [ ] 8.2 `mousemove` moves a horizontal and a vertical cursor line.
      `mouseleave` hides them. Listeners disconnect on cleanup.
- [ ] 8.3 The ruler is chrome. Export does not gain a Ruler band.
- [ ] 8.4 Browser: the 1 inch tick aligns with a band edge placed at
      `1 in`. The hairline tracks and disappears outside the page.

### Done means

Rulers track the pointer and agree with `model/units.js`.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 9 — View scale

### Goal

Fit the page to the canvas width, and zoom around that fit.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 A CSS variable scales the page, the grid, and the rulers
      together. The definition's physical units do not change.
- [ ] 9.2 Fit-width, 100%, zoom in, and zoom out. 100% is 96 CSS
      pixels per inch.
- [ ] 9.3 Browser: fit-width keeps the whole Letter page visible in
      the designer pane. 100% makes the ruler's 1 inch mark match a
      CSS measurement of 96 pixels.

### Done means

Zoom changes pixels on screen and leaves the exported JSON alone.

### Exit gate

Lint, a unit test that fit-width math returns the expected scale for
a known pane width, and the browser check.

### Status

**not started**

---

## Phase 10 — Static text

### Goal

Place, move, resize, and select a static text item.

### Entry gate

Phase 9 Status complete.

### Work items

- [ ] 10.1 The palette adds a static text item to the selected band.
       The new item writes the Defaults table: box `1.5 in` by
       `0.2 in`, centered in the band, `align` `Left`, `text`
       `Text`. It omits `font` and `mask`.
- [ ] 10.2 Drag moves `left` and `top`. Corner resize changes `width`
       and `height`. Commit on pointerup through the model. Clamp to
       the band.
- [ ] 10.3 Selection draws an inset outline. Delete removes the item.
       Undo restores position and existence.
- [ ] 10.4 The item shows its text. v1 text is the item's `text`
       property, edited in Phase 14. Until then a new item reads
       "Text".
- [ ] 10.5 Browser: add, drag, resize, undo, export. The file has
       `typeKey` 0 and unit strings.
- [ ] 10.6 The canvas clips item text to the item box. A string
       longer than the box is cut off on screen. Emit clips the same
       way. The band does not grow.

### Done means

A static text item survives export, import, and undo.

### Exit gate

Lint, unit tests for the clamp, and the browser check.

### Status

**not started**

