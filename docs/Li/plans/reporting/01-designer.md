<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Designer

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 1–10, and Phase 10-UI
**Stop when:** The author can place static text and export the definition file. Phase 10-UI is the manager chrome and is in progress.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 0 in the parent is complete.
**Next file:** [`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md) after Phase 10-UI, when Andrew names Phase 11.

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

- [x] 3.1 `model/document.js` loads a fixture, rejects `format` other
      than 1, and exports canonical JSON (stable key order is not
      required; semantic equality is).
- [x] 3.2 Add, move, resize, and delete a band and an item. Ids are
      stable across undo.
- [x] 3.3 `model/history.js` snapshots the document. Undo and redo
      restore it. A new edit drops the redo tail.
- [x] 3.4 Vitest uses the Phase 1 fixtures.
- [x] 3.5 `createBlank()` returns the new-report document from the
      Defaults table: Letter portrait, the grid and font rows, and
      page header, detail, and page footer at the table heights.
      Vitest checks those written keys. Detail has no datasource.

### Done means

A test imports the detail fixture, moves a field, undoes the move,
and the export matches the fixture again.

### Exit gate

`npm test` and `npm run lint` exit 0.

### Status

**complete — 2026-10-31.** `npm test` exited 0 (62 files, 1238
passed). `npm run lint` exited 0. No variance. The detail fixture's
`customer` field moves to `1 in` and undo exports the fixture again,
with id `customer`.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/model/document.js`](/elements/003-lithium/src/managers/reports/model/document.js)
  exports `importDefinition`, `exportDefinition`, `createBlank`,
  `addBand`, `moveBand`, `resizeBand`, `deleteBand`, `addItem`,
  `moveItem`, `resizeItem`, and `deleteItem`. Each edit returns a
  new document. Import and export are a JSON copy. `format` must be
  the number 1. Omitted keys stay omitted.
- `createBlank()` writes Letter portrait (`sizeKey` 0, `8.5 in` by
  `11 in`), the fixture grid and font, and bands `ph` (0.4 in),
  `detail` (0.25 in), and `pf` (0.4 in). Detail has no `datasource`.
  A generated band id is `b1`, `b2`, and so on. A generated item id
  is `i1`, `i2`, and so on.
- A new item writes `align` `Left` and a `1.5 in` by `0.2 in` box
  centered in the band. Static text writes `Text`. Number, datetime,
  aggregate number, aggregate datetime, parameter, system datetime,
  page number, and page count write `mask` `""`. Text omits `mask`.
  Font stays omitted. Band height clamps to `0.15 in` on add and
  resize, and the stored length uses the document unit.
- [`/elements/003-lithium/src/managers/reports/model/history.js`](/elements/003-lithium/src/managers/reports/model/history.js)
  exports `createHistory`. `commit` snapshots the current document.
  Undo and redo restore that snapshot. A later `commit` drops the
  redo tail. The caller commits. The document functions do not.
- [`/elements/003-lithium/src/managers/reports/model/units.js`](/elements/003-lithium/src/managers/reports/model/units.js)
  exports `convertLength`. `CSS_PX_PER_INCH` is still 96.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-document.test.js`](/elements/003-lithium/tests/unit/reports-document.test.js)
  (9) and
  [`/elements/003-lithium/tests/unit/reports-history.test.js`](/elements/003-lithium/tests/unit/reports-history.test.js)
  (5). The units file gained one conversion case.
  `npm test` and `npm run lint` from `elements/003-lithium` exited 0.
  `npx eslint` on the three model files also exited 0.

### Lessons learned

- `npm run lint` still misses `src/managers/reports/model/*.js`.
  Lint those files with `npx eslint`. `structuredClone` is not an
  ESLint global. Copies go through `JSON.parse(JSON.stringify)`,
  which is the canonical JSON form.
- On a Letter page with `0.25 in` margins, a new item in the
  `0.25 in` detail band is `left` `3.25 in`, `top` `0.025 in`,
  `1.5 in` by `0.2 in`.
- Import does not clamp a short band. Resize and add do.
- `grid.weight` is the string `0.5`, matching the fixtures.
- Image, chart, and barcode dpi are not written here. Those
  objects arrive in later phases.
- The snapshot sentences above are this phase's close-out. The
  live trail is diffs on `history`. The rule is the definition
  lock in [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
  and [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).

### Handoff

Phase 4 is next in this file. It calls `createBlank`,
`importDefinition`, and `exportDefinition`. `exportDefinition`
returns an object. Stringify it for the download. Key order is
not significant. A bad file throws `Error`, and `message` is the
toast text. Format other than 1 says `Report format must be 1.`
An imported file that omits a key still omits it. The static
fixture title has no `align`, `font`, or `mask`. `createBlank`
writes `ph`, `detail`, and `pf`. Detail has no datasource.
Undo is `createHistory(doc)` then `commit(next)`. `resizeBand`
clamps to `0.15 in` and stores the document unit. `96 px` on an
inch report becomes `1 in`. There is no DOM in this module.
Manager 24 is still the placeholder.

---

## Phase 4 — Manager shell, import, export

### Goal

Replace the placeholder with a manager that starts a blank report,
loads a fixture, and downloads the definition file.

### Entry gate

Phase 3 Status complete.

### Work items

- [x] 4.1 `reports.html` / `reports.css` / `reports.js`. Fetch the
      HTML template. `templates:copy`. Stay under 1000 lines.
- [x] 4.2 `init`, `destroy`, and `cleanup`. CATCHUP Phase 1
      (2026-10-16) calls `destroy`, otherwise `cleanup`, otherwise
      `teardown`. `destroy` releases the listeners. `cleanup`
      releases the same set when `destroy` is absent. Drop the
      "under development" copy.
- [x] 4.3 Import picks a local JSON file, runs it through the
      document model, and shows the report name. A bad file toasts
      the model error and leaves the previous document in place.
- [x] 4.4 Export downloads the current definition as
      `<name>.report.json`.
- [x] 4.5 Correct the Reports id sentence in
      [`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
      from 12 to 24.
- [x] 4.6 Browser: import the detail fixture, export it, import the
      export, and confirm the name survived. Confirm a bad file
      toasts and the name stays. Andrew confirmed this on
      `https://lithium.500courses.com` on 2026-10-31. The stage
      listed the blank definition, import showed `Orders`, and New
      restored the blank lines.
- [x] 4.7 New calls `createBlank()`. Export of that document contains
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

**complete — 2026-10-31.** `npm test` exited 0 (63 files, 1258
passed). `npm run lint` exited 0. `npm run lint:css` exited 0.
`npm run templates:copy` copied `reports.html` and `reports.css`.
`npx eslint` on the shell files exited 0. Andrew confirmed the
browser check on `https://lithium.500courses.com`: the stage
lists the blank definition, import of the detail fixture shows
`Orders`, New restores the blank lines, the export round-trips,
and a bad file toasts while the name stays. No variance.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/reports.html`](/elements/003-lithium/src/managers/reports/reports.html),
  [`reports.css`](/elements/003-lithium/src/managers/reports/reports.css),
  and [`reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  replace the placeholder. The manager fetches
  `/src/managers/reports/reports.html`. The toolbar shows the report
  name and New, Import, and Export. `#reports-stage` lists the open
  definition: name, download name, page, margins, grid, font, and
  each band with its items. Omitted sections say `Omitted`. A file
  with no bands says `No bands.` The page drawing is Phase 5.
- `init` calls `createBlank()`. `destroy` and `cleanup` both release
  the New, Import, Export, and file-input listeners. Close calls
  `destroy` because that method exists.
- Import reads a local JSON file through `importDefinition`. The
  toolbar shows `name`, or `Report` when that key is omitted or
  empty. A bad file toasts `error.message` and leaves the open
  document in place.
- Export downloads `<name>.report.json`. The text is pretty JSON
  from `exportDefinition`, with a trailing newline. An omitted key
  stays omitted. A name that cannot be a file name is adjusted only
  on the download. The stored name is unchanged.
- [`/elements/003-lithium/src/managers/reports/reports-file.js`](/elements/003-lithium/src/managers/reports/reports-file.js)
  holds the name, the text, the stage lines, and the import result.
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  covers them (20). The stage paints those lines as text.
- [`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
  names Reports as id 24.

### Lessons learned

- `npm run lint` still misses `src/managers/reports/*.js`. Lint
  those files with `npx eslint`. A regular expression that names
  `\u0000` fails `no-control-regex`. The download name walks
  characters instead.
- The shell shows an omitted name as `Report` and downloads
  `Report.report.json`. The file still omits `name`. `/`, `\`,
  `:`, `*`, `?`, `"`, `<`, `>`, `|`, and control characters stay
  in the shown name and become `_` in the file name. A name that
  is only spaces or dots downloads as `Report.report.json`.
- `lint:css` also failed on a rule that was already in
  `page-login-history.css`: two declarations on one line. That
  rule is now two lines, so the named command exits 0.
- happy-dom clicked New, Import, and Export. That is not the
  500 Courses check.
- The first 500 Courses check loaded manager 24 and logged
  `[Reports] New report` twice. The stage was empty, and the
  toolbar was already `Report`, so New looked idle. The stage now
  lists the definition. New on a blank report keeps those same
  lines. New after an import restores them.

### Handoff

Phase 5 is next in this file. It paints one page into
`#reports-stage` at 96 CSS pixels per inch. Margin guides sit
inset from the page edge. Andrew confirmed the shell on
2026-10-31. The stage already lists the open definition, so the
page is drawn into that same element. A blank report is Letter
portrait, `8.5 in` by `11 in`, margins `0.25 in`. Unzoomed Letter
is larger than a 4 in custom page. `exportDefinition` returns an
object. Stringify it for a download. Key order is not significant.
Undo is `createHistory(doc)` then `commit(next)`. Undo is not
wired in the shell. There is no DOM in the model. Phase 5 has
not started.

---

## Phase 5 — Page surface

### Goal

Show one page at the definition's size, with margins.

### Entry gate

Phase 4 Status complete.

### Work items

- [x] 5.1 Draw the page from the document model. Unzoomed scale is
      96 CSS pixels per inch. Margins are guides.
- [x] 5.2 Changing the page in the model (test hook or a temporary
      control that Phase 14 will replace) redraws the page.
      `loadReport` is that hook. Import and New use the same paint.
- [x] 5.3 Browser: Letter portrait is larger than a 4 in custom page,
      and the margin guides sit inset by 0.25 in on the fixture.

### Done means

The open fixture is visible as a page, and the unit tests still pass.

### Exit gate

Phase 4 commands plus the browser check.

### Status

**complete — 2026-10-31.** Andrew confirmed the page on
`https://lithium.500courses.com`. Letter portrait is larger than a
4 in page. The fixture margin guides sit inset 0.25 in. `npm test`
exited 0 (63 files, 1261 passed). `npm run lint` exited 0.
`npm run lint:css` exited 0. `npm run templates:copy` copied
`reports.css`. `npx eslint` on `canvas.js`, `reports.js`, and
`units.js` exited 0. Work item 5.3 is checked. Phase 6 is next and
has not started.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/model/units.js`](/elements/003-lithium/src/managers/reports/model/units.js)
  exports `pageBox`. It is the full sheet in CSS pixels. An omitted
  margin side is `0.25 in`. `contentBox` stays the area inside the
  margins.
- [`/elements/003-lithium/src/managers/reports/canvas.js`](/elements/003-lithium/src/managers/reports/canvas.js)
  exports `pageFrame` and `paintReportPage`. A missing page says
  `The file omits the page.` A length that cannot be measured
  shows that message and does not draw a sheet.
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  paints the page above the definition list. `loadReport` replaces
  the document and redraws. A bad length toasts the model message.
  Size and insets are CSS variables on `#reports-page`. Letter is
  `816px` by `1056px`. A `4 in` page is `384px` by `384px`. The
  fixture margins are `24px`.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-units.test.js`](/elements/003-lithium/tests/unit/reports-units.test.js)
  (9) and
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  (22).

### Lessons learned

- The sheet and the content box are different pixel sizes. Letter
  with `0.25 in` margins is `816` by `1056` on the sheet and `768`
  by `1008` inside the guides.
- The page size is a CSS variable set from the model. The guide
  reads those variables and has no inline style. Bands, the grid,
  and rulers are still later phases.
- A file with no page is valid. It does not invent Letter. A width
  with no unit toasts and leaves the sheet off the stage.
- happy-dom reads the CSS variables. That is not the 500 Courses
  check.

### Handoff

Phase 6 is next in this file. It has not started. Andrew confirmed
the page on `https://lithium.500courses.com` on 2026-10-31. Letter
portrait is larger than a 4 in page. The fixture guides sit inset
0.25 in. The page stays `816px` by `1056px` with `24px` insets.
The definition list stays under the page. Do not redraw the page.
Phase 6 paints each band as a labeled strip and a body of the
stored height, in array order. Drag uses `element.style.height` as
the preview. Mouseup commits through the model and pushes history.
The minimum height is `0.15 in`. Undo restores the previous height.
The browser check resizes Detail, exports, and confirms the file
height changed, then undo restores the canvas and a second export.
The grid and the rulers are later phases. Undo is not wired.

---

## Phase 6 — Bands and resize

### Goal

Render the v1 bands and let the operator change a band's height.

### Entry gate

Phase 5 Status complete.

### Work items

- [x] 6.1 Paint each band as a labeled strip and a body of the stored
      height. Order is the array order.
- [x] 6.2 Drag the bottom edge. Write the new height back in the
      document unit. `element.style.height` is the drag preview;
      mouseup commits through the model and pushes history.
- [x] 6.3 Minimum height 0.15 in. Undo restores the previous height.
- [x] 6.4 Browser: resize Detail, export, and confirm the file's
      height changed. Undo restores the canvas and a second export.

### Done means

Band height round-trips through the definition file.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**complete — 2026-10-31.** Andrew confirmed drag, Undo, Redo,
export, and import on the alternate database. Redo is the
toolbar button beside Undo. Ctrl+Z undoes and Ctrl+Y redoes.
Both are registered with the shortcut list under Report
Manager, and they run only while that slot is visible.
Ctrl+Shift+Z stays Toggle zoom. `npm test` exited 0 (66 files,
1269 passed, 12 skipped). The 12 skipped cases are the auth
integration file. `npm run lint` exited 0. `npm run templates:copy`
copied `reports.html`. Phase 7 closed in the same report.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/bands.js`](/elements/003-lithium/src/managers/reports/bands.js)
  paints each band, in array order, as a role strip and a body.
  The body height is the stored length at 96 CSS pixels per inch.
  An omitted height uses the Defaults table and is not written.
  A length that cannot be measured shows that message and draws
  no body.
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  drags the bottom edge. `element.style.height` is the preview.
  Mouseup calls `resizeBand` and `history.commit`. The stored
  length uses the document unit. The floor is `0.15 in`. Undo,
  from the toolbar or Ctrl+Z, restores the previous height.
  New and Import start a fresh history. Redo, from the toolbar
  or Ctrl+Y, restores the undone snapshot. A new edit drops
  that tail. Ctrl+Z and Ctrl+Y are registered for manager 24.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-bands.test.js`](/elements/003-lithium/tests/unit/reports-bands.test.js)
  (4) and
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  (27).

### Lessons learned

- The strip is chrome. The body is the stored height. Dragging
  Detail down by one inch stores `1.25 in`. The strip is not added
  to that length.
- The resting height is a CSS variable. `element.style.height` is
  only the drag preview. Mouseup repaints from the model, so the
  inline height is gone.
- Import does not clamp a short band. The drag and `resizeBand` do.
- An omitted height is drawn from the Defaults table. The key stays
  omitted until the drag writes it.
- New and Import replace history. Undo restores the resize. It does
  not restore the file that was open before New.
- happy-dom checks the CSS variable and the exported JSON. That is
  not the 500 Courses check.
- Ctrl+Shift+Z is already Toggle zoom. Redo is Ctrl+Y. The
  shortcut list owns both keys, and a manager shortcut runs only
  while that manager's slot is visible.
- The "fresh history" and "snapshot" sentences above are this
  phase's close-out. Later the same day, Import restores
  `history` from the file. New still omits the key. The live
  rule is the definition lock.

### Handoff

Phase 6 stays in progress. Andrew deploys and opens Report Manager
on `https://lithium.500courses.com`. New shows a Letter page with
three bands inside the margin guides, top to bottom: Page header
`ph` (`0.4 in`), Detail `detail` (`0.25 in`), Page footer `pf`
(`0.4 in`). Each one is a gray strip and a white body. Drag the
bottom edge of Detail. The body follows the pointer. Release, then
Export. The file's Detail height is no longer `0.25 in`. A
one-inch drag stores `1.25 in`. Undo, the toolbar button or
Ctrl+Z, restores the body and a second export is `0.25 in` again.
A drag shorter than `0.15 in` stays `0.15 in`. The page size stays
Letter. The grid and the rulers were later when this handoff was
written. Later the same day the test database was offline.
Andrew confirmed the export and import on the alternate
database on 2026-10-31, along with Undo and Redo. Phase 7
closed in that same report. Phase 8 paints the rulers. It
has not started.

---

## Phase 7 — Section grid

### Goal

Recreate the dot, line, dotted, and dashed backgrounds.

### Entry gate

Phase 6 Status complete.

### Work items

- [x] 7.1 `grid.js` draws an SVG pattern from the definition's grid
      object: dots, grid, dotted, dashed, and none.
- [x] 7.2 A `ResizeObserver` redraws when the band size changes.
      Disconnect it in `cleanup` / `destroy`.
- [x] 7.3 Spacing follows the unit string. The default is the
      Defaults table: `0.125 in`, style `dots`.
- [x] 7.4 Browser: switch the fixture grid style across the five
      values and resize a band. The pattern stays aligned to the
      band origin.

### Done means

The five grid styles render, and cleanup removes the observer.

### Exit gate

Lint, unit tests for the pattern geometry that can run without a
browser, and the browser check.

### Status

**complete — 2026-10-31.** Andrew saw the dots and said this
slice is done. The first paint used a radius of `0.5` on the
tile corner and did not show. The painter now draws a full dot
on each spacing point, at least 1 CSS pixel in radius. The
stored weight stays `0.5`. `npm test` exited 0 (66 files,
1269 passed, 12 skipped). `npm run lint` exited 0.
`npm run templates:copy` copied `reports.html`. Phase 6 closed
in the same report.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/grid.js`](/elements/003-lithium/src/managers/reports/grid.js)
  resolves the grid. An omitted key uses the Defaults table and
  is not written. `dots`, `grid`, `dotted`, and `dashed` are SVG
  patterns. `none` and `visible: false` draw no marks. The tile
  origin is the band body's top-left. Spacing `0.125 in` is 12
  CSS pixels. Each tile draws the dot on all four corners so the
  mark sits on the spacing lattice. A stored weight below 1 CSS
  pixel paints at radius 1.
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  watches each band body with a `ResizeObserver`. A resize sets
  the svg viewport and leaves the pattern at `0,0`. `destroy` and
  `cleanup` disconnect that observer. A bad spacing toasts the
  units message and draws no pattern.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-grid.test.js`](/elements/003-lithium/tests/unit/reports-grid.test.js)
  (6) and
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  (29).

### Lessons learned

- The pattern origin is the band corner. A resize changes the svg
  viewport. The pattern's `x` and `y` stay `0`, and the tile width
  stays the spacing.
- A dot of radius `0.5` centered on the tile corner does not
  show. The pattern clips it to a quarter of a pixel. Lines at
  stroke width `0.5` do show, because the stroke runs the length
  of the cell. The painted dot radius is at least 1 CSS pixel,
  and each tile draws all four corners so the dot sits on the
  lattice. The stored weight stays `0.5`.
- An omitted grid paints `dots` at `0.125 in` and the export still
  has no `grid` key. A length with no unit toasts and draws no
  pattern. The bands stay.
- happy-dom checks the pattern attributes and that `disconnect`
  ran. That is not the 500 Courses check.

### Handoff

Phase 7 is complete. Phase 6 is complete. Andrew confirmed
Undo, Redo, export, import, and the dots on the alternate
database on 2026-10-31. Ctrl+Z undoes and Ctrl+Y redoes.
Both are in the shortcut list while Report Manager is the
visible slot. Ctrl+Shift+Z remains Toggle zoom. Phase 8
paints the rulers and the mouse line. It has not started.

---

## Phase 8 — Rulers and the mouse line

### Goal

Top and left rulers in the document unit, with a hairline that
follows the pointer.

### Entry gate

Phase 7 Status complete.

### Work items

- [x] 8.1 `rulers.js` builds an SVG ruler for `in` and `cm`. Major
      ticks labeled, minor ticks at eighths of an inch or millimetres
      of a centimetre. Match the retired ruler's density closely
      enough that a 1 in mark lands on 96 CSS pixels when zoom is 1.
- [x] 8.2 `mousemove` moves a horizontal and a vertical cursor line.
      `mouseleave` hides them. Listeners disconnect on cleanup.
- [x] 8.3 The ruler is chrome. Export does not gain a Ruler band.
- [x] 8.4 Browser: the 1 inch tick aligns with a band edge placed at
      `1 in`. The hairline tracks and disappears outside the page.

### Done means

Rulers track the pointer and agree with `model/units.js`.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**complete — 2026-10-31.** Andrew accepted the rulers on
`https://lithium.500courses.com`. He said the first rulers
work, accepted the 28 CSS pixel bar, said the corner cycles,
and said he likes the unit name. He asked if the phase is
wrapped up. Work items 8.1–8.4 are checked. `npm test`
exited 0 (68 files, 1292 passed, 12 skipped). The skipped
cases are the auth integration file. That host did not
answer. `npm run lint` exited 0. `npm run lint:css` exited
0. `npx eslint` on the report files exited 0. `npm run
templates:copy` copied `reports.css`. Phase 9 has not
started.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/rulers.js`](/elements/003-lithium/src/managers/reports/rulers.js)
  builds the top and left SVG rulers. The origin is the top-left
  of the white page. Inches tick at eighths and label each inch.
  The major tick at 1 inch is 96 CSS pixels from that corner.
  The label sits 2 pixels to the left of that tick, and its top
  aligns with the top of the tick. On the left ruler the label
  sits 2 pixels above the tick, and its left edge aligns with
  the outer end of the tick. Major ticks are 21 pixels, the
  half step is 13.5, and minor ticks are 7.5. Seven pixels
  remain outside the major tick. Centimetres tick at millimetres
  and label each centimetre. Millimetres label every 10. Points
  label every 72, which is the same 96 CSS pixels. Pixels label
  every 96. A half step is a longer tick. The origin stays
  unlabeled. The corner is a button labeled with the unit
  name. Each click advances inches, then centimetres, then
  points, then inches. Both rulers relabel in that unit at
  the same physical scale. A centimetre mark lands on
  `toCssPx('1 cm')`, about 37.8 CSS pixels. The point mark
  `72` lands on 96 CSS pixels. The file keeps `page.units`.
  New and Import show the file unit again. A millimetre or
  pixel file stays in that unit until the first click, which
  starts at inches. The choice lasts for this open document.
  Undo and redo of a band edit keep it. The trail records
  the band edit. An omitted unit paints inches. An unknown
  unit toasts
  and draws the page with no rulers. The definition gains no
  ruler key. `mousemove` places a vertical line and a horizontal
  line. `mouseleave`, and a point outside the page, hide both.
- [`/elements/003-lithium/src/managers/reports/canvas.js`](/elements/003-lithium/src/managers/reports/canvas.js)
  wraps the page in `#reports-sheet`. The sheet and the page
  both carry the page width and height. The ruler thickness is
  `--reports-ruler-size` (28 CSS pixels). A ruler error notes
  the message and appends the page without the sheet.
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  binds the mouse line to `#reports-page` after each paint.
  The line is offset by the ruler thickness, so it also crosses
  the ruler bars. A band drag inverts the vertical ruler from
  the edge's old page position to its new one. Releasing the
  edge clears that span. A click on the corner advances the
  cycle and repaints. `destroy` and `cleanup` disconnect the
  mouse listener, the corner listener, and the grid observer.
  A reload of the document binds the listeners again.
- Export still matches `createBlank` for a new report. The file
  has bands `ph`, `detail`, and `pf`. It has no Ruler band.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-rulers.test.js`](/elements/003-lithium/tests/unit/reports-rulers.test.js)
  (9) and
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  (34).

### Lessons learned

- A custom property set on the page does not feed the sheet's
  grid tracks. The sheet carries the same width and height.
  The page keeps them because the existing tests read them
  from `#reports-page`.
- The zero tick is unlabeled. The corner is a button labeled
  with the unit name.
- Stylelint wants a blank line between a custom property and
  the next declaration.
- The ESLint test environment does not define `MouseEvent`.
  The tests dispatch `CustomEvent` with `clientX` and
  `clientY`, and they stub `getBoundingClientRect`. happy-dom
  returns a zero box, which would hide every point.
- `npm run lint` does not expand into `src/managers/reports`.
  `npx eslint` on those files is the extra gate. It exited 0.
- `templates:copy` copies every manager HTML and CSS file.
  `reports.css` is the file this phase changed.
- The work item names inch eighths and centimetre millimetres.
  Millimetres, points, and pixels were unnamed. They are view
  chrome on the same physical inch. The definition does not
  store them. Andrew can reject that density on the browser
  check.
- The first bar was 56 pixels so the labels could sit in the
  outer half. With the labels on the ticks, three quarters of
  that outer band is gone. The bar is 28.
- A dark span alone hides the dark ticks. The invert is a
  second copy of the marks, clipped to the span, drawn in the
  ruler background color.
- The corner cycles inches, centimetres, and points for this
  view. The file keeps `page.units`. New and Import show the
  file unit again. A millimetre or pixel file stays in that
  unit until the first click, which starts at inches. The
  choice lasts for the open document. It stays out of the
  definition and out of settings. Right and bottom rulers
  stay later. Andrew left those for a later pass on
  2026-10-31.
- The corner first used `color: inherit`. The manager text
  is `--text-primary` (`#E0E0E0`) and the ruler face is
  `#ececec`, so the unit name disappeared. The letters are
  `#2a2a2a`, the same ink as the ticks.
- happy-dom checks tick positions and that the listener
  disconnected. That is not the 500 Courses check.

### Handoff

Phase 8 is complete. Andrew accepted the rulers on
`https://lithium.500courses.com` on 2026-10-31. He said the
first rulers work, accepted the 28 CSS pixel bar, said the
corner cycles, and said he likes the unit name. The 1 inch
mark is 96 CSS pixels from the page edge. The hairline
follows the pointer and hides on leave. The corner cycles
inches, centimetres, and points and leaves `page.units`
alone. The letters are `#2a2a2a`. Right and bottom rulers
stay later. Phase 9 has not started. It scales the page,
the grid, and the rulers together. 100% stays 96 CSS pixels
per inch. The 28 pixel bar and the corner button scale with
that variable.

---

## Phase 9 — View scale

### Goal

Fit the page to the canvas width, and zoom around that fit.

### Entry gate

Phase 8 Status complete.

### Work items

- [x] 9.1 A CSS variable scales the page, the grid, and the rulers
      together. The definition's physical units do not change.
- [x] 9.2 Fit-width, 100%, zoom in, and zoom out. 100% is 96 CSS
      pixels per inch.
- [x] 9.3 Browser: fit-width keeps the whole Letter page visible in
      the designer pane. 100% makes the ruler's 1 inch mark match a
      CSS measurement of 96 pixels.

### Done means

Zoom changes pixels on screen and leaves the exported JSON alone.

### Exit gate

Lint, a unit test that fit-width math returns the expected scale for
a known pane width, and the browser check.

### Status

**complete — 2026-10-31.** Andrew said the view scale works
pretty well. Work items 9.1–9.3 are checked. `npm test`
exited 0 (69 files, 1301 passed, 12 skipped). The skipped
cases are the auth integration file. The server check for
`https://lithium.500courses.com` was aborted. `npm run lint`
exited 0. `npm run lint:css` exited 0. `npx eslint` on the
report files exited 0. `npm run templates:copy` copied 137
templates, including `reports.html` and `reports.css`.
Phase 10 has not started.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/scale.js`](/elements/003-lithium/src/managers/reports/scale.js)
  is the view math. `1` is 100%, which is 96 CSS pixels per
  inch. Fit-width is `floor(100 * paneWidth / sheetWidth) / 100`.
  A Letter sheet is the page (`816`) plus the 28 pixel ruler
  bar, `844`. A `1000` pixel pane returns `1.18`. A `600` pixel
  pane returns `0.71`. Equal widths return `1`. A pane or a
  sheet that is not positive returns `1`. Below one percent the
  exact ratio is kept. Zoom steps by `0.05` on integer
  hundredths. The step stops at `0.1` and `4`. A fit that
  already sits outside those limits stays there until the next
  step walks back in. The label is the rounded percent.
- [`/elements/003-lithium/src/managers/reports/canvas.js`](/elements/003-lithium/src/managers/reports/canvas.js)
  sets `--reports-page-width`, `--reports-page-height`,
  `--reports-ruler-size`, and `--reports-scale` on the canvas,
  and adds `reports-canvas-fitted`. The page element keeps the
  unscaled size. A Letter page stays `816px` wide. The 1 inch
  major tick stays at `x1=96`.
- [`/elements/003-lithium/src/managers/reports/reports.css`](/elements/003-lithium/src/managers/reports/reports.css)
  sizes the canvas to the scaled sheet. The sheet is
  `transform: scale(var(--reports-scale, 1))` from the top left,
  so the page, the grid, the rulers, and the corner scale
  together. Layout coordinates stay at 96 CSS pixels per inch.
  `.reports-stage` uses `scrollbar-gutter: stable`.
- [`/elements/003-lithium/src/managers/reports/reports.html`](/elements/003-lithium/src/managers/reports/reports.html)
  adds Fit width, Zoom out, the percent label, 100%, and Zoom
  in after Clear. 100% is titled "100% is 96 CSS pixels per
  inch".
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  keeps the scale on the manager. It is absent from the
  definition, from history, and from settings. Fit width
  follows a later resize. 100% and the zoom buttons keep the
  chosen scale. The wheel over `#reports-page` takes the same
  five percent step. Scroll up zooms in. That wheel does not
  scroll the stage. A wheel on the summary, the rulers, or a
  wheel with no vertical travel still scrolls. New and Import
  clear the choice and fit again. A pane whose width is still
  `0` paints at `1`. A `ResizeObserver` fits once the stage
  has a width. The corner cycle and undo repaint through the
  same scale. A band drag divides the pointer delta by the
  scale. `destroy` and `cleanup` disconnect the pane observer
  and the wheel listener. Export of a zoomed blank report
  matches `createBlank()`.
- [`/elements/003-lithium/src/managers/reports/rulers.js`](/elements/003-lithium/src/managers/reports/rulers.js)
  divides the pointer by the scale before placing the mouse
  line. At scale `1` the line is unchanged.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-scale.test.js`](/elements/003-lithium/tests/unit/reports-scale.test.js)
  (5),
  [`/elements/003-lithium/tests/unit/reports-shell.test.js`](/elements/003-lithium/tests/unit/reports-shell.test.js)
  (37), and
  [`/elements/003-lithium/tests/unit/reports-rulers.test.js`](/elements/003-lithium/tests/unit/reports-rulers.test.js)
  (10). The shell proves a `1000` pixel pane fits at `1.18`,
  zoom in reaches `1.23`, 100% returns `1`, a wheel over the
  page reaches `1.05`, and a zoomed export matches
  `createBlank()`.

### Lessons learned

- Fit is the pane width over the sheet width. The sheet is the
  page plus the 28 pixel bar. Height is outside the formula. A
  pane shorter than the scaled Letter page scrolls vertically.
  `scrollbar-gutter: stable` keeps that bar from stealing the
  width the fit just used. Work item 9.3 says the whole Letter
  page stays visible. This sitting reads that as the full page
  width, with vertical scroll when the pane is shorter. Andrew
  accepted that reading on 2026-10-31.
- One transform scales the sheet, including the ruler bar and
  the corner. Tick coordinates stay at 96 CSS pixels per inch,
  so the 1 inch mark is still 96 layout pixels at 100%. The
  canvas width and height match the visual size, because a
  transform on its own does not change layout.
- happy-dom reports `clientWidth` `0` and does not apply the
  `@layer` rules to computed style. Open paints at `1`. The
  tests set the CSS variable, the tick attribute, and the
  source text of the transform. A real browser fits when the
  stage has a width. If that observer misses, Fit width is the
  control.
- The scale is view state. It is absent from the definition,
  from the undo trail, and from settings. New and Import fit
  again. The corner cycle and undo keep the open scale.
- Andrew asked for five percent per bump, on the buttons and
  on the wheel, the same rate the photo editor uses. The step
  is `0.05` on integer hundredths. The `0.1`–`4` limits stay.
  He accepted them on 2026-10-31. The wheel acts only while
  the pointer is over the page. The summary and the rulers
  still scroll.
- Ctrl+Shift+Z stays the global Toggle zoom shortcut. Report
  zoom has no shortcut. Undo stays Ctrl+Z and redo stays
  Ctrl+Y, manager 24 only.
- `npm run lint` does not expand into `src/managers/reports`.
  `npx eslint` on those files is the extra gate. It exited 0.
- The 12 skipped auth cases are the standing variance. The
  server check for `https://lithium.500courses.com` was
  aborted. That skip is outside this phase's gate.
- happy-dom is the math and the variable. That is not the 500
  Courses check.

### Handoff

Phase 9 is complete. Andrew said the view scale works pretty
well on 2026-10-31. Fit width shows the Letter width in the
designer pane. A shorter pane scrolls vertically. 100% is 96
CSS pixels per inch. The zoom buttons and the wheel over the
page step by 5 points, and the buttons stop at 10% and 400%.
The scale stays off the definition. Phase 10 has not started.
It places, moves, resizes, and selects a static text item.

---

## Phase 10 — Static text

### Goal

Place, move, resize, and select a static text item.

### Entry gate

Phase 9 Status complete.

### Work items

- [x] 10.1 The palette adds a static text item to the selected band.
       The new item writes the Defaults table: box `1.5 in` by
       `0.2 in`, centered in the band, `align` `Left`, `text`
       `Text`. It omits `font` and `mask`.
- [x] 10.2 Drag moves `left` and `top`. Corner resize changes `width`
       and `height`. Commit on pointerup through the model. Clamp to
       the band.
- [x] 10.3 Selection draws an inset outline. Delete removes the item.
       Undo restores position and existence.
- [x] 10.4 The item shows its text. v1 text is the item's `text`
       property, edited in Phase 14. Until then a new item reads
       "Text".
- [x] 10.5 Browser: add, drag, resize, undo, export. The file has
       `typeKey` 0 and unit strings.
- [x] 10.6 The canvas clips item text to the item box. A string
       longer than the box is cut off on screen. Emit clips the same
       way. The band does not grow.

### Done means

A static text item survives export, import, and undo.

### Exit gate

Lint, unit tests for the clamp, and the browser check.

### Status

**complete — 2026-10-31.** Andrew said it works pretty well.
Work items 10.1–10.6 are checked. `npm test`
exited 0 (70 files, 1309 passed, 12 skipped). The skipped
cases are the auth integration file. The Hydrogen server at
`https://lithium.500courses.com` was not available. `npm run
lint` exited 0. `npm run lint:css` exited 0. `npx eslint` on
`src/managers/reports` and the two report unit files exited 0.
`npm run templates:copy` copied 137 templates, including
`reports.html` and `reports.css`. Phase 11 has not started.

### Accomplished

- [`/elements/003-lithium/src/managers/reports/items.js`](/elements/003-lithium/src/managers/reports/items.js)
  paints `typeKey` 0. `staticItemText` returns a stored
  string, including a blank, and reads "Text" when the key
  is absent. `dragItemBox` slides `left` and `top` on a
  move and keeps the top left on a resize. Both stay inside
  the band. A resize is at least 1 CSS pixel when the band
  has that room. `bandItemBounds` uses the content width
  and the stored band height. `defaultBandId` returns
  `detail` when that band exists. `paintBandItems` writes
  the box in CSS pixels, sets the font variables, and
  appends the text and the corner. Other type keys stay in
  the file and off the page.
- [`/elements/003-lithium/src/managers/reports/model/document.js`](/elements/003-lithium/src/managers/reports/model/document.js)
  clamps inside `moveItem` and `resizeItem`. `addItem`
  still writes the Defaults table and leaves an explicit
  box as given. Lengths convert into `page.units`. A
  resize to `0.3 in` at top `0.025 in` on the `0.25 in`
  detail band stores height `0.225 in`. A resize to `0 in`
  stores `0.010417 in`, which is 1 CSS pixel.
- [`/elements/003-lithium/src/managers/reports/bands.js`](/elements/003-lithium/src/managers/reports/bands.js)
  paints the items after the band edge. The band height
  stays the stored height.
- [`/elements/003-lithium/src/managers/reports/reports.html`](/elements/003-lithium/src/managers/reports/reports.html)
  adds Delete after Clear. The shortcut id is
  `reports-delete`. The palette row holds Text
  (`#reports-text-btn`, `fa-font-case`).
- [`/elements/003-lithium/src/managers/reports/reports.css`](/elements/003-lithium/src/managers/reports/reports.css)
  clips `.reports-item` with `overflow: hidden` and
  `white-space: nowrap`. The ink is `#2a2a2a`. The
  selected item uses an inset outline. The selected band
  strip uses an inset accent bar. The corner is 8 CSS
  pixels.
- [`/elements/003-lithium/src/managers/reports/reports.js`](/elements/003-lithium/src/managers/reports/reports.js)
  selects `detail` on New and Import. A band click selects
  that band and clears the item. Text adds `typeKey` 0 on
  the selected band and selects the new item. Drag and the
  corner preview in pixels, divide the pointer by
  `--reports-scale`, and commit on pointerup. An unchanged
  box does not add a history step. Delete removes the item.
  The item id stays so Undo selects it again. A second
  Delete while the item is gone does nothing. The Delete
  key is registered for manager 24.
- Tests:
  [`/elements/003-lithium/tests/unit/reports-items.test.js`](/elements/003-lithium/tests/unit/reports-items.test.js)
  (7) and
  [`/elements/003-lithium/tests/unit/reports-document.test.js`](/elements/003-lithium/tests/unit/reports-document.test.js)
  (10). The shell adds Text on detail at `3.25 in`,
  `0.025 in`, `1.5 in` by `0.2 in`, exports `typeKey` 0
  with unit strings, and proves move, resize, clamp,
  delete, undo, scale, and the clip rules in the
  stylesheet.

### Lessons learned

- `moveItem` and `resizeItem` clamp. `addItem` and import
  leave the box as written. A short band can still receive
  the default `0.2 in` height until the next move or
  resize. The older resize expectation of `0.3 in` on
  detail is now `0.225 in`.
- The resize floor is 1 CSS pixel when the band has that
  room, so a zero box still has a corner. `formatLength`
  stores that pixel as `0.010417 in`. A move slides a box
  and shrinks a side only when that side is already larger
  than the band.
- Corner resize keeps the top left. One handle sits at the
  bottom right. The item is `z-index` 2 and the band edge
  is `z-index` 1, so the corner stays hittable where they
  overlap.
- The palette is one row under the toolbar. This phase adds
  Text. Later phases add their choices to the same row.
  Andrew then locked the manager chrome in the parent plan.
  That lock moves Text onto the third editor toolbar. The
  running shell still has this single row.
  Detail is the selected band on New and Import. A band
  click moves the selection. Text is disabled when no band
  is selected.
- Delete is the toolbar button and the Delete key, manager
  24 only. Undo is the way back, so there is no confirm.
  The item id is kept after Delete so Undo selects the item
  again. New and Import clear the band choice and the item
  id. Selection stays off the definition, the same way the
  view scale does.
- The canvas ink is `#2a2a2a`. A missing text key paints
  "Text". A stored string paints as written. The item font
  wins when it is present. Otherwise the report font is
  used. Otherwise the face is Vanadium Sans, 10 pt, Normal.
  The family has to match a plain name or the paint falls
  back to Vanadium Sans. Only `typeKey` 0 is painted. Field
  items stay in the summary until Phase 15.
- The canvas clips with `overflow: hidden` and
  `white-space: nowrap`. The band does not grow. Emit is
  Phase 24. The page-rules lock already says text clips to
  the box, and that emit has to clip the same way.
- happy-dom canonicalizes `2.4000000000000004px` to
  `2.4px`, so the paint test compares parsed pixels.
  happy-dom does not apply an `@layer` stylesheet to
  computed style, so the clip test reads `reports.css`.
  `npm run lint` does not expand into
  `src/managers/reports`. `npx eslint` on those files is
  the extra gate. It exited 0.
- The 12 skipped auth cases are the standing variance.
  That skip is outside this phase's gate. happy-dom is the
  clamp and the stylesheet text. That is not the 500
  Courses check.

### Handoff

Phase 10 is complete. Andrew said it works pretty well on
2026-10-31. Text on Detail adds, drags, resizes, undoes,
and exports. The file has `typeKey` 0, `text` "Text",
`align` `Left`, and a box of `1.5 in` by `0.2 in`. On
Letter detail the origin is `3.25 in`, `0.025 in`. Font
and mask are absent. Delete removes the item, and Undo
brings it back. A long string is cut off inside the box.
Emit clips the same way in Phase 24. The manager chrome
lock is in the parent, under Designer rules. The running
shell is still the single toolbar and the one Text row.
Phase 11 has not started. It is the lookup 053 packet in
[`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md).
Phase 10-UI, below, is the shell he named next.

## Phase 10-UI — Manager chrome

### Goal

Give Report Manager the Query Manager shell: a report list, an
Editor page, a JSON page, and toolbars that show only on the
page they change.

### Entry gate

Phase 10 Status complete. Andrew named this sidequest Phase
10-UI. The count stays seventy-two phases, numbered 0 through
71.

### Work items

- [x] 10-UI.1 The left pane is a LithiumTable of `reports`.
       There is no QueryRef and no Lookup 059 key. Name and
       summary are the visible columns. Navigator Add starts a
       blank report. Save, duplicate, and delete of a row toast
       and do not store or remove a row.
- [x] 10-UI.2 Editor is the stage. JSON is a CodeMirror buffer
       of the definition with `history` removed. Folds start
       collapsed. A parsed edit commits one history step on blur
       or when the page changes. A parse error toasts and leaves
       the definition.
- [x] 10-UI.3 The main toolbar keeps Import and Export. Zoom
       sits at the right end and shows only on Editor. The
       second toolbar, Editor only, is Static, then Undo, Redo,
       and Clear. The third toolbar, Editor only, is Text, then
       Clone, then Delete. The page-size control and the
       property panel are absent.
- [x] 10-UI.4 Browser check on
       `https://lithium.500courses.com`. Collapse the list,
       place text, clone it, open JSON, and confirm a bad
       buffer leaves the report. On Report, edit the name.
       On the list, Add and type the name. Hold Ctrl or Cmd
       and roll the wheel over the page. A bare wheel scrolls.
- [x] 10-UI.5 Undo, Redo, and Clear show only on the Editor
       toolbar. Clone sits before Delete on the item toolbar.
- [x] 10-UI.6 Report is the first tab. It edits definition
       `name` and `title`, and the list row's `summary`.
       There is no group field.
- [x] 10-UI.7 Name and summary cells use the string and text
       editors. Save, duplicate, and delete of a row still
       fail without a QueryRef.
- [x] 10-UI.8 The wheel over the page zooms only while Ctrl
       or Cmd is held. A bare wheel still scrolls.

### Done means

The author can list, edit, and inspect a definition from one
manager, and a control is hidden when its page is hidden.

### Exit gate

Lint, unit tests for the pages, the clone, and the honest row
actions, then Andrew's check on
`https://lithium.500courses.com`.

### Status

**complete — 2026-10-31.** Andrew said the changes are
working. Work items 10-UI.1 through 10-UI.8 are checked.
`npm test` later in the same stretch exited 0 (73 files,
1335 passed, 12 skipped). The skipped cases are the auth
integration file. `npm run lint` and `npm run lint:css`
exited 0. `npx eslint` on `src/managers/reports` and the
report unit tests exited 0. `npm run templates:copy` copied
137 templates. Phase 11 has not started.

### Accomplished

- The shell follows Query Manager. The left pane lists
  `reports` from an in-memory table definition. The columns
  match `acuranzo_1015.lua`. There is no `collection` column
  and no QueryRef. Navigator Add calls `newReport()`. The
  footer Save and Cancel use `ManagerEditHelper`. Until
  Phases 17 and 18, save and duplicate throw, and delete
  toasts without removing the row.
- Editor keeps `#reports-stage`. JSON uses
  `buildEditorExtensions` with `language: 'json'` and a
  `LithiumEditorFooter`. The buffer is `reportBody`. Folds
  start collapsed on each open. The same bad buffer toasts
  once. The external editor registered for a later save
  still includes `history`.
- Static is the only category. Text is `#reports-text-btn`.
  Delete is `#reports-delete-btn` and the Delete key. Clone
  uses `cloneItem`: same band, new id, same box, and the
  copy is selected. Both stay disabled when no item is
  selected. Switching Static leaves the definition alone.
- `#reports-new-btn`, the page-size control, and the
  property panel are absent. Import and Export stay on the
  main toolbar. The open report name stays there too.
- Finishing touches, same phase: Undo, Redo, and Clear moved
  to the Editor toolbar. Clone then Delete moved onto the
  item toolbar. Report is the first tab and edits `name`,
  `title`, and the row `summary`. There is no group. The
  list cells open because the local string and text coltypes
  now carry `input` and `textarea`. The wheel requires Ctrl
  or Cmd. Save still fails without a QueryRef. Overlapping
  clones stay on the same box until Andrew picks a mark.
  He closed the phase on 2026-10-31 without picking one.
  Save, delete, and the Revisions page landed in Phase 18.

### Lessons learned

- An author `display: flex` rule beats the `hidden` attribute
  inside the same CSS layer. `.reports-manager [hidden]`
  sets `display: none`. Tests assert the attribute, because
  happy-dom does not apply the layered stylesheet.
- LithiumTable's default delete removes the row and toasts
  "Row Deleted" when `deleteQueryRef` is missing. The
  report list replaces that handler. Save and duplicate
  throw `No queryRef configured for …` so the existing
  toast path can run later. This sitting does not call
  `authQuery`.
- `reports.js` stayed at 857 lines by moving the pointer
  methods to `reports-pointer.js`. Those methods call free
  functions, so that file imports them.
- Blur and the following page click both parse one bad
  buffer. The second toast is skipped when the text has
  not changed.
- The JSON editor is created when the JSON page opens, so
  CodeMirror is not measured while the pane is hidden.
- A local coltype map without `editor` never opens a cell.
  `editable: true` is not enough. String uses `input` and
  text uses `textarea`, the same editors as `coltypes.json`.
- The wheel zoomed on a bare scroll. Ctrl and Cmd are the
  modifier already used for shortcuts. Cmd is recorded as
  Ctrl. A bare wheel does not call `preventDefault`.

### Handoff

Phase 10-UI is complete. Andrew said the changes are
working on `https://lithium.500courses.com` on 2026-10-31.
There is no page-size control and no property panel.
Identical overlapping clones still share one box. He did
not pick a mark. Phase 11 has not started. It is the
lookup 053 packet in
[`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md).
Do not start it until he names it. Phase 18 is complete
in that same file.

