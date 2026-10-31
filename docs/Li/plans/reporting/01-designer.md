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
- [ ] 6.4 Browser: resize Detail, export, and confirm the file's
      height changed. Undo restores the canvas and a second export.

### Done means

Band height round-trips through the definition file.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**in progress — 2026-10-31.** The bands are drawn and a drag writes
the height. `npm test` exited 0 (64 files, 1270 passed).
`npm run lint` exited 0. `npm run lint:css` exited 0.
`npm run templates:copy` copied `reports.html` and `reports.css`.
`npx eslint` on `bands.js`, `canvas.js`, `reports.js`,
`reports-file.js`, and `document.js` exited 0. Work item 6.4 is
waiting on Andrew at `https://lithium.500courses.com`. The test
database was offline later the same day, so that check was not
run. Phase 7 started in the next sitting. Andrew later
reported, on the alternate database, that a drag, Undo, and New
behave. The export sentence of 6.4 was not in that report, so
this phase stays in progress. Redo is the toolbar button beside
Undo. Ctrl+Shift+Z and Ctrl+Y redo. A new edit drops that tail.

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
  New and Import start a fresh history. Redo, from the toolbar,
  Ctrl+Shift+Z, or Ctrl+Y, restores the undone snapshot. A new
  edit drops that tail.
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
Andrew said to continue, so Phase 7 started with this check
still open. On the alternate database he reported that a drag,
Undo, and New behave. The export sentence was not in that
report. Redo now sits beside Undo. When he also confirms the
exported height, mark 6.4 and this phase complete, then stop,
unless the same message also reports the Phase 7 check.

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
- [ ] 7.4 Browser: switch the fixture grid style across the five
      values and resize a band. The pattern stays aligned to the
      band origin.

### Done means

The five grid styles render, and cleanup removes the observer.

### Exit gate

Lint, unit tests for the pattern geometry that can run without a
browser, and the browser check.

### Status

**in progress — 2026-10-31.** The five grid styles are drawn.
Andrew reported that the dot pattern was not visible. A circle
of radius `0.5` centered on the tile corner is clipped away.
The painter now draws a full dot on each spacing point. The
painted radius is at least 1 CSS pixel. The stored weight stays
`0.5`. `npm test` exited 0 (65 files, 1266 passed, 12 skipped).
The 12 skipped cases are the auth integration file.
`npm run lint` exited 0. `npm run templates:copy` copied
`reports.html`. `npx eslint` on `grid.js`, `reports.js`, and the
grid tests exited 0. Work item 7.4 is waiting until he sees the
dots after the next deploy. Phase 6's export sentence is still
open. Redo was added in this sitting. Do not start Phase 8.

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

Phase 7 stays in progress. Phase 6 stays in progress too.
Andrew reported drag, Undo, and New on the alternate database.
He also reported that no dot pattern was showing. Deploy this
build. On a new report, each band body shows silver dots on a
12px step, sitting on the band's top-left lattice. Import a
file whose `grid.style` is `grid`, then `dotted`, then
`dashed`, then `none`. The first three draw lines from the
band's top-left corner. `none` draws no marks. Drag Detail's
bottom edge. The pattern stays on that corner and does not
slide. A spacing of `1 in` puts the step on 96 CSS pixels.
Redo sits beside Undo. After Undo, Redo restores the height.
Ctrl+Shift+Z and Ctrl+Y do the same. A new drag clears Redo.
Phase 6's export sentence is still open: a one-inch drag of
Detail stores `1.25 in`, and a second export after Undo is
`0.25 in`. When both checks pass, mark 6.4 and 7.4 and both
phases complete, then stop, unless the same message names
Phase 8. Phase 8 paints the rulers. Do not start Phase 8.

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

