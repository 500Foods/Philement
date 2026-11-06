<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — PDF

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 28–32, and Phase 31-UI
**Stop when:** The author can run a QueryRef from a stored report and download a text PDF. Phase 31-UI is the Preview PDF viewer and is in progress.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 27 in [`/docs/Li/plans/reporting/03-preview.md`](/docs/Li/plans/reporting/03-preview.md) is complete. Phases 28 and 29 are complete. Phase 30 follows its own entry gate. Phase 31-UI follows its own entry gate and does not wait on Phase 30.
**Next file:** [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md) after Phase 32.

## Phase 28 — Query designer

### Goal

The designer stores a QueryRef datasource and its parameters, runs
that query with the session, and previews the rows it returned.

### Entry gate

Phase 27 Status complete. Andrew waived this gate on 2026-11-03 so
there is data to work with before Phase 21.

### Work items

- [x] 28.1 A query datasource stores `kind` `query`, `queryRef`,
       `queryName`, `params` (name to value), `fields` (metadata), and
       `rows`. Rows are stored in the definition, not the session,
       and a Run replaces them. `kind` `json` stays the fixture and
       paste path. The query's metadata lives in its
       `collection.reporting`: `fields` and `parameters` (`name`,
       `type`, `default`; `calculated` comes later).
- [x] 28.2 Reporting queries are query type 3 (lookup 028) and
       status Active. QueryRef 173 lists them sorted by name. The
       list loads with the panel lookups and a Queries toolbar button
       refreshes it. The Datasource rows add Query (picker) and Params
       (JSON, defaults filled in). Run calls each QueryRef through
       conduit `authQuery`. A failed query toasts and keeps the
       previous rows. QueryRef 174 "Reporting Demo" returns synthetic
       rows (`ROW_LIMIT`, `TITLE`, `SCALE`).
- [x] 28.3 A parameter item (lookup 053 key 63) prints one parameter
       with the same `mask` as a field. It may sit on any band.
       Built: new **System** palette category → **Parameter** button
       adds `{ typeKey: 63, param, mask }`. Property **Parameter** lists
       datasource param names (`NAME`, or `ds.NAME` when two share one).
       Acuranzo **1449** teaches Reports.Render compose to print it
       (numeric → number mask, else text; missing → blank + one warn).
       Useful mostly for titles and troubleshooting.
- [x] 28.2b Panel polish: one row per query parameter labelled
       `NAME type` (type in a muted span, e.g. `ROW_LIMIT int`,
       `TITLE str`), typed editors (Number/Boolean/Text) writing into
       `params`; the Params JSON box is gone. **Rows** became **Fields**:
       a read-only multiline `name  type` list (row count in its tooltip).
- [x] 28.4 Browser: QueryRef 174 with `ROW_LIMIT` 2 returns two rows
       and the preview shows two detail bands. The preview request
       includes those rows in `sources`.

### Done means

Andrew runs a QueryRef from the designer and the preview shows two
detail bands.

### Exit gate

Lint and the browser check.

### Status

**Complete** — 28.1, 28.2, 28.2b, 28.3 and 28.4 done. 1447/1448
applied; Acuranzo **1449** (Render Parameter items) accepted after
Andrew's browser check (2026-11-03). Ran ahead of Phase 21 on
Andrew's gate waiver. Phase 21 is Complete; next is **Phase 22**
(03-preview.md), Acuranzo file **1451**; Phase 29 waits its turn.

### Accomplished

- `kind` `query` datasource: `queryRef`, `queryName`, `params`,
  `fields`, `rows` stored in the definition; Run replaces rows.
- QueryRef 173 (List Reporting Queries, acuranzo_1447) and 174
  (Reporting Demo, synthetic rows, acuranzo_1448). Queries toolbar
  refresh; Run via conduit `authQuery`; failure toasts, keeps rows.
- Panel: one typed row per parameter (`ROW_LIMIT int`), read-only
  Fields list.
- System palette → Parameter item (053 key 63), rendered by
  acuranzo_1449; draggable onto a band (follow-up below).

### Lessons learned

- Getting real data in early (waiving the 27 gate) unblocked
  designer testing for everything after; worth the out-of-order phase.
- Query metadata belongs in the query's `collection.reporting`, not
  the report.
- Palette items must be draggable from day one (Andrew's rule).

### Handoff

- Datasource design is settled; later phases should not re-derive it.
- Phase 29 (server-side query execution) still open; next caller-facing
  QueryRef 175.

### Follow-up (not a Phase 28 reopen)

- [x] Palette drag: all future report-editor palette items (e.g.
      System → Parameter and anything similar) must support drag onto
      the canvas, not click-only. Parameter done 2026-11-03: the button
      is draggable and drops `{typeKey: 63, param: ''}` on the band under
      the pointer through the Field drop path (`reports-field:63`). The
      rule stays for future palette items.

---

## Phase 29 — Query execution

### Goal

A render that does not include rows runs the same QueryRef inside
`Reports.Render`.

### Entry gate

Phase 28 Status complete. Phase 17 is applied when the call uses
`report_id`. Re-check that `H.altquery_sync` still accepts a database name,
SQL, and named binds.

### Work items

- [x] 29.1 When `sources` omits a query datasource, `Reports.Render`
       loads `queries.code` with `query_ref` and
       `query_type_a28 = 3` (SQL - Reporting; Andrew chose this on
       2026-11-04 because type 1 does not find QueryRef 174), then runs
       `H.altquery_sync(params._hydrogen.database, code, binds)`
       with the resolved binds. Binds are `datasource.params`: a
       scalar is the value, `{ "parameter": "<name>" }` uses
       `params.parameters` then the definition default, and
       `{ "literal": "<value>" }` uses that value.
       `params.parameters` overrides a same-named bind. Types come
       from `collection.reporting.parameters`. When `sources`
       includes the array, the script does not query. When the
       database name is empty or is not a configured connection,
       the script adds a warn entry and returns no rows. The worker
       login is the one on that connection. Row-level permissions
       stay in Later.
- [x] 29.2 Hand the amendment over. Andrew applied
       `acuranzo_1457.lua` and asked to close the phase
       (2026-11-04). He did not quote the two-row log line.

### Done means

A `report_id` render with no `sources` returns the same two rows as
the designer run.

### Exit gate

Andrew's apply report and that call.

### Status

**Complete** (2026-11-04). Andrew applied
[`/elements/002-helium/acuranzo/migrations/acuranzo_1457.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1457.lua)
and asked to close the phase. He did not quote the two-row log
line. No new QueryRef. No new lookup. The next free Acuranzo file
is **1458**. QueryRef **175** stays unused.

### Accomplished

- `acuranzo_1457.lua` copies the 1456 body. Reverse restores that
  body. Leave `acuranzo_1455.lua` and `acuranzo_1456.lua` as written.
- A query datasource whose name is absent from `sources`, or whose
  value there is not a table, loads
  `SELECT code, collection FROM ${SCHEMA}queries WHERE query_ref = :QREF AND query_type_a28 = 3`
  and runs that `code` with `H.altquery_sync` on
  `params._hydrogen.database`. An array in `sources`, including an
  empty one, is used and the script does not query. A `json`
  datasource with no array still warns
  `datasource <name> has no array in sources; no rows`.
- Binds come from `datasource.params`. A scalar is sent as stored.
  `{parameter}` reads `params.parameters`, then
  `definition.parameters[].default`. `{literal}` sends that value.
  `params.parameters` overrides a same-named bind. Types follow
  `collection.reporting.parameters` (object or a one-element array).
  `integer` / `int` are integers, `float` / `number` are numbers,
  `boolean` / `bool` are booleans, and string-like types are text.
- An empty session database warns
  `datasource <name> has no session database; no rows`.
  A connection `H.altquery_sync` reports as not found warns
  `datasource <name> database <db> is not configured; no rows`.
  A missing row warns
  `datasource <name> query <ref> is not installed; no rows`.
  A query error warns
  `datasource <name> query <ref> failed: <err>; no rows`.
  A missing `queryRef` warns
  `datasource <name> has no queryRef; no rows`.
  An unset bind warns
  `datasource <name> parameter <name> is not set; no rows`.
  A run that returns rows adds
  `datasource <name> ran query <ref>; <n> rows` at info.
  Stored `datasource.rows` are not a row source.
- `luacheck` on `acuranzo_1457.lua` reported 0 warnings. Lua 5.5.1
  loaded the new body. The probe ran QueryRef 174 with `ROW_LIMIT` 2
  and got Alpha and Bravo, left a supplied array alone, honored
  `params.parameters` and the lock's bind forms, and warned on an
  empty database, a missing connection, a missing query, a query
  error, a missing `queryRef`, and an unset parameter. No JavaScript
  changed, so `npm test` was not required.

### Lessons learned

- The phase text said `query_type_a28 = 1`. QueryRef 174 is type 3.
  A type-1 lookup cannot return the two demo rows. Andrew chose
  type 3 only.
- Phase 28 stores plain values on `datasource.params`. The lock's
  `{parameter}` and `{literal}` forms still resolve. The client does
  not send `params.parameters` today, so a saved `ROW_LIMIT` of 2
  stays 2.
- Compose and Preview HTML still fetch an un-run query in the
  browser and send the rows in `sources`. That path does not run
  the new server query. The check is a call that omits `sources`.
- A parameter item still prints `datasource.params`. A
  `params.parameters` override changes the query bind and does not
  change that printed item.
- PostgreSQL sends binds as text. An integral `SCALE` stays a Lua
  number. `CAST(:SCALE AS double precision)` accepts that text.
- Andrew's close reported the apply and asked to mark the phase
  complete. He did not quote the two-row log line or the Alpha and
  Bravo bands.

### Handoff

- Andrew applied [`/elements/002-helium/acuranzo/migrations/acuranzo_1457.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1457.lua).
  Leave `acuranzo_1455.lua`, `acuranzo_1456.lua`, and
  `acuranzo_1457.lua` as written. Reverse of 1457 restores the
  1456 body.
- A query datasource with no array in `sources` loads
  `query_type_a28 = 3` and runs that SQL with `H.altquery_sync`
  on the session database. An array in `sources`, including an
  empty one, still wins. Preview HTML and Compose still send
  `sources`, so they do not exercise this path.
- The call he did not quote is `Reports.Render` with `report_id`,
  `rev_id`, `stage` `compose`, and no `sources` and no
  `definition`. The saved query is QueryRef 174 with `ROW_LIMIT`
  2. The log line is `datasource <name> ran query 174; 2 rows`.
  Do not send `params._hydrogen`.
- Phase 30 (Seed lua-pdfkit) has not started. Its body is below.
  Next free Acuranzo file is **1458**. QueryRef **175** stays
  unused. No new lookup.

---

## Phase 30 — Seed lua-pdfkit

### Goal

Put the pure-Lua PDFKit port into `scripts` so `require` loads it.
No report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–29 may still be open. Phases 31 and 52
wait for the apply.

### Work items

- [ ] 30.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `lua-pdfkit.*` module (`init`,
       `document`, `page`, `path`, `image`, `font.afm`, …).
       `invokable = 0`, `mcp_access = 0`. Group name `lua-pdfkit`.
       The script name is the require path after the first dot. One
       library per file, the exception to one-script-per-migration.
       Reverse deletes only those rows. The packet records the commit.
       If neither the port repository nor the task-tree path is
       present, stop and ask.
- [ ] 30.2 Confirm a standard face (`Helvetica`) renders with `io`
       nil. If the port still `io.open`s an AFM file, bundle that AFM
       text into the seeded module before handing the packet over. A
       phase that needs `io` stops and asks.
- [ ] 30.3 Hand the packet over. After apply, Andrew runs a one-page
       probe: `require("lua-pdfkit.init")`, `doc:text`, `doc:finish()`,
       and `doc:output()`. The probe returns a PDF byte length.
       `AllowDBModuleLoad` is on for that call.

### Done means

Andrew reports the probe PDF length from inside Hydrogen, with `io`
left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 31 — PDF text

### Goal

`stage=pdf` returns a text PDF of the composed pages, and `pdf`
becomes the default stage.

### Entry gate

Phase 30 and Phase 21 Status complete.

### Work items

- [ ] 31.1 Amend `Reports.Render` in a new migration. Do not edit an
       applied file. Re-read one script-update migration before
       writing.
- [ ] 31.2 `PDFDocument.new` uses the definition page size. One
       `addPage` per intermediate page. Text uses a standard face
       the port already carries. Positions are in PDF points. Align,
       font size, and font style come from the item, then the
       definition font. Bold and italic are the PDF flags. The face
       stays that standard face when `embedFonts` is false or
       absent. When `embedFonts` is true, each used non-standard
       family is one subset TrueType from `params.fonts`, referenced
       for every placement. A standard face is never attached.
       Text clips to the item box when
       `fitHeight` is `clip`. When the item grows or shrinks, paint
       the shared IR lines in the used box at the shared line height,
       in the face those lines were measured with.
- [ ] 31.3 A call that omits `stage` now runs `pdf`. The detail
       fixture returns `{ "pdf_base64": "...", "log": [] }` and
       opens with the right page count. The result-cap behavior is
       Phase 32.
- [ ] 31.4 Hand the packet over. Andrew's call opens as a PDF with
       the right page count.

### Done means

The detail fixture opens as a PDF with the right page count.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

Phase 31-UI, below, is the viewer he named on 2026-11-05. It may
run while Phase 30 and Phase 31 are still open. Phase 31 later
points that same viewer at `pdf_base64` and adds Run.

## Phase 31-UI — Preview PDF viewer

### Goal

Preview PDF shows a multi-page file in Lithium's own chrome. The
page loads a fixture until a report emits bytes. Documents reuses
this viewer later. This phase does not emit a PDF.

### Entry gate

Phase 25 Status complete. Phases 30 and 31 may still be open.
Andrew named this sidequest Phase 31-UI. The count stays
seventy-two phases, numbered 0 through 71. Naming the phase
starts it. No migration, no new lookup, and no new QueryRef.
The page does not call `Reports.Render` and does not emit
`stage=pdf`.

### Work items

- [x] 31-UI.1 Add `pdfjs-dist` as a project dependency. This work
      item is the permission for that dependency. The worker is
      bundled with the app. Lithium draws its own chrome. The
      stock Firefox viewer stays out. `jsPDF` stays out. A CDN
      script stays out.
- [x] 31-UI.2 The fixture is
      `elements/003-lithium/public/assets/documents/test.pdf`,
      served at `/assets/documents/test.pdf`. It has seven pages,
      selectable words, and a nested outline. Every destination
      in this file is `/Fit` (the page, and no point). A
      destination that names a point still scrolls to that point.
      The file is not packed into the Hydrogen payload. Until a
      report returns `pdf_base64`, Preview PDF loads this fixture.
- [x] 31-UI.3 A second toolbar shows only on page `pdf`, the way
      `#reports-categories` shows only on Editor. The Preview PDF
      tab stays. Import and Export stay on that page. The
      designer zoom controls stay on Editor. Left to right:
      a thumbnails toggle; first, previous, a page field and
      the page count, next, last; zoom out, the scale, fit
      page, zoom in, fit width; rotate left and rotate right;
      flip horizontal, flip vertical, and flip both; search,
      with next and previous hit; the outline toggle; Download
      and Print; text, hand, and marquee; PDF to text and OCR.
      Marquee sits beside those two. Those last two stay
      disabled. Their titles say they
      belong to Documents. Do not extract text and do not run
      OCR. Do not build a Documents manager.
- [x] 31-UI.4 The body replaces the note in `#reports-page-pdf`.
      It is a framed thumbnail strip and a continuous stack. The
      strip uses the property panel's padding, header, and footer.
      Zoom, in 5 percent steps, is in the header. Column count,
      from 1 through 5, is in the footer. The toolbar toggle shows
      or hides the strip. There is no splitter. Every page uses
      one cell. At 100% one row is as tall as the frame's body and
      the columns share its width, so that row has no gap around
      it. The page is contained and centered in the cell. The page
      number sits on the bottom row of every cell. Pages near the
      viewport are drawn, so a long file does not paint every
      page at once. The page that occupies the pane is current.
      That thumb is highlighted and scrolled into view. Clicking
      a thumb scrolls the stack to that page. Arrow keys move
      through the thumb grid, including across columns. Home and
      End go to the first and last page. Page Up and Page Down
      move the stack.
- [x] 31-UI.5 A page's `/Rotate` is 0, 90, 180, or 270. That value
      is which side is up. The viewer's quarter-turns and flips
      sit on top of it and do not replace it. Rotation in this
      phase is for the whole document. Turning one page of a scan
      is a Documents follow-on. Flip horizontal and flip vertical
      are independent. Flip both turns both mirrors on, and a
      second click clears both. Thumbnails use the same rotation
      and the same flips. These are view transforms. The file is
      not rewritten. An open action and `/ViewerPreferences` are
      not orientation.
- [x] 31-UI.6 Fit page is the control he called 100%. It fits one
      page into the pane. Fit width sits beside it. Zooming out
      leaves fit page, so a smaller scale can show a stack of
      about three pages. Resizing the pane refits only while that
      fit mode is still on. The numeric scale runs from about 10%
      to 800%. The wheel and the buttons multiply the current
      scale. Ctrl or Cmd plus the wheel matches the design
      surface. A bare wheel scrolls. Double-clicking a page
      toggles fit page and the last explicit scale. The
      designer's 100% stays 96 CSS pixels per inch, and it stays
      on Editor. PDF user space stays 72 points per inch. The
      viewer's fit page is neither of those scales.
- [x] 31-UI.7 The outline is nested. Clicking an entry scrolls to
      its page and, when the PDF names a point, to that point.
      The entry for the current page stays highlighted. An empty
      outline shows a quiet empty state. Search shows a count
      such as "3 of 12", highlights every hit, and Enter and
      Shift+Enter move to the next and previous hit. The hit
      scrolls into the stack. Escape closes search. The search
      words are not stored.
- [x] 31-UI.8 Text, marquee, and hand are three pointer modes, so
      one drag does not do all three. Text selection uses the PDF
      text layer, and copy takes that text. The marquee grabs a
      rectangle of the page and copies it to the clipboard as a
      PNG. It sits beside the disabled PDF-to-text and OCR
      buttons, where a later Documents pass will read that
      region. Saving the marquee as a file waits. The hand tool
      pans.
- [x] 31-UI.9 Download and Print use the bytes on screen. Both
      work for the fixture, and both are how a finished report
      leaves the page once Phase 31 supplies `pdf_base64`. A
      password prompt appears only when the file asks for one.
      There is no toolbar button for it.
- [x] 31-UI.10 Remember the view in `window.lithiumSettings` at
      `lithium_reports_pdf`. Remember columns, whether the strip
      is open, thumbnail zoom as a percent, fit or numeric
      mode, the numeric scale, rotate, both
      flips, whether the outline is open, whether search is
      open, and the pointer mode. Do not store the search words.
      Remember the page, per report once real files exist, and
      per the fixture until then. No database column.
- [x] 31-UI.11 Tests cover column count, fit against an explicit
      scale, flip composition, a quarter-turn on top of page
      `/Rotate`, and a settings round-trip. From
      `elements/003-lithium`, `npm test`, `npm run lint`, and
      `npm run lint:css`. HTML changes run `npm run templates:copy`.
      Each new source file stays at or under 1000 lines. The
      viewer module lives under `src/managers/reports/`. CSS
      stays in the reports layer. CSS-first. No `alert`,
      `confirm`, or `prompt`. `log()` is the logger.
- [ ] 31-UI.12 Browser check on
      `https://lithium.500courses.com`. Andrew opens Preview PDF,
      fits a page, zooms out until several pages show, moves
      through the thumbs with the keyboard, opens the outline
      and jumps, searches, selects text, and copies a marquee.
      PDF to text and OCR are visible and disabled.

### Done means

Andrew opens Preview PDF on the fixture. One page fits the pane.
Zooming out shows a stack of pages. The current thumb stays
highlighted. The outline jumps. Search counts hits. Selected text
copies. A marquee copies as a PNG. PDF to text and OCR stay
disabled.

### Exit gate

`npm test` and `npm run lint` from `elements/003-lithium`, then
Andrew's browser check. Set Status to **in progress** after the
tests, and leave the browser check for him.

### Status

**in progress**

### Accomplished

- Preview PDF loads
  `public/assets/documents/test.pdf`
  (`/assets/documents/test.pdf`). pdf.js 6.5.438 reads 7 pages
  and 23 nested outline entries. Every destination is `/Fit`,
  so a click opens that page. `destTarget` still returns a
  point for XYZ, FitH, FitV, and FitR. Page 7 `/Rotate` is 90.
  The last finished work item is 31-UI.11.
- The pdf toolbar, the stack, the thumbs, and the outline live
  in `reports.html`. `showPage('pdf')` calls `openPdf`, which
  imports `reports-pdf.js`. The worker is bundled.
  `vite.config.js` puts pdfjs in `vendor-pdfjs`.
- View math is `pdf-view.js`. Search and the outline list are
  `reports-pdf-find.js`. Paint is `reports-pdf-render.js`. CSS
  is `reports-pdf.css` inside `@layer managers`.
  The thumbnail strip is `reports-pdf-thumbs.js`.
  `reports-pdf.js` is 947 lines.
- The settings key is `lithium_reports_pdf`. Search words stay
  out of that object. PDF to text and OCR stay disabled.
- The thumbnail strip is framed like the property panel.
  Zoom, in 5 percent steps, is in the header. Column count is
  in the footer. The upper toolbar keeps one thumbnails toggle.
  Every page uses one cell. The page is contained and centered,
  and the number sits on the bottom row. At 100% one row fills
  the frame's body.
- From `elements/003-lithium`, `npm test` was 99 files, 1553
  passed, 12 skipped. `npm run lint` and `npm run lint:css`
  exited 0. `npm run templates:copy` copied 143 templates.
  The viewer was not opened in a browser.

### Lessons learned

- `getViewport` scale is PDF user space. Numeric 100% multiplies
  by 96/72. Fit page is its own mode, and that scale is left
  unclamped.
- pdf.js `getOutline()` nests children in `items`. qpdf JSON
  uses `kids`. `flattenOutline` walks both.
- `/Fit` names a page and no point. `_goTo` scrolls to the page
  top when `left` and `top` are both empty. A flipped view maps
  a named point through `height - py` and `width - px`.
- `onPassword` is a callback. The dialog calls `update` later.
  Cancel calls `task.destroy()`.
- `getDocument({ data })` transfers the typed array. Download
  keeps `bytes.slice()`.
- The modern pdf.js build prints "Please use the `legacy` build
  in Node.js environments" under Vitest. The browser bundle
  stays the modern build. The warning can land on a later chrome
  test, because the Preview PDF click does not await the import.
- One full `npm test` saw the JSON fold count stay 0 inside
  `shows the editor chrome and hides it on the JSON page`. The
  chrome file alone, and a second full run, both passed. The
  assertion was left as written.
- The fixture stays `public/assets/documents/test.pdf`. Deploy
  copies that file. The page fetches `/assets/documents/test.pdf`.
  Vite does not hash it. The pdf.js worker is the same kind of
  file: `scripts/copy-pdf-worker.js` writes
  `assets/pdf.worker.min.js`. Hydrogen labels `.js` as JavaScript
  and does not label `.mjs`, so the Vite-hashed `.mjs` worker
  never started.
- Thumbnail zoom is a percent, in steps of 5, from 25% to 400%.
  100% makes one row as tall as the framed body, and the columns
  share that width. Each page is contained in that fixed cell.
  The number is a 16px row at the bottom. A saved thumb size of
  64, 96, 128, or 176 pixels becomes 100%. A saved column count
  of 0 closes the strip and keeps one column.

### Handoff

Andrew opens Preview PDF on
`https://lithium.500courses.com`. The thumbnail strip is framed
like the property panel. Zoom is in its header, in 5 percent
steps, and column count is in its footer. The upper toolbar
only toggles the strip. At 100% one row fills the frame. Fit a
page, zoom out until several pages show, move through the thumbs
with the keyboard, open the outline and jump, search, select
text, and copy a marquee. The outline entries are his bookmarks. Each one is
`/Fit`, so the jump opens the page. A destination that names a
point scrolls to that point. Page 7 is `/Rotate` 90. PDF to text
and OCR stay disabled. 31-UI.12 stays unchecked. The phase stays
in progress until he reports that check. The fixture stays
untracked under `public/assets/documents/`. Phases 53 and 54
stay in progress. Do not start Phase 30, Phase 31, Phase 45,
Phase 50, or Phase 55. The next free Acuranzo file is 1480.
QueryRef 175 stays unused. Lookup 080 and lookup 054 key 53
stay free.

---

## Phase 32 — Result cap

### Goal

A PDF whose JSON does not fit the result cap returns
`result_too_large` instead of a rejected body.

### Entry gate

Phase 31 Status complete.

### Work items

- [ ] 32.1 The script measures the encoded JSON before
       `H.set_result_json`. When it does not fit, the script submits
       `{ "error": "result_too_large", "cap": N, "size": N }`.
       The ceiling is `SCOREBOARD_RESULT_JSON_MAX` (1 MiB).
       `ClientInvokeMaxResultBytes` can only lower it. Write that
       into the contract note.
- [ ] 32.2 One probe builds a result over the cap and receives that
       error object. The Phase 31 detail fixture still returns
       `pdf_base64`.
- [ ] 32.3 Hand the amendment over.

### Done means

The over-cap probe returns `result_too_large` with `cap` and `size`,
and the detail fixture still returns `pdf_base64`.

### Exit gate

Andrew's apply report and the two calls.

### Status

**not started**

