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

**Entry:** Phase 32 in [`/docs/Li/plans/reporting/04-pdf.md`](/docs/Li/plans/reporting/04-pdf.md) is complete.
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

Phase 37 Status complete.

### Work items

- [ ] 38.1 An aggregate item stores `op` and either `field` or
       `function`. This phase's ops are `count`, `filled`, `unique`,
       and `sum`. Numeric results use lookup 053 key 35. A text
       result uses key 34. The picker offers a function only when
       its `returns` allows the op. The fixture uses a field.
- [ ] 38.2 Compose reduces the raw values for the current group and
       for the whole detail source, and places them on
       `ctx.aggregates`. `count` counts every row. `filled`,
       `unique`, and `sum` skip nulls. An empty `count`, `filled`,
       or `unique` is 0. An empty `sum` is an empty string. A
       non-numeric value in `sum` warns and is skipped. The header
       and the footer of one scope show the same total. The item
       mask formats the reduction.
- [ ] 38.3 The designer places the item on a group header, a group
       footer, a report header, or a report footer.
- [ ] 38.4 Fixture: two groups. Hand the amendment over. The preview
       shows each op this phase names, and the header matches the
       footer.

### Done means

The two-group fixture shows count, filled, unique, and sum, and the
header matches the footer.

### Exit gate

Lint, the script packet apply, the fixture call, and the browser
preview.

### Status

**not started**

---

## Phase 39 — Page totals

### Goal

A page header or page footer shows the sum of the detail rows on
that page.

### Entry gate

Phase 38 and Phase 21 Status complete.

### Work items

- [ ] 39.1 A page-footer aggregate reduces the detail rows placed on
       that page. The same window serves later ops. The page header
       and the page footer show the same page total.
- [ ] 39.2 The designer can place the aggregate on a page header or
       a page footer.
- [ ] 39.3 Fixture: the page sum sits beside the report sum, and the
       two differ when the rows span two pages. Hand the amendment
       over.

### Done means

The fixture shows a page sum and a report sum, and they differ across
a two-page report.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 40 — Average and weighted

### Goal

`avg`, `mean`, and `weighted` reduce a numeric field.

### Entry gate

Phase 39 Status complete.

### Work items

- [ ] 40.1 `avg` and `mean` are the same arithmetic mean. `weighted`
       stores `weight` as a field and computes field times weight,
       divided by the sum of the weights. A non-numeric value warns
       and is skipped. A zero sum of weights, or no numeric pair
       left, prints an empty string and adds one warn entry. An
       empty result is an empty string.
- [ ] 40.2 Add the three ops to the picker Phase 38 built. Fixture:
       `avg` and `mean` match, and `weighted` matches its inputs.
       Hand the amendment over. The preview shows the mean.

### Done means

The fixture shows `avg` equal to `mean`, and the weighted total
matches its inputs.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 41 — Median

### Goal

`median` reduces a numeric field. An even count uses the mean of the
two central numbers.

### Entry gate

Phase 40 Status complete.

### Work items

- [ ] 41.1 Implement `median` on the raw values. An even count is the
       mean of the two central numbers. An empty result is an empty
       string.
- [ ] 41.2 Add `median` to the picker. One fixture has an odd count
       and one has an even count. Hand the amendment over. The
       preview shows the odd-count median.

### Done means

The odd-count median and the even-count median both match, and the
preview shows the odd-count median.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 42 — Mode

### Goal

`mode` reduces a field. A tie picks the smallest value under Lua `<`
and warns.

### Entry gate

Phase 41 Status complete.

### Work items

- [ ] 42.1 Implement `mode`. A single mode returns that value. A tie
       returns the smallest value under Lua `<` and adds a warning.
       A text mode uses lookup 053 key 34.
- [ ] 42.2 Add `mode` to the picker. One fixture has a single mode
       and one has a tie. Hand the amendment over. The preview shows
       the single mode.

### Done means

The single-mode fixture matches, the tie fixture returns the
smallest value plus a warning, and the preview shows the single mode.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 43 — Min and max

### Goal

`min` and `max` reduce a number or a datetime.

### Entry gate

Phase 42 Status complete.

### Work items

- [ ] 43.1 `min` and `max` compare numbers as numbers and datetimes
       as one kind per total. The first non-null value picks the
       kind. A later value of the other kind warns once and is
       skipped. Nulls are skipped and do not warn. An empty result
       is an empty string. A datetime result uses lookup 053 key 38.
       A number uses key 35.
- [ ] 43.2 Add the ops to the picker. The fixture records `min` and
       `max` for a number and for a datetime. Hand the amendment
       over. The preview shows both.

### Done means

The number fixture and the datetime fixture each show `min` and
`max`.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 44 — First and last

### Goal

`first` and `last` follow row order and skip nulls.

### Entry gate

Phase 43 Status complete.

### Work items

- [ ] 44.1 `first` and `last` walk row order and skip nulls. `first`
       is not the minimum. A null in the middle is skipped. An empty
       result is an empty string. Text uses key 34. A number uses
       key 35. A datetime uses key 38.
- [ ] 44.2 Add the ops to the picker. The fixture shows `first` and
       `last` for a number and for a datetime, with a null in the
       middle. Hand the amendment over.

### Done means

The fixture shows `first` and `last` in row order, and the null in
the middle is skipped.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 45 — Group-key function

### Goal

A group band can take its key from a text or number function.

### Entry gate

Phase 44 Status complete.

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

