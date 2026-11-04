<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Formulas

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 33–49
**Stop when:** The author can use formulas, totals, and breaks, and can download CSV.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phases 33–37 have not started. Phase 38 is complete (2026-11-04). Phase 39 is complete (2026-11-04). Andrew said the latest migration applied is 1459, and that aggregate functions in page headers and footers work as indicated. Phases 40, 41, and 42 are complete (2026-11-04). Andrew said they are fully operational and confirmed the values returned. He did not name `acuranzo_1460.lua`, `acuranzo_1461.lua`, or `acuranzo_1462.lua`, and he did not quote the fixture figures. Phases 43 and 44 are complete (2026-11-04). Andrew said they look good. He did not name `acuranzo_1463.lua` or `acuranzo_1464.lua`, and he did not quote the fixture figures. That check is the apply report. The next free Acuranzo file is **1465**. QueryRef **175** stays unused. No new lookup. Phases 44a and 44b are written and have not started. Phase 45 has not started.
**Next file:** [`/docs/Li/plans/reporting/06-graphics.md`](/docs/Li/plans/reporting/06-graphics.md) after Phase 49.

## Phase 33 — Text formula

### Goal

`Reports.Render` compiles an author function and a calculated text
item prints what it returns.

### Entry gate

Phase 32 Status complete. lua-pdfkit, lua-zint, and d3.lua stay on
`require`. Author formulas do not.

### Work items

- [ ] 33.1 Implement the User Lua contract. Compile every function at
       the start of compose with `load(source, name, "t", env)`. A
       compile error returns through `H.set_result_json` and does not
       throw out of the worker. A runtime error warns and yields an
       empty value. An unknown function name is a compose error.
       This is the one compiler. Chart `custom` waits for Phase 64.
- [ ] 33.2 Fixture function `pack_code`, `returns` `text`. Letters
       `A` through `L` are months 1 through 12. The return value is
       that letter, the week digit, and the day as two digits. Row
       `month=3`, `week=2`, `day=5` renders `C205`. The fixture
       definition is JSON. The editor is Phase 34.
- [ ] 33.3 Hand the amendment over. After apply, the fixture renders
       `C205`.

### Done means

The pack-code fixture renders `C205`.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 34 — Formula editor

### Goal

The designer edits a function's source and `returns`, and the
definition file round-trips them.

### Entry gate

Phase 33 and Phase 14 Status complete.

### Work items

- [ ] 34.1 Palette shows lookup 053 keys 28, 29, and 32. The item
       stores the function name. The Functions list and that item
       share one CodeMirror 6 buffer, `language: 'lua'`, Vanadium
       Mono, and the shared footer. The list shows name, `returns`,
       and references. The item picker offers functions whose
       `returns` matches the item, and Edit focuses the same buffer.
       A new function writes the Defaults table's `returns` and
       `source`. Source is saved when the editor blurs. The browser
       does not run the Lua. Diagnostics are Phase 35.
- [ ] 34.2 Browser: edit the pack-code source, export, reload, and
       see the same source and `returns`. The document model stores
       the source and does not execute it.
- [ ] 34.3 Unit tests cover that storage if a pure helper exists.

### Done means

The designer round-trips function source and `returns` in the
definition file.

### Exit gate

Lint, the unit test, and the browser export/reload.

### Status

**not started**

---

## Phase 35 — Formula check

### Goal

`stage=check` underlines a Lua compile error in the formula editor.

### Entry gate

Phase 33 and Phase 34 Status complete.

### Work items

- [ ] 35.1 `stage=check` compiles the posted functions with the same
       `load` path as compose and returns `{ name, line, message }`
       for each failure. No rows are read.
- [ ] 35.2 Add `@codemirror/lint` to the direct dependencies. The
       editor debounces that call for the open function and underlines
       the line. Empty source is clean. Typing is never blocked, and
       a bad chunk can still be saved. Compose still refuses to render
       a chunk that does not compile. Undefined names such as `io` or
       `H` are not gutter diagnostics.
- [ ] 35.3 Browser: type `function(ctx` with no `end`, see the
       diagnostic, close the chunk, and see the diagnostic clear.

### Done means

An unclosed chunk shows a CodeMirror diagnostic, and closing the
chunk clears it.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 36 — Number and datetime formula

### Goal

A calculated number and a calculated datetime print through the item
mask.

### Entry gate

Phase 35 Status complete.

### Work items

- [ ] 36.1 A function with `returns` `number` returns the raw number.
       The item mask formats it. Fixture: return `1234.5` with mask
       `#,##0.00` and print `1,234.50`. A function with `returns`
       `datetime` returns a value the datetime mask accepts, and the
       item mask formats it. The calculated number and calculated
       datetime use the mask editor from work item 15.5.
- [ ] 36.2 Hand the amendment over. Browser: the masked number prints
       `1,234.50`.

### Done means

The number fixture prints `1,234.50`, and the datetime fixture prints
through its mask.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 37 — Formula runtime errors

### Goal

One bad function call warns and prints empty, and the rest of the
report still renders.

### Entry gate

Phase 36 Status complete.

### Work items

- [ ] 37.1 A row whose function raises comes back empty with a
       warning. A number function that returns a string warns once
       per item and prints empty.
- [ ] 37.2 Hand the amendment over. The fixture matches, and the
       other rows of that report still render.

### Done means

The raising row and the type-mismatch row each warn and print empty,
and the other rows still render.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 38 — Count, unique, and sum

### Goal

Group and report headers and footers can show count, filled, unique,
and sum.

### Entry gate

Phase 37 Status complete. Andrew named Phase 38 on 2026-11-04 and
left Phases 33–37 as they are. A field total does not use the
formula compiler.

### Work items

- [x] 38.1 An aggregate item stores `op` and either `field` or
       `function`. This phase's ops are `count`, `filled`, `unique`,
       and `sum`. Numeric results use lookup 053 key 35. A text
       result uses key 34. The picker offers a function only when
       its `returns` allows the op. The fixture uses a field.
- [x] 38.2 Compose reduces the raw values for the current group and
       for the whole detail source, and places them on
       `ctx.aggregates`. `count` counts every row. `filled`,
       `unique`, and `sum` skip nulls. An empty `count`, `filled`,
       or `unique` is 0. An empty `sum` is an empty string. A
       non-numeric value in `sum` warns and is skipped. The header
       and the footer of one scope show the same total. The item
       mask formats the reduction.
- [x] 38.3 The designer places the item on a group header, a group
       footer, a report header, or a report footer.
- [x] 38.4 Fixture: two groups. Hand the amendment over. The preview
       shows each op this phase names, and the header matches the
       footer. The Lua probe passed. The preview is Andrew's check.

### Done means

The two-group fixture shows count, filled, unique, and sum, and the
header matches the footer.

### Exit gate

Lint, the script packet apply, the fixture call, and the browser
preview.

### Status

**Complete** (2026-11-04). Andrew said it works well, that 1457
has been applied, and that the aggregate functions were tested
with no issues found. He did not name
[`/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua)
or quote the fixture totals. The packet for this phase is that
file. No new QueryRef. No new lookup. The next free Acuranzo file
is **1459**. QueryRef **175** stays unused.

### Accomplished

- Andrew named Phase 38 on 2026-11-04. Phases 33–37 stay as they
  are. The written entry gate was Phase 37. A field total does not
  need that compiler.
- The Aggregate category places lookup 053 key 35 on a report
  header (12), group header (14), group footer (16), or report
  footer (18). Detail and page bands are refused. A new item stores
  `op` `count` and `field` `""`. Op is a client row. Setting a
  field clears `function`. Setting a function clears `field`.
  `sum` is offered for a number function. `count`, `filled`, and
  `unique` are offered for text, number, and datetime. A missing
  `returns` is text. Boolean and `rows` are omitted. No new lookup.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua)
  copies the 1457 `Reports.Render` body. Reverse restores that
  body. Leave `acuranzo_1455.lua` through `acuranzo_1458.lua` as
  written. The raw total is
  `ctx.aggregates[op][field]`. The map stays out of the emitted
  JSON. The item mask formats it as a number. A detail row and a
  page band have no aggregates.
- `count` counts every row. `filled`, `unique`, and `sum` skip
  nulls. An empty `count`, `filled`, or `unique` is 0. An empty
  `sum` prints blank. A sum value that is not a Lua number warns
  once per item and is skipped. A string `"10"` stays out of the
  sum. Unique keeps a number and the same text distinct. The
  header and the footer of one scope share one reduction.
  Consecutive group spans stay separate when the same key returns
  later. A function name warns once and prints blank.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/totals.definition.json`](/elements/003-lithium/tests/fixtures/reporting/totals.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/totals.sources.json`](/elements/003-lithium/tests/fixtures/reporting/totals.sources.json).
  The definition also stores those rows, so an import sends them
  in `sources`. Mask `#,##0.00`. East, West, East again, and
  South. East is count `3.00`, filled `1.00`, unique name `2.00`,
  unique amount `3.00`, sum `30.00`. West is `1.00`, `1.00`,
  `1.00`, `1.00`, `5.00`. The second East is `1.00`, `0.00`,
  `1.00`, `1.00`, `7.00`. South is `1.00`, `0.00`, `1.00`,
  `0.00`, and a blank sum. The report is `6.00`, `2.00`, `4.00`,
  `5.00`, `42.00`. Each header matches its footer. The log is
  four warns, one each for the group header, group footer, report
  header, and report footer sum:
  `sum skipped a value that is not a number`.
- `luacheck` on `acuranzo_1458.lua` reported 0 warnings. Lua 5.5.1
  loaded the new body. The probe matched those totals, the HTML
  text, an empty source (count `0.00`, sum blank, no warn), a
  page-band blank, an unbound field blank, a function blank, and
  a detail blank. The grouped fixture still matches
  `grouped.intermediate.json`.
- `npm test` was 93 files, 1472 passed, 12 skipped. The skipped
  file is the live auth check; the server check aborted. `npm run
  lint` and `npm run lint:css` exited 0. `npm run templates:copy`
  copied `reports.html`.
- Andrew said it works well on 2026-11-04. He said 1457 has been
  applied and the aggregate functions were tested with no issues
  found. He did not name `acuranzo_1458.lua`. He did not quote
  East `30.00`, the report `42.00`, the four sum warns, or that
  the header matches the footer. The packet for this phase is
  `acuranzo_1458.lua`. `acuranzo_1457.lua` is the Phase 29 query
  script.

### Lessons learned

- Compose breaks a group when consecutive keys differ, and it does
  not sort. The totals follow those consecutive spans. A later
  East is its own group.
- `ctx.aggregates` is keyed `aggregates[op][field]`. A later
  formula can read `ctx.aggregates.sum.amount`. A function name
  will use that same slot when the compiler exists. The emitted
  item stays `id`, `typeKey`, `text`, `align`, `font`, and `box`.
- Sum accepts a finite Lua number. It does not coerce a numeric
  string. Nulls are skipped with no warn. One bad value warns
  once on each sum item that saw it.
- An empty sum is nil, so the mask prints blank. An empty count,
  filled, or unique is the number 0, and the mask prints `0.00`.
- Key 35 already has the field attribute. Op and Function are
  client rows. The function list is empty until the report has a
  function whose `returns` allows the op.
- Andrew's close-out named file 1457. That file is the Phase 29
  query script. The aggregate packet is `acuranzo_1458.lua`. He
  said the aggregate functions were tested and no issues were
  found. He did not quote the fixture totals. Those four ops are
  field reductions. A named function still prints blank until
  Phase 33.

### Handoff

- Phase 38 is complete (2026-11-04). Andrew said it works well.
  He said 1457 has been applied and the aggregate functions were
  tested with no issues found. He did not name
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1458.lua)
  or quote the fixture totals. The packet for this phase is that
  file. Leave `acuranzo_1455.lua`, `acuranzo_1456.lua`,
  `acuranzo_1457.lua`, and `acuranzo_1458.lua` as written.
  Reverse of 1458 restores the 1457 body.
- Next free Acuranzo file is **1459**. QueryRef **175** stays
  unused. No new lookup. No new QueryRef.
- A named function still prints blank and warns
  `function totals are not available yet` until Phase 33. A page
  band still prints blank. That placement is Phase 39.
- Phase 39 (Page totals) was previewed and not started. Its entry
  gate is Phase 38 and Phase 21. Both are complete. Do not start
  it in this conversation.

---

## Phase 39 — Page totals

### Goal

A page header or page footer shows the sum of the detail rows on
that page.

### Entry gate

Phase 38 and Phase 21 Status complete. Both are complete
(2026-11-04). Andrew said "please continue" after the peek.

### Work items

- [x] 39.1 A page-footer aggregate reduces the detail rows placed on
       that page. The same window serves later ops. The page header
       and the page footer show the same page total.
- [x] 39.2 The designer can place the aggregate on a page header or
       a page footer.
- [x] 39.3 Fixture: the page sum sits beside the report sum, and the
       two differ when the rows span two pages. Hand the amendment
       over.

### Done means

The fixture shows a page sum and a report sum, and they differ across
a two-page report.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said the latest migration
applied is 1459, and that aggregate functions in page headers
and footers work as indicated. He did not quote the page sums,
the counts, or the names. The packet is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua).
No new QueryRef. No new lookup. The next free Acuranzo file is
**1460**. QueryRef **175** stays unused.

### Accomplished

- Andrew said "please continue" after the Phase 39 peek. Phase 38
  and Phase 21 are complete. Phases 33–37 stay as they are.
- The Aggregate category places lookup 053 key 35 on a page header
  (13) and a page footer (17), as well as the group and report
  bands from Phase 38. A detail band is still refused. No new
  lookup. No new QueryRef.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua)
  copies the 1458 `Reports.Render` body. Reverse restores that
  body. Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as
  written. After the page list exists, each page reduces the
  detail rows placed on that page. The page header and the page
  footer share that reduction. `count`, `filled`, `unique`, and
  `sum` use the same rows. Group and report totals are unchanged.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/pagetotals.definition.json`](/elements/003-lithium/tests/fixtures/reporting/pagetotals.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/pagetotals.sources.json`](/elements/003-lithium/tests/fixtures/reporting/pagetotals.sources.json).
  The definition stores the three rows, so an import sends them
  in `sources`. The page is `2 in` tall with `0 in` margins.
  Detail is `0.5 in`. The report header, page header, page
  footer, and report footer are `0.25 in`. Mask `#,##0.00`.
  Page 1 is Ann `10` and Bo `20`: count `2.00`, sum `30.00`.
  Page 2 is Cy `7`: count `1.00`, sum `7.00`. The report sum is
  `37.00`. Each page header matches its page footer. The report
  sum differs from both page sums. The log is empty.
- Lua 5.5.1 loaded the new body. An empty source prints count
  `0.00` and a blank sum, with no warn. A string `"10"` on page
  1 stays out of that page sum (`10.00`) and out of the report
  sum (`17.00`). Page 2 stays `7.00`. Four warns name the report
  header, report footer, page header, and page footer sum items.
  A detail aggregate prints blank and warns once. A function
  name on a page footer prints blank and warns once. Grouped,
  totals, detail, page number, and static compose match the
  1458 body.
- `luacheck` on `acuranzo_1459.lua` reported 0 warnings. `npm
  test` was 93 files, 1472 passed, 12 skipped. The skipped file
  is the live auth check; the server check aborted. `npm run
  lint` and `npm run lint:css` exited 0. `npm run templates:copy`
  copied `reports.html`.
- Andrew said the latest migration applied is 1459, and that
  aggregate functions in page headers and footers work as
  indicated. He did not quote page 1 `30.00`, page 2 `7.00`,
  the report `37.00`, the counts, or the names Ann, Bo, and Cy.

### Lessons learned

- The page header is emitted before that page's rows are known.
  The aggregate text is filled after the page list exists, in
  the same pass that fills page number and page count. The page
  header and the page footer share one reduction.
- A detail row is recorded once on the page that printed it.
  Two detail bands for the same row on one page count as one
  row. The page window is those rows, not the group span.
- Sum still accepts only a finite Lua number. A bad value warns
  once per sum item. The same item on a later page does not add
  a second warn. A page with only numeric values does not warn
  for a bad value on another page.
- An empty page sum prints blank. An empty page count prints
  `0.00` under the money mask. A named function still prints
  blank until Phase 33.
- Andrew's close-out named file 1459. That file is the page-totals
  packet. He said aggregate functions in page headers and footers
  work as indicated. He did not quote the fixture figures. Those
  ops are field reductions. A named function still prints blank
  until Phase 33.

### Handoff

- Phase 39 is complete (2026-11-04). Andrew said the latest
  migration applied is 1459, and that aggregate functions in
  page headers and footers work as indicated. He did not quote
  the page sums, the counts, or the names. The packet is
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1459.lua).
  Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as
  written. Reverse of 1459 restores the 1458 body.
- Next free Acuranzo file is **1460**. QueryRef **175** stays
  unused. No new lookup. No new QueryRef.
- A named function still prints blank until Phase 33. His phrase
  "aggregate functions" is the field ops on a page header and a
  page footer.
- Phases 40, 41, and 42 are complete (2026-11-04). Andrew
  said they are fully operational and confirmed the values
  returned. He did not name `acuranzo_1460.lua`,
  `acuranzo_1461.lua`, or `acuranzo_1462.lua`, and he did not
  quote the fixture figures. Leave `acuranzo_1455.lua` through
  `acuranzo_1462.lua` as written. The next free Acuranzo file
  at that close was **1463**. Phases 43 and 44 are complete
  (2026-11-04). Phases 44a and 44b are written below and have
  not started.

---

## Phase 40 — Average and weighted

### Goal

`avg`, `mean`, and `weighted` reduce a numeric field.

### Entry gate

Phase 39 Status complete.

### Work items

- [x] 40.1 `avg` and `mean` are the same arithmetic mean. `weighted`
       stores `weight` as a field and computes field times weight,
       divided by the sum of the weights. A non-numeric value warns
       and is skipped. A zero sum of weights, or no numeric pair
       left, prints an empty string and adds one warn entry. An
       empty result is an empty string.
- [x] 40.2 Add the three ops to the picker Phase 38 built. Fixture:
       `avg` and `mean` match, and `weighted` matches its inputs.
       Hand the amendment over. The preview shows the mean.

### Done means

The fixture shows `avg` equal to `mean`, and the weighted total
matches its inputs.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said Phases 40, 41, and 42
are fully operational and confirmed the values returned. He
did not name this file and he did not quote Avg `20.00`, Mean
`20.00`, or Weighted `24.00`. The packet is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua).
No new QueryRef. No new lookup. The next free Acuranzo file
is **1463**. QueryRef **175** stays unused.

### Accomplished

- Andrew named Phases 40, 41, and 42 after the Phase 40 peek.
  Phase 39 is complete. Phases 33–37 stay as they are. Phase 43
  has not started.
- The Aggregate Number picker offers `count`, `filled`,
  `unique`, `sum`, `avg`, `mean`, `weighted`, `median`, and
  `mode`. Weight is a client row, shown only while the op is
  `weighted`. A text function on `mode` uses lookup 053 key 34
  and drops the mask. A field-bound mode stays key 35. No new
  lookup. No new QueryRef.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua)
  copies the 1459 `Reports.Render` body. Reverse restores that
  body. Leave `acuranzo_1455.lua` through `acuranzo_1459.lua`
  as written. `avg` and `mean` are the arithmetic mean of the
  numeric values. `weighted` stores `weight` as a field name.
  A non-numeric value warns and is skipped. A missing weight,
  a zero sum of weights, or no numeric pair prints blank and
  warns once. An empty mean prints blank.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/mean.definition.json`](/elements/003-lithium/tests/fixtures/reporting/mean.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/mean.sources.json`](/elements/003-lithium/tests/fixtures/reporting/mean.sources.json).
  The definition stores the rows, so an import sends them in
  `sources`. The rows are amount/weight `10/1`, `20/1`, and
  `30/3`. Mask `#,##0.00`. Avg and mean are `20.00`. Weighted
  is `24.00`. The report header matches the report footer. The
  log is empty.
- Lua 5.5.1 loaded the new body. A string `"10"` stays out of
  avg and mean. An empty mean prints blank and an empty count
  prints `0.00`. Zero weights print blank and warn once. A
  missing weight prints blank and warns once. Two weighted
  items on `amount` with different weight fields print `15.00`
  and `12.50`. A null weight does not warn by itself. A
  function name prints blank. A detail aggregate prints blank.
  Grouped, totals, page totals, detail, page number, and static
  compose match the 1459 body.
- `luacheck` on `acuranzo_1460.lua` reported 0 warnings. `npm
  test` was 93 files, 1476 passed, 12 skipped. The skipped
  file is the live auth check. The server check aborted.
  `npm run lint` and `npm run lint:css` exited 0. `npm run
  templates:copy` copied `reports.html` (139 templates).
- Andrew said the three phases are fully operational and
  confirmed the values returned. He did not name
  `acuranzo_1460.lua`. He did not quote the fixture figures.

### Lessons learned

- `avg` and `mean` are two stored ops. The arithmetic mean
  matches. The memo key keeps the two results apart.
- `weight` is part of the memo key, so two weighted items on
  the same value field stay distinct. `ctx.aggregates.weighted`
  still keys the last result for that field. The printed item
  uses the weight-qualified key.
- An empty mean prints blank. An empty count stays `0`. A
  string is not converted into the total. The number mask may
  still read a string when it formats a field.
- The picker lists `median` and `mode` in this same sitting.
  Until `acuranzo_1461.lua` and `acuranzo_1462.lua` are
  applied, those ops warn and print blank.
- A named function still prints blank until Phase 33.
- His close named the set, not the file. That check is the
  apply report for `acuranzo_1460.lua` through
  `acuranzo_1462.lua`. The fixture figures stayed unquoted.

### Handoff

- Phase 40 is complete (2026-11-04). Andrew said Phases 40,
  41, and 42 are fully operational and confirmed the values
  returned. He did not name
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1460.lua)
  or quote Avg `20.00`, Mean `20.00`, and Weighted `24.00`.
  Leave `acuranzo_1455.lua` through `acuranzo_1462.lua` as
  written. Reverse of 1460 restores the 1459 body.
- Next free Acuranzo file is **1463**. QueryRef **175** stays
  unused. No new lookup. No new QueryRef.
- A named function still prints blank until Phase 33.
- Phase 43 (Min and max) was previewed and not started. Do
  not start it in this conversation.

---

## Phase 41 — Median

### Goal

`median` reduces a numeric field. An even count uses the mean of the
two central numbers.

### Entry gate

Phase 40 Status complete.

### Work items

- [x] 41.1 Implement `median` on the raw values. An even count is the
       mean of the two central numbers. An empty result is an empty
       string.
- [x] 41.2 Add `median` to the picker. One fixture has an odd count
       and one has an even count. Hand the amendment over. The
       preview shows the odd-count median.

### Done means

The odd-count median and the even-count median both match, and the
preview shows the odd-count median.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said Phases 40, 41, and 42
are fully operational and confirmed the values returned. He
did not name this file and he did not quote Odd `20.00` or
Even `25.00`. On his eight amounts the two central values are
42.00 and 89.99, and the median is 65.995. A `#,##0.00` mask
prints 66.00. The packet is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1461.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1461.lua).
Reverse restores the 1460 body. No new QueryRef. No new lookup.

### Accomplished

- `median` sorts the numeric values. An odd count returns the
  middle value. An even count returns the mean of the two
  central numbers. A non-numeric value warns and is skipped.
  An empty median prints blank.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/median.definition.json`](/elements/003-lithium/tests/fixtures/reporting/median.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/median.sources.json`](/elements/003-lithium/tests/fixtures/reporting/median.sources.json).
  The Odd group is `10`, `30`, `20`. Its median is `20.00`.
  The Even group is `10`, `40`. Its median is `25.00`. The
  group header matches the group footer. There is no report
  median. Mask `#,##0.00`. The log is empty.
- Lua 5.5.1 loaded the 1461 body. The same fixture on the 1460
  body prints blank and warns that `median` does not reduce.
  `luacheck` on `acuranzo_1461.lua` reported 0 warnings.
- Andrew confirmed the median of his eight amounts. Sorted,
  the two central values are 42.00 and 89.99. Their mean is
  65.995. He did not quote Odd `20.00` or Even `25.00`.

### Lessons learned

- The odd-count preview and the even-count check share one
  definition. Compose does not sort the groups. Odd is first
  because its rows are first.
- Putting both groups into one report median would hide the
  even case. The group footer is the even check. The report
  has no median item.
- An even count is the mean of the two central numbers after
  the numeric sort, not the mean of every value.
- His eight amounts are an even count. The middle pair is
  42.00 and 89.99. The raw median is 65.995. Lua stores that
  mean as `65.995000000000005` because 89.99 is not an exact
  binary value. A blank mask prints `65.995`. A `#,##0.00`
  mask rounds half away from zero and prints `66.00`. He
  confirmed those figures. He did not quote the fixture.

### Handoff

- Phase 41 is complete (2026-11-04). Andrew said the three
  phases are fully operational and confirmed the median of
  his eight amounts: 65.995, and `66.00` under `#,##0.00`.
  He did not name
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1461.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1461.lua)
  or quote Odd `20.00` and Even `25.00`. Leave
  `acuranzo_1455.lua` through `acuranzo_1462.lua` as written.
  Reverse of 1461 restores the 1460 body.
- Next free Acuranzo file is **1463**. QueryRef **175** stays
  unused. No new lookup. No new QueryRef.
- A named function still prints blank until Phase 33.
- Phase 43 (Min and max) was previewed and not started. Do
  not start it in this conversation.

---

## Phase 42 — Mode

### Goal

`mode` reduces a field. A tie picks the smallest value under Lua `<`
and warns.

### Entry gate

Phase 41 Status complete.

### Work items

- [x] 42.1 Implement `mode`. A single mode returns that value. A tie
       returns the smallest value under Lua `<` and adds a warning.
       A text mode uses lookup 053 key 34.
- [x] 42.2 Add `mode` to the picker. One fixture has a single mode
       and one has a tie. Hand the amendment over. The preview shows
       the single mode.

### Done means

The single-mode fixture matches, the tie fixture returns the
smallest value plus a warning, and the preview shows the single mode.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said Phases 40, 41, and 42
are fully operational and confirmed the values returned. He
did not name this file and he did not quote Labels `A` or
Numbers `1.00`. On his eight amounts every value appears
once, so the tie prints -17.25. The packet is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1462.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1462.lua).
Reverse restores the 1461 body. No new QueryRef. No new
lookup. The next free Acuranzo file is **1463**. QueryRef
**175** stays unused.

### Accomplished

- `mode` returns the most common value. A tie returns the
  smallest value under Lua `<` and warns once. A string prints
  with no mask. A number uses the number mask. Text mode is
  lookup 053 key 34. Number mode is key 35. A value that is
  neither text nor a finite number warns and is skipped. An
  empty mode prints blank and does not warn.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/mode.definition.json`](/elements/003-lithium/tests/fixtures/reporting/mode.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/mode.sources.json`](/elements/003-lithium/tests/fixtures/reporting/mode.sources.json).
  The Labels group is `A`, `B`, `A`. Its mode is `A` on key 34,
  with no mask. The Numbers group is `2`, `1`, `2`, `1`. Its
  mode is `1.00` on key 35. One warn says the mode tied. There
  is no report mode.
- Lua 5.5.1 loaded the 1462 body. The number mode on the 1461
  body prints blank and warns that `mode` does not reduce.
  `luacheck` on `acuranzo_1462.lua` reported 0 warnings.
- Andrew confirmed the mode of his eight amounts. Each amount
  appears once. The tie prints -17.25. He did not quote
  Labels `A` or Numbers `1.00`.

### Lessons learned

- A field has no declared type, so a field-bound mode stays
  key 35. The script still prints a string result without the
  number mask. A text function is what switches the item to
  key 34 and deletes the mask.
- Mixed number and string values do not use Lua `<`. Numbers
  sort before strings. The fixtures are one type each, so the
  tie uses `<`.
- The tie warn is once per item for the whole compose. The
  Numbers footer is the only item that ties, so the log has
  one warning.
- Datetime is not a mode input in the designer. `min` and
  `max` wait for Phase 43.
- An all-unique list is a tie. Mode counts frequency. The
  tie-break is the smallest value, so his amounts print
  -17.25 and warn once. He confirmed that print. He did not
  quote the fixture.

### Handoff

- Phases 40, 41, and 42 are complete (2026-11-04). Andrew
  said they are fully operational and confirmed the values
  returned. He did not name
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1462.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1462.lua)
  or quote Labels `A` and Numbers `1.00`. On his eight
  amounts the mode tie printed -17.25. Leave
  `acuranzo_1455.lua` through `acuranzo_1462.lua` as written.
  Reverse of 1462 restores the 1461 body.
- Next free Acuranzo file at that close was **1463**. QueryRef
  **175** stays unused. No new lookup. No new QueryRef. A named
  function still prints blank until Phase 33.
- The next sitting named Phases 43 and 44 together. That waives
  Phase 44's written gate. The packets are
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua)
  and
  [`/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua).
  He applied them. Phases 43 and 44 are complete (2026-11-04).
  The next free file is **1465**. Phases 44a and 44b are written
  and have not started. Phase 45 has not started. Phases 33–37,
  45–48, PDFs, barcodes, and charts stay on the temporary skip.

---

## Phase 43 — Min and max

### Goal

`min` and `max` reduce a number or a datetime.

### Entry gate

Phase 42 Status complete.

### Work items

- [x] 43.1 `min` and `max` compare numbers as numbers and datetimes
       as one kind per total. The first non-null value picks the
       kind. A later value of the other kind warns once and is
       skipped. Nulls are skipped and do not warn. An empty result
       is an empty string. A datetime result uses lookup 053 key 38.
       A number uses key 35.
- [x] 43.2 Add the ops to the picker. The fixture records `min` and
       `max` for a number and for a datetime. Hand the amendment
       over. The preview shows both.

### Done means

The number fixture and the datetime fixture each show `min` and
`max`.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said Phases 43 and 44 look
good. He did not name this file and he did not quote min
`1.00`, max `3.00`, `2026-01-15`, or `2026-03-02`. The packet
is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua).
Reverse restores the 1462 body. No new QueryRef. No new
lookup. The next free Acuranzo file is **1465**. QueryRef
**175** stays unused.

### Accomplished

- Andrew named Phases 43 and 44 together. Phase 42 is complete.
  Naming both waives Phase 44's written gate. Phases 33–37 stay
  on the skip. Phase 45 has not started.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1463.lua)
  copies the 1462 `Reports.Render` body. `min` and `max` compare
  numbers as numbers and strings as written. The first finite
  number or string picks the kind. A later value of the other
  kind warns once and is skipped. Nulls do not warn. An empty
  result prints blank. A datetime item is typeKey 38. A text
  item is typeKey 34. A number item is typeKey 35. Reverse
  restores the 1462 body. No new lookup. No new QueryRef.
- The picker offers `min`, `max`, `first`, and `last` with the
  ops from Phases 40–42. A datetime function sets key 38 and
  clears a mask from another kind. A field has no type, so a
  field-bound item stays key 35 unless it is already key 34 or
  key 38. The datetime fixture stores key 38.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/minmax.definition.json`](/elements/003-lithium/tests/fixtures/reporting/minmax.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/minmax.sources.json`](/elements/003-lithium/tests/fixtures/reporting/minmax.sources.json).
  Number amounts `3`, null, `1`, `2` print min `1.00` and max
  `3.00` on key 35 with mask `#,##0.00`. Date values
  `2026-03-02`, `2026-01-15`, null, `2026-02-01` print min
  `2026-01-15` and max `2026-03-02` on key 38 with mask
  `yyyy-MM-dd`. The unused kind prints blank and does not warn.
  Header matches footer. The log is empty.
- Lua 5.5.1 loaded the 1463 body. On the 1462 body a key 35
  `min` or `max` warns that the op does not reduce. A key 38
  item on that body prints blank and does not warn. `luacheck`
  on `acuranzo_1463.lua` reported 0 warnings.

### Lessons learned

- A civil date and a text value are both strings. The comparison
  is Lua `<` on the stored string. The script does not parse the
  string into a date to compare it.
- The kind is string or finite number. A non-finite number, a
  boolean, or a table does not pick the kind. It warns once and
  is skipped. If every value is that kind of skip, the total
  prints blank and still warns.
- Datetime formatting on key 38 is limited to `min` and `max` in
  this file, and to `first` and `last` in the next file. A count
  stored on key 38 stays a number.
- The designer offers `first` and `last` in the same picker.
  Until `acuranzo_1464.lua` is applied, those ops warn and print
  blank.
- Andrew's close named the phases, not the files. He did not
  quote the fixture figures. That check is the apply report.
- The property row is labeled `Op` and the list is the lowercase
  codes in `AGGREGATE_OPS`. That list is not a lookup. Sorting
  lookup 054 does not reorder it. He asked about the label
  `Summarize`, a sort, and capital letters. The stored codes
  stay lowercase because the script matches them. The label and
  the display capitals were left as he previewed them.

### Handoff

- Phases 43 and 44 are complete (2026-11-04). Andrew said they
  look good. He did not name the files and he did not quote the
  fixture figures. Leave `acuranzo_1455.lua` through
  `acuranzo_1462.lua` as written. Reverse of 1463 restores the
  1462 body. Reverse of 1464 restores the 1463 body.
- Next free Acuranzo file is **1465**. QueryRef **175** stays
  unused. No new lookup. A named function still prints blank
  until Phase 33.
- Phases 44a and 44b are written below and have not started.
  The next conversation is Phase 44a. Do not start it here.
  Phase 45 has not started. Phases 33–37, 45–48, PDFs,
  barcodes, and charts stay on the temporary skip.

---

## Phase 44 — First and last

### Goal

`first` and `last` follow row order and skip nulls.

### Entry gate

Phase 43 Status complete. Andrew named Phases 43 and 44 together
on 2026-11-04, which waives this gate for this sitting.

### Work items

- [x] 44.1 `first` and `last` walk row order and skip nulls. `first`
       is not the minimum. A null in the middle is skipped. An empty
       result is an empty string. Text uses key 34. A number uses
       key 35. A datetime uses key 38.
- [x] 44.2 Add the ops to the picker. The fixture shows `first` and
       `last` for a number and for a datetime, with a null in the
       middle. Hand the amendment over.

### Done means

The fixture shows `first` and `last` in row order, and the null in
the middle is skipped.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**Complete** (2026-11-04). Andrew said Phases 43 and 44 look
good. He did not name this file and he did not quote first
`10.00`, last `7.00`, `2026-03-02`, or `2026-01-01`. The packet
is
[`/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua).
Reverse restores the 1463 body. No new QueryRef. No new
lookup. The next free Acuranzo file is **1465**. QueryRef
**175** stays unused.

### Accomplished

- Andrew named Phases 43 and 44 together, which waives the
  written gate on Phase 43 being complete. Phase 45 has not
  started.
- [`/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1464.lua)
  copies the 1463 `Reports.Render` body. `first` and `last`
  walk row order and skip nulls. `first` is not the minimum.
  A null in the middle is skipped. An empty result prints
  blank. Text uses key 34. A number uses key 35. A datetime
  uses key 38. Reverse restores the 1463 body. No new lookup.
  No new QueryRef.
- The fixture is
  [`/elements/003-lithium/tests/fixtures/reporting/firstlast.definition.json`](/elements/003-lithium/tests/fixtures/reporting/firstlast.definition.json)
  with
  [`/elements/003-lithium/tests/fixtures/reporting/firstlast.sources.json`](/elements/003-lithium/tests/fixtures/reporting/firstlast.sources.json).
  Number amounts `10`, null, `30`, `7` print first `10.00` and
  last `7.00`. Date values `2026-03-02`, null, `2026-01-01`
  print first `2026-03-02` and last `2026-01-01`. Header matches
  footer. The log is empty.
- Lua 5.5.1 loaded the 1464 body. On the 1463 body `first` and
  `last` warn that the op does not reduce and print blank.
  `luacheck` on `acuranzo_1464.lua` reported 0 warnings. The
  same probe kept the Phase 43 min and max figures, and the
  page, group, mean, median, and mode fixtures matched the
  1462 body.

### Lessons learned

- `first` keeps the first non-null value. A later smaller
  number does not replace it. `last` keeps the last non-null
  value. A null does not clear either one.
- The result is formatted from the item typeKey. A string on
  key 35 prints with no number mask. A datetime on key 38 uses
  the datetime mask.
- Apply 1463 before 1464. The 1464 reverse puts the 1463 body
  back, and `first` and `last` print blank again.
- Andrew said Phases 43 and 44 look good. He did not quote
  first `10.00` or last `7.00`. The row he was looking at is
  still labeled `Op`. Capitalizing that list is a display
  change. The file keeps `min` and `first`.

### Handoff

- Phases 43 and 44 are complete (2026-11-04). Andrew said they
  look good. The packets are `acuranzo_1463.lua` and
  `acuranzo_1464.lua`. Leave `acuranzo_1455.lua` through
  `acuranzo_1462.lua` as written. QueryRef **175** stays
  unused. No new lookup. The next free Acuranzo file is
  **1465**.
- Phases 44a and 44b are written below and have not started.
  The next conversation is Phase 44a. Do not start it here.
  Do not start Phase 45.

---

## Phase 44a — Static shapes

### Goal

A Static shape fills its box. The design surface and Preview HTML
draw it. The author can stack items on the band.

### Entry gate

Phase 44 Status complete.

### Work items

- [ ] 44a.1 Seed a lookup of shape kinds. Re-check the next free
       lookup id on disk that day. Lookup 077 is the highest
       reporting lookup as of this write-up. Built-in rows are
       `line`, `rectangle`, `roundrect`, and `ellipse`. Path rows
       are `x`, `trapezoid`, `star`, `pentagon`, and `hexagon`.
       `value_txt` is the label. `code` is the token the item
       stores. A path row's collection is `{ "closed": true,
       "commands": [["M", 0, 0], ["L", 1, 0], ["C", ...], ["Z"]] }`
       with coordinates from 0 to 1. Commands are `M`, `L`, `C`,
       and `Z` only. `x` is two open strokes, corner to corner.
       The polygons point up. The trapezoid's short edge is on
       top. A user-added row uses the same collection. A bad
       command warns and skips that shape. Lookup 053 key 25
       stays Static Shape. Keys 26, 33, 39, and 53 stay hidden.
       No new QueryRef.
- [ ] 44a.2 The item is typeKey 25. It stores `kind`, and for a
       line `side` (`top`, `right`, `bottom`, or `left`). A
       rounded rectangle stores `corners.topLeft`,
       `corners.topRight`, `corners.bottomRight`, and
       `corners.bottomLeft` as lengths. `strokeWidth` is a
       length, default `1 pt`. `strokeStyle` is `solid`,
       `dashed`, `dotted`, or `none`. `strokeColor` is hex.
       `fillStyle` is `none` or `solid`. A line defaults to
       `none`. The other kinds default to `solid`. `fillPaint`
       is `{ "kind": "solid", "color": "#d3d3d3" }` even when
       the style is `none`. Phase 44b adds the other styles and
       does not rename this field. `rotation` is degrees, default `0`,
       clockwise on the page, about the center of the box.
       Lookup 054 key 14 is that rotation. New attribute keys
       start after key 40. Re-check the next free key on disk.
       A new shape is a rectangle, rotation 0, stroke `1 pt`
       solid `#000000`, fill `#d3d3d3`. A new rounded rectangle
       uses `0.125 in` on every corner. A new line uses the
       bottom edge and fill `none`. The stroke is centered on
       the path. One Shape control lists the lookup. The
       palette's Static group shows it.
- [ ] 44a.3 The design surface and Preview HTML draw an SVG from
       those commands. The side is applied, then rotation.
       Rounded corners and the ellipse are cubics. A corner is
       clamped so it cannot swallow the next corner. Fill `none`
       draws no fill. The preview sanitizer allows the shape
       tags this phase generates. The item is selected by its
       box. PDF does not draw the shape. The commands stay
       usable by a later PDF path. Do not rasterize the shape.
- [ ] 44a.4 Raise moves the item one step later in `band.items`.
       Lower moves it one step earlier. The outline stays in
       array order, so the top row is the back-most item and a
       raised row moves down. The icons are neither the band-add
       arrows nor the outline Prev/Next angles. A drag in the
       outline reorders within that band. There is no `order`
       property. Later items paint over earlier ones.
- [ ] 44a.5 Fixture: a rectangle, a rounded rectangle with unequal
       corners, a bottom line, a point-up hexagon rotated 30
       degrees, and two overlapping shapes with the later one on
       top. Hand the amendment over. The preview shows those
       shapes.

### Done means

The preview shows the rectangle, the rounded rectangle, the line,
the rotated hexagon, and the later shape on top.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 44b — Shape fill and gradient

### Goal

A shape's fill style and its paint are separate. The paint can be
a gradient. Preview HTML draws both.

### Entry gate

Phase 44a Status complete.

### Work items

- [ ] 44b.1 `fillStyle` is `none`, `solid`, `grid`, `dots`,
       `hatched`, or `noise`. Grid and dots use the report grid
       spacing. Hatch is one diagonal, down to the right. Noise
       is a dot field seeded from the item id, the same on every
       run. An SVG turbulence filter is not used. A line ignores
       fill. The style and the paint are separate, so a grid can
       use a gradient.
- [ ] 44b.2 `fillPaint.kind` is `solid`, `linear`, or `radial`.
       Stops are `{ "at", "color" }`, at least two, with `at`
       from 0 to 1. Linear `angle` `0` runs left to right, and
       the angle grows clockwise. Radial runs from the center of
       the box to the farthest corner. Colors are hex.
- [ ] 44b.3 Preview HTML draws the pattern and the gradient from
       the same commands as Phase 44a. PDF still does not draw
       the shape.
- [ ] 44b.4 Fixture: a hatched rectangle with a two-stop linear
       gradient, and a noise ellipse whose second run matches
       the first. Hand the amendment over. The preview shows both.

### Done means

The preview shows the hatched gradient and the same noise field
on a second run.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 45 — Group-key function

### Goal

A group band can take its key from a text or number function.

### Entry gate

Phase 44 Status complete. Phases 44a and 44b sit ahead of this
phase. Andrew named Phase 44a next. Do not start Phase 45 while
44a or 44b is open.

### Work items

- [ ] 45.1 A group band may set `groupKey` to a text or number
       function. A text key compares as a string. A number key
       compares as a number. `sort` defaults to true and uses Lua
       `<`. `sort` false breaks only when consecutive keys differ.
       One level. The one-field group remains the path with no Lua.
- [ ] 45.2 The group band's key picker lists text and number
       functions. Edit focuses the same Functions buffer.
- [ ] 45.3 Fixture: group by the pack-code month letter. Hand the
       amendment over. Browser: the preview shows that split.

### Done means

The fixture intermediate shows the Lua group split.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 46 — Page-break predicate

### Goal

A boolean function can start a new page before a detail row.

### Entry gate

Phase 45 Status complete.

### Work items

- [ ] 46.1 A detail band may set `pageBreak` to a boolean function.
       A true result starts a new page before that row. Any other
       value warns once and counts as false. Fixed-height paging
       still applies.
- [ ] 46.2 The detail band's picker lists boolean functions.
- [ ] 46.3 Fixture: start a new page when `ctx.row.break_before` is
       true. Hand the amendment over. Browser: the preview shows the
       extra page.

### Done means

The fixture intermediate shows the forced page.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 47 — Line-break predicate

### Goal

A boolean function can insert one blank detail-height before a row.

### Entry gate

Phase 46 Status complete.

### Work items

- [ ] 47.1 A detail band may set `lineBreak` to a boolean function.
       A true result inserts one blank detail-height before that
       row. Any other value warns once and counts as false. A
       newline inside a calculated value is kept by HTML
       (`white-space: pre-line`) and by the PDF text drawer. The
       stored band height does not change. Overflow clips.
- [ ] 47.2 The detail band's picker lists boolean functions.
- [ ] 47.3 Fixture: insert a blank line when `ctx.row.blank_before`
       is true. Hand the amendment over. Browser: the preview shows
       the blank line.

### Done means

The fixture intermediate shows the blank line.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 48 — Header and footer context

### Goal

A calculated item on a header or footer sees that band's context.

### Entry gate

Phase 47 Status complete. Header and footer formulas can read
`ctx.aggregates`. This phase's fixture does not require an aggregate.

### Work items

- [ ] 48.1 Calculated items on a group, page, or report header or
       footer receive the context in the User Lua table. On a group
       band, `ctx.row` is the first row of the group and `ctx.rows`
       is the whole group. Every `ctx` includes `parameters`.
       `page.count` is filled after the page list exists, before the
       page formulas run.
- [ ] 48.2 Fixture: a group footer formula reads `ctx.group.key` and
       prints it. Hand the amendment over.

### Done means

The group-footer formula prints `ctx.group.key`.

### Exit gate

Script packet apply and the fixture call.

### Status

**not started**

---

## Phase 49 — CSV emit

### Goal

`stage=csv` returns one row per detail record, as the text compose
already stored.

### Entry gate

Phase 48 Status complete. The mask table is already applied.

### Work items

- [ ] 49.1 Amend `Reports.Render` with `stage=csv`. Columns are the
       field items on the detail band, in item order. Header and
       footer bands are not rows. Every value exports as the
       formatted text compose stored.
- [ ] 49.2 Grouped reports export flat rows. The group key is
       repeated as leading columns. Aggregates are not CSV rows.
       Write that shape into the contract note.
- [ ] 49.3 Values are escaped per RFC 4180. A newline inside a value
       is quoted, not split.
- [ ] 49.4 Add the expected CSV for the detail fixture. Hand the
       packet over. Andrew's call matches.

### Done means

The detail fixture renders to the expected CSV.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

