<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — PDF

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 28–32
**Stop when:** The author can run a QueryRef from a stored report and download a text PDF.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 27 in [`/docs/Li/plans/reporting/03-preview.md`](/docs/Li/plans/reporting/03-preview.md) is complete. Phases 28 and 29 follow their own entry gates.
**Next file:** [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md) after Phase 32.

## Phase 28 — Query designer

### Goal

The designer stores a QueryRef datasource and its parameters, runs
that query with the session, and previews the rows it returned.

### Entry gate

Phase 27 Status complete.

### Work items

- [ ] 28.1 The definition stores `parameters`: `name`, `type`
       (`text`, `number`, `date`, or `boolean`), and `default`. A
       query datasource stores `kind` `query`, `queryRef`, and
       `params` mapping each bind to a parameter or a literal.
       `kind` `json` stays the fixture and paste path. Sample rows
       stay session state and are not exported.
- [ ] 28.2 The designer edits that list and shows a run strip: one
       control per parameter, defaults filled in. Run calls each
       QueryRef through conduit `authQuery` with the session JWT
       and stores the rows as the session sample. A failed query
       toasts and leaves the previous sample.
- [ ] 28.3 A parameter item (lookup 053 key 63) prints one parameter
       with the same `mask` as a field. It may sit on any band.
       Phase 20 formats that mask.
- [ ] 28.4 Browser: a small QueryRef returns two rows and the preview
       shows two detail bands. The preview request includes those
       rows in `sources`.

### Done means

Andrew runs a QueryRef from the designer and the preview shows two
detail bands.

### Exit gate

Lint and the browser check.

### Status

**not started**

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

- [ ] 29.1 When `sources` omits a query datasource, `Reports.Render`
       loads `queries.code` with `query_ref` and
       `query_type_a28 = 1`, then runs
       `H.altquery_sync(params._hydrogen.database, code, binds)`
       with the resolved binds. `params.parameters` overrides
       defaults. When `sources` includes the array, the script does
       not query. When the database name is empty or is not a
       configured connection, the script adds a warn entry and
       returns no rows. The worker login is the one on that
       connection. Row-level permissions stay in Later.
- [ ] 29.2 Hand the amendment over. A `report_id` render with no
       `sources` returns the same rows the designer run returned.

### Done means

A `report_id` render with no `sources` returns the same two rows as
the designer run.

### Exit gate

Andrew's apply report and that call.

### Status

**not started**

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
       stays that standard face. Text clips to the item box.
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

