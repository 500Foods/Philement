<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting Plan

**Date:** 2026-10-10
**Revised:** 2026-11-04
**Status:** Phase 0 complete. Approved 2026-10-10. Phases 1–7
complete. Phases 6 and 7 closed 2026-10-31. Andrew confirmed
drag, Undo, Redo, export, import, and the dots on the alternate
database. Phase 8 is complete (2026-10-31). Andrew
accepted the rulers, the corner cycle, and the unit name.
The 2026-11-01 sidequest draws each band once, at the content
width. The top ruler sits above the first band's descriptor and
starts at 0. Each left ruler starts at 0 at that band's body.
Snap starts on. Paper height and margin guides stay off the
surface. Phase 9 is complete
(2026-10-31). Andrew said the view scale works pretty
well. Phase 10 is complete (2026-10-31). Andrew said
it works pretty well. Phase 10-UI is complete
(2026-10-31). Andrew said the changes are working. The
shell has the report list, Report, Editor, JSON, and
Revisions pages, and the three toolbars. The wheel zooms
with Ctrl or Cmd. Phase 17 is complete (2026-10-31).
Andrew reported QueryRefs 165–169 applied. Phase 18 is
complete (2026-10-31). Andrew said the changes are
working. The manager calls 165, 166, 167, and 168. Andrew reported Delete Report applied. The list calls QueryRef 170. Category is a reports row column. The Revisions page lists saved revisions through QueryRef 171, copies one onto a new report, and reverts through QueryRef 172. acuranzo_1433.lua stores the signed-in account on insert 168. acuranzo_1428.lua through acuranzo_1435.lua are applied on Andrew's 2026-10-31 check. The next Acuranzo file is 1436. The next caller-facing QueryRef is 173.
Phase 11 is complete (2026-11-02). Andrew reported acuranzo_1436.lua applied, confirmed lookup 053, and Tests 31 and 98 pass. The next caller-facing QueryRef is 173. Phase 12 is complete (2026-11-02): Andrew reported acuranzo_1437.lua applied and confirmed lookup 054. Phase 13 is complete (2026-11-02): Andrew reported acuranzo_1438.lua applied and confirmed lookup 057. Phase 13b is complete (2026-11-02): Andrew reported acuranzo_1439.lua (lookup 074), acuranzo_1440.lua (lookup 075), and acuranzo_1441.lua (054 `section` key) applied. The next Acuranzo file is 1442. The next caller-facing QueryRef is 173. Phase 14 (property panel) is complete (2026-11-03). Phase 14b (property panel preferences) is complete (2026-11-03): Andrew said "Pretty good. Let's close this phase." The next phase starts when Andrew names it. Phase 15 (field items and a sample datasource) is complete (2026-11-03): Andrew said "Everything looks good." acuranzo_1443.lua is applied; the next Acuranzo file is 1444.
Phase 16 (one group level) is complete (2026-11-03): Andrew said "Seems to be working pretty well." Phases 19, 19b, 20 and 28 are complete (2026-11-03); 28 ran ahead of 21 on Andrew's waiver. Phase 21 (Paging) is complete (2026-11-03). Acuranzo 1444–1450 are applied. Phases 22, 22a and 22b are complete (2026-11-03): Andrew applied acuranzo_1451–1453 and confirmed User time, UTC time, mask presets, and System User. Phase 23 (Page number and page count) is complete (2026-11-04): Andrew applied acuranzo_1454 and confirmed Page Number, Page Count and Page Label. **Phase 24 (HTML emit)** is complete (2026-11-04). Andrew confirmed the rendered Preview HTML page and the Editor-matching chrome. `acuranzo_1455.lua` is the applied script. `stage=html` returns `{ html, log }`. The Lua 5.5.1 probe matched `detail.html`. He did not send a separate byte dump. Preview HTML's Run button calls that stage and shows the sanitized page in a sandboxed iframe. Font settings stay off that tab. The next free Acuranzo file is **1456**. The next caller-facing QueryRef is **175**. No new lookup. Phase 25 (Preview) is complete (2026-11-04). Andrew declined auto-render. Run is the only trigger. He previewed two detail rows and the second band rendered. Phase 26 (Generation log) is complete (2026-11-04). Andrew said "Ok, I think that is working." The Log tab is aligned text in CodeMirror, Vanadium Mono, filled by the last Run. He did not quote the warn line, the warn count, or a clean-detail zero dump. Phase 27 (Over-cap preview) has not started. Phase 18's body is in
[`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md).
Phase 10-UI's body stays in
[`/docs/Li/plans/reporting/01-designer.md`](/docs/Li/plans/reporting/01-designer.md).
The contract note is
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).
**Home:** Manager **24**, Report Manager, already registered.
**Design:** Acuranzo (shared product database). Lookups 0–199. Migrations
1000–1999.

Lithium is the designer. It writes a versioned JSON definition. The
Helium script `Reports.Render` reads that definition plus zero or more
JSON datasources, builds an intermediate document, and emits a format.
PDF is the default stage. The build order is this index. Phase bodies
for Phases 1–71 live in [`/docs/Li/plans/reporting/`](/docs/Li/plans/reporting/).
This file keeps the locks, the phase index, Phase 0, the backlog, and
the Working Log. This plan does not move Course Manager or Scripting
invoke.
CATCHUP Phase 1 (2026-10-16) already shipped the lifecycle fix.

Argent Phase 15 "Report queries" in
[`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md) is a set of
bookkeeping QueryRefs. It is a different piece of work.

## Phases

One phase per conversation, top to bottom. A phase whose entry gate
says earlier phases may still be open follows that gate. Every other
phase waits until the previous phase's Exit gate is green. A packet
phase ends when the packet is handed over, and it is complete when
Andrew reports the apply.

| Phase | What | Kind | Effort | Status |
| --- | --- | --- | --- | --- |
| 0 | Design lock | Decision | Easy | Complete |
| 1 | Contracts and fixtures | Docs + fixtures | Medium | Complete |
| 2 | Units and geometry | JS + Vitest | Easy | Complete |
| 3 | Document model and undo | JS + Vitest | Medium | Complete |
| 4 | Manager shell, import, export | Lithium UI | Medium | Complete |
| 5 | Page surface | Lithium UI | Easy | Complete |
| 6 | Bands and resize | Lithium UI | Medium | Complete |
| 7 | Section grid | Lithium UI | Easy | Complete |
| 8 | Rulers and the mouse line | Lithium UI | Medium | Complete |
| 9 | View scale | Lithium UI | Easy | Complete |
| 10 | Static text | Lithium UI | Medium | Complete |
| 10-UI | Manager chrome | Lithium UI | Medium | Complete |
| 11 | Lookup 053 repair | Helium, human apply | Medium | Complete |
| 12 | Lookup 054 fonts | Helium, human apply | Easy | Complete |
| 13 | Lookup 057 page sizes | Helium, human apply | Easy | Complete |
| 13b | Section lookups | Helium, human apply | Easy | Complete |
| 14 | Property panel | Lithium UI | Medium | Complete |
| 14b | Property panel preferences | Lithium UI | Medium | Complete |
| 15 | Field items and a sample datasource | Lithium UI | Easy | Complete |
| 16 | One group level | Lithium UI | Easy | Complete |
| 17 | Save and load packet | Helium, human apply | Medium | Complete |
| 18 | Revisions in the manager | Lithium UI | Medium | Complete |
| 19 | Compose skeleton | Helium seed, human apply | Medium | Complete |
| 19b | Datasources editor and embedded JSON | Lithium UI + Helium lookup | Medium | Complete |
| 20 | Value format | Helium amendment | Medium | Complete |
| 21 | Paging | Helium amendment | Easy | Complete |
| 22 | System datetime and system user (Timezone User/UTC/Explicit) | Lithium UI + Lua | Medium | Complete |
| 22a | Seed luatz (host tzdata) | Helium library pack | Easy | Complete |
| 22b | Luxon masks and format presets Lookup | Helium amendment + Lithium UI | Medium | Complete |
| 23 | Page number and page count | Lithium UI + Lua | Medium | Complete |
| 24 | HTML emit | Helium amendment | Easy | Complete |
| 25 | Preview | Lithium UI | Medium | Complete |
| 26 | Generation log | Lithium UI | Medium | Complete |
| 27 | Over-cap preview | Lithium UI | Easy | Not started |
| 28 | Query designer | Lithium UI | Medium | Complete |
| 29 | Query execution | Helium amendment | Medium | Not started |
| 30 | Seed lua-pdfkit | Helium library pack | Easy | Not started |
| 31 | PDF text | Helium amendment | Medium | Not started |
| 32 | Result cap | Helium amendment | Easy | Not started |
| 33 | Text formula | Helium amendment | Medium | Not started |
| 34 | Formula editor | Lithium UI | Medium | Not started |
| 35 | Formula check | Lithium UI + Lua | Easy | Not started |
| 36 | Number and datetime formula | Helium amendment | Medium | Not started |
| 37 | Formula runtime errors | Helium amendment | Easy | Not started |
| 38 | Count, unique, and sum | Helium + designer | Medium | Not started |
| 39 | Page totals | Helium + designer | Medium | Not started |
| 40 | Average and weighted | Helium amendment | Medium | Not started |
| 41 | Median | Helium amendment | Medium | Not started |
| 42 | Mode | Helium amendment | Medium | Not started |
| 43 | Min and max | Helium amendment | Medium | Not started |
| 44 | First and last | Helium amendment | Medium | Not started |
| 45 | Group-key function | Helium + designer | Medium | Not started |
| 46 | Page-break predicate | Helium + designer | Easy | Not started |
| 47 | Line-break predicate | Helium + designer | Easy | Not started |
| 48 | Header and footer context | Helium amendment | Medium | Not started |
| 49 | CSV emit | Helium amendment | Easy | Not started |
| 50 | Embedded image | Lithium UI + Lua | Medium | Not started |
| 51 | Reference and URL images | Helium amendment | Medium | Not started |
| 52 | PDF image stored once | Helium amendment | Easy | Not started |
| 53 | Seed lua-zint | Helium library pack | Easy | Not started |
| 54 | Barcode SVG | Lithium UI + Lua | Medium | Not started |
| 55 | Barcode PDF raster | Lithium UI | Easy | Not started |
| 56 | Seed d3.lua | Helium library pack | Easy | Not started |
| 57 | Chart design lock | Decision | Easy | Not started |
| 58 | Charts in the designer | Lithium UI | Medium | Not started |
| 59 | Bar chart | Helium amendment | Medium | Not started |
| 60 | Group and record chart scopes | Helium amendment | Medium | Not started |
| 61 | Line charts | Helium amendment | Medium | Not started |
| 62 | Combo chart | Helium amendment | Medium | Not started |
| 63 | Pie chart | Helium amendment | Medium | Not started |
| 64 | Custom chart | Helium amendment | Medium | Not started |
| 65 | Chart PDF raster | Lithium UI | Easy | Not started |
| 66 | SVG emit | Helium amendment | Easy | Not started |
| 67 | PNG export | Lithium UI | Easy | Not started |
| 68 | Native PDF contract | Decision | Easy | Not started |
| 69 | Zint PDF payload | Port repository | Medium | Not started |
| 70 | D3 PDF payload | Port repository | Medium | Not started |
| 71 | Place native PDF objects | Helium amendment | Medium | Not started |

Seventy-two phases, numbered 0 through 71. Each one is Easy or
Medium, and each one closes on a single behavior. Easy is a
fixture, a probe, or a decision. Medium adds a designer surface
or a browser pass. A sitting that ends early marks that phase in
progress and keeps its number. Phase 10-UI is a labeled
sidequest between 10 and 11. Phases 13b and 14b are labeled
phases added 2026-11-02 for the property panel. None of the
three is an extra number in that count.

A phase that hands a packet to a human is one sitting of agent work
plus the wait for the apply. Its Status block records the wait.

What a finished phase lets an author do. Later phases add to this.
They do not replace it.

| After | The author can |
| --- | --- |
| 10 | Place static text and export the definition file. |
| 18 | Save a revision, open it again, and list older revisions. |
| 25 | Preview the HTML export. |
| 26 | Read the generation log and its three counts. |
| 29 | Run a QueryRef from a stored report with no sources array. |
| 31 | Download a text PDF. |
| 35 | See a syntax error in the formula editor. |
| 36 | See a pack-date code and a masked number. |
| 44 | Put the built-in totals on a footer. |
| 49 | Download CSV. |
| 54 | See a barcode in the preview. A PDF barcode is a raster until Phase 71. |
| 71 | Get charts and barcodes as vectors in the PDF. |

## Milestones

Phase numbers stay 0 through 71. A sitting opens this file and the one
milestone file that holds the current phase.

| File | Phases | Stop when |
| --- | --- | --- |
| This file | 0 | The locks are approved. |
| [`01-designer.md`](/docs/Li/plans/reporting/01-designer.md) | 1–10, and 10-UI | Place static text and export the definition file. Phase 10-UI is the manager chrome (complete). |
| [`02-save.md`](/docs/Li/plans/reporting/02-save.md) | 11–18 | Save a revision and open it again. |
| [`03-preview.md`](/docs/Li/plans/reporting/03-preview.md) | 19–27 | Preview the HTML export and read the generation log. |
| [`04-pdf.md`](/docs/Li/plans/reporting/04-pdf.md) | 28–32 | Run a QueryRef and download a text PDF. |
| [`05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md) | 33–49 | Use formulas, totals, and breaks, and download CSV. |
| [`06-graphics.md`](/docs/Li/plans/reporting/06-graphics.md) | 50–67 | Place images, barcodes, and charts, and export SVG and PNG. |
| [`07-native-pdf.md`](/docs/Li/plans/reporting/07-native-pdf.md) | 68–71 | Get charts and barcodes as vectors in the PDF. |

**Resume:** Phases 1–7 are complete. Phases 6 and 7 closed
2026-10-31. Andrew confirmed drag, Undo, Redo, export, import,
and the dots. Phase 8 is complete (2026-10-31). Andrew
accepted the rulers, the corner cycle, and the unit name.
The 2026-11-01 sidequest draws each band once, at the content
width. The top ruler sits above the first band's descriptor and
starts at 0. Each left ruler starts at 0 at that band's body.
Snap starts on. Paper height and margin guides stay off the
surface. Phase 9 is complete
(2026-10-31). Andrew said the view scale works pretty
well. Phase 10 is complete (2026-10-31). Andrew said
it works pretty well. Phase 10-UI is complete
(2026-10-31). Andrew said the changes are working. The
shell has the report list, Report, Editor, JSON, and
Revisions pages, and the three toolbars. The wheel zooms
with Ctrl or Cmd. Phase 17 is complete (2026-10-31).
Andrew reported QueryRefs 165–169 applied. Phase 18 is
complete (2026-10-31). Andrew said the changes are
working. The manager calls 165, 166, 167, and 168. Andrew reported Delete Report applied. The list calls QueryRef 170. Category is a reports row column. The Revisions page lists saved revisions through QueryRef 171, copies one onto a new report, and reverts through QueryRef 172. acuranzo_1433.lua stores the signed-in account on insert 168. acuranzo_1428.lua through acuranzo_1435.lua are applied on Andrew's 2026-10-31 check. The next Acuranzo file is 1436. The next caller-facing QueryRef is 173.
Phase 11 is complete (2026-11-02). Andrew reported acuranzo_1436.lua applied, confirmed lookup 053, and Tests 31 and 98 pass. The next caller-facing QueryRef is 173. Phase 12 is complete (2026-11-02): Andrew reported acuranzo_1437.lua applied and confirmed lookup 054. Phase 13 is complete (2026-11-02): Andrew reported acuranzo_1438.lua applied and confirmed lookup 057. Phase 13b is complete (2026-11-02): Andrew reported acuranzo_1439.lua (lookup 074), acuranzo_1440.lua (lookup 075), and acuranzo_1441.lua (054 `section` key) applied. The next Acuranzo file is 1442. The next caller-facing QueryRef is 173. Phase 14 (property panel) is complete (2026-11-03). Phase 14b (property panel preferences) is complete (2026-11-03): Andrew said "Pretty good. Let's close this phase." The next phase starts when Andrew names it. Phase 15 (field items and a sample datasource) is complete (2026-11-03): Andrew said "Everything looks good." acuranzo_1443.lua is applied; the next Acuranzo file is 1444.
Phase 16 (one group level) is complete (2026-11-03): Andrew said "Seems to be working pretty well." Phases 19, 19b, 20 and 28 are complete (2026-11-03); 28 ran ahead of 21 on Andrew's waiver. Phase 21 (Paging) is complete (2026-11-03). Acuranzo 1444–1450 are applied. Phases 22, 22a and 22b are complete (2026-11-03): Andrew applied acuranzo_1451–1453 and confirmed User time, UTC time, mask presets, and System User. Phase 23 (Page number and page count) is complete (2026-11-04): Andrew applied acuranzo_1454 and confirmed Page Number, Page Count and Page Label. **Phase 24 (HTML emit)** is complete (2026-11-04). Andrew confirmed the rendered Preview HTML page and the Editor-matching chrome. `acuranzo_1455.lua` is the applied script. `stage=html` returns `{ html, log }`. The Lua 5.5.1 probe matched `detail.html`. He did not send a separate byte dump. Preview HTML's Run button calls that stage and shows the sanitized page in a sandboxed iframe. Font settings stay off that tab. The next free Acuranzo file is **1456**. The next caller-facing QueryRef is **175**. No new lookup. Phase 25 (Preview) is complete (2026-11-04). Andrew declined auto-render. Run is the only trigger. He previewed two detail rows and the second band rendered. Phase 26 (Generation log) is complete (2026-11-04). Andrew said "Ok, I think that is working." The Log tab is aligned text in CodeMirror, Vanadium Mono, filled by the last Run. He did not quote the warn line, the warn count, or a clean-detail zero dump. Phase 27 (Over-cap preview) has not started. Phase 18's body is in
[`/docs/Li/plans/reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md).
Phase 10-UI's body stays in
[`/docs/Li/plans/reporting/01-designer.md`](/docs/Li/plans/reporting/01-designer.md).
The contract note is
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md). One inch is
96 CSS pixels. Open this file for a lock. Open the milestone file
for the phase body.

## How to use this document

- Work **one phase at a time**, top to bottom. The Sitting section
  is the operating model for this launch.
- Start in this file. Read Resume and the phase index, then open the
  milestone file that holds that phase.
- Write the phase close-out in the milestone file. Set the same Status
  in the phase index here. Append the Working Log in this file.
- This plan follows [`/docs/PLANNING.md`](/docs/PLANNING.md). The
  steps below match that file.
- A phase whose entry gate says earlier phases may still be open
  follows that gate. Every other phase waits until the previous
  Status is complete and its Exit gate is green.
- Each phase has one **Done means** line. That is the testable state.
- Mark a work item `[x]` only when that item's verification passed.
- Defer with `[~]` plus one line and the phase it moves to.
- Before marking a phase complete, write Status, Accomplished,
  Lessons learned, and Handoff on that phase in its milestone file,
  set the phase-index Status in this file, and append the Working Log
  in this file. Then stop. The next phase starts in a new conversation.
- A sitting that ends early sets Status to **in progress**, names
  the last finished work item, and does not mark the phase complete.
- A Helium phase ends when the packet is handed over. It is complete
  only after Andrew reports the apply. The agent does not run SchemaTool
  or SchemaHelper apply, and does not apply a migration.
- The locks and the phase body are the contract. The locks are in
  this file. The phase body is in the milestone file. A Working Log
  entry keeps the numbers and the decisions of the day it was written.

## Sitting

Andrew starts each phase in a new chat with `cdli && grok`. `cdli`
is `cd ${LITHIUM_ROOT}`, so the session loads
[`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).
That guide points here. Read Resume, then the milestone file it
names. Do that phase. Write the close-out. Stop.

Opening the chat is the approval for that one phase. Edit source
for it. Ask only when a lock names a behavior and leaves a stored
string, a column, the units, or an order unpinned. Ask again when
a requirement is ambiguous during the work. Leave a schema change,
a stored string, and a column name for Andrew to confirm.

**Tests.** When the phase changes a function a test can call, add
or extend a Vitest file under
`elements/003-lithium/tests/unit/`. From `elements/003-lithium`,
run `npm test`. When JS or CSS changed, also run `npm run lint`
and `npm run lint:css`. When HTML changed, run
`npm run templates:copy`. Phase 1 writes the contract note and the
fixtures. Its gate is Andrew reading the note, with `npm test`
still passing. A work item has to say so before a Hydrogen
`test_NN` script, a new npm dependency, or a new manager id is
added.

**500 Courses.** Andrew deploys and checks the running app at
`https://lithium.500courses.com`. The agent does not deploy, does
not log in, and does not drive a browser for that check. When a
phase's exit gate names a browser check, run the unit tests, write
the close-out, and set Status to **in progress**. Handoff names the
check. He reports it in the next session. That session records the
result. When the check passed, mark the phase complete and stop,
unless the same message also names the next phase. A failed check
stays on that phase. Opening a chat by naming the next phase means
he has accepted the previous gate.

**Migrations.** New report migrations are Acuranzo files
(1000–1999), lookup ids 0–199, and low-series caller QueryRefs.
Prepare the packet and hand it over. Andrew applies it. The agent
does not run SchemaTool or SchemaHelper. A Helium phase stays
**in progress** until he reports the apply. Re-check the next free
file, QueryRef, and lookup id on disk the day the packet is
written. When the opening message reports the apply and names the
next phase, record the apply, mark the packet phase complete, and
do the named phase. When it only reports the apply, mark the phase
complete and stop.

**Sidequests.** A sidequest is its own chat. It stays unnumbered
unless Andrew names it. He named Phase 10-UI on 2026-10-31. That
label sits between 10 and 11 and does not change the count.
Record a sidequest in the Working Log in this file. When it
changes a lock, amend that lock here. When it blocks the current
phase, that phase stays **in progress** and the handoff names the
sidequest. Defer a work item with `[~]`, one line, and the phase
it moves to. Moving a row out of "Later, not scheduled" means
amending this file. Phase numbers stay 0 through 71.

**Close-out.** In the milestone file, on the phase, write Status,
Accomplished, Lessons learned, and Handoff. Handoff names what the
next phase re-checks, what is waiting on Andrew, and the next
milestone file when the phase span changes. Set the same Status in
the phase index here. Append the Working Log here. Then stop.

When the previous phase is still **in progress** and waiting on an
apply or a 500 Courses check, record that report first. Start the
next phase once he says the check passed, or once the same message
names that phase.

## Implementor workflow

Each phase is its own conversation. A new conversation may start with
only a pointer to this file. From Resume, open the milestone file for
the phase body.

1. Read Resume, the phase-index Status column, and the previous
   phase's Status, Accomplished, Lessons learned, and Handoff. The
   previous phase may sit in the previous milestone file.
2. Re-read this phase Goal, Work items, Done means, and Exit gate.
   Open a lock when the phase cites it.
3. Opening this chat is the approval for this one phase. The Sitting
   section is the rule. Ask only when a lock names a behavior and
   leaves a stored string, a column, the units, or an order unpinned.
4. Edit source for this phase.
5. Ask again when a requirement is ambiguous. Do not guess a schema
   change, a stored string, or a column name.
6. Update the Working Log in this file when a major piece lands.
7. The named command has passed. Then write Status, Accomplished,
   Lessons learned, and Handoff. Check work items and set Status
   complete only after that. Intent to verify is not verification.
   A sitting that ends before Done means writes in-progress Status
   and Handoff, and leaves unfinished items unchecked.
8. Stop. The next phase starts in a new conversation.
9. Never apply a database migration. Prepare the packet and hand it
   over. Re-check the next free Acuranzo file, QueryRef, and lookup
   id on disk before numbering anything. The snapshot in this file
   goes stale.
10. Lithium rules win on the SPA:
   [`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) and
   [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).
   Files stay at or under 1000 lines. CSS first. `element.style` is
   limited to drag geometry (`top`, `left`, `width`, `height`).
   No `alert`, `confirm`, or `prompt`. Log through `log(...)`.
   Preview HTML goes through DOMPurify and a sandboxed iframe.
11. Helium rules win on packets:
    [`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md),
    then Designs, packs, and numbers in
    [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). One file, one change.
    Copy an existing seed. Do not invent macros.
12. Do not add a Hydrogen `test_NN` script, a new npm dependency, or a
    new manager id unless a work item in this plan says so.
13. Do not log JWTs, script params that contain report data, or
    datasource rows in normal logs.

### Verification

| Change | Check |
| --- | --- |
| Lithium JS / CSS | `npm test`, `npm run lint`, `npm run lint:css` from `elements/003-lithium` |
| Vitest | Add or extend a test under `tests/unit/` when the phase changes a function the test can call |
| HTML template | `npm run templates:copy` after the edit |
| Designer UI | Unit tests in the phase chat. Andrew checks the flow on `https://lithium.500courses.com`. A screenshot is not the gate |
| 500 Courses | Andrew deploys and checks `https://lithium.500courses.com`. The agent does not deploy, log in, or drive that check |
| Helium packet | Hand it over. Test 31 and Test 98 when Andrew asks. Payload regenerate (`mka` or `payload-generate.sh`) before tests 30–38 or 71. `mkt` does not refresh the archive |
| Live render | Andrew runs the conduit script call after the seed is applied and reports the result |

## Priority

| | |
| --- | --- |
| **Band** | Open implementation as of 2026-10-30. Each phase starts with `cdli && grok`. CATCHUP Phase 19 stays parked until Andrew names Course Builder. |
| **Effort** | XL. Each phase is one behavior and one gate. A sidequest stays unnumbered unless he names it. Phase 10-UI is that name. It does not change the count of 72. |
| **Done** | Phases 0–18 of 72 complete (2026-11-03), plus labeled Phase 13b (section lookups 074 and 075, applied); labeled Phase 14b (property panel preferences, complete 2026-11-03). Phase 14 closed after Andrew said "Lets wrap this phase". Phases 6 and 7 closed after Andrew confirmed drag, Undo, Redo, export, import, and the dots. Phase 8 closed after Andrew accepted the rulers, the corner cycle, and the unit name. The 2026-11-01 sidequest draws each band once, at the content width. The top ruler sits above the first band's descriptor and starts at 0. Each left ruler starts at 0 at that band's body. Snap starts on. Paper height and margin guides stay off the surface. Phase 9 closed after Andrew said the view scale works pretty well. Phase 10 closed after Andrew said static text works pretty well. Phase 10-UI is complete. Andrew said the changes are working. Phase 17 is complete. Phase 18 is complete (2026-10-31). Andrew said the changes are working. Phase 11 is complete (2026-11-02). Andrew confirmed lookup 053 after the 1436 apply. Phase 13 is complete (2026-11-02); lookup 057 has eight page sizes, with A4 and A3 landscape skipped by choice. Phase 15 is complete (2026-11-03): Andrew confirmed fields, sample, band floor, and the audit footer. Phase 16 is complete (2026-11-03). |
| **Why now** | Hydrogen's host work is close to done. The report writer is the next product surface, and the lookups and table already exist. |
| **Do not start casually** | The JSON contract has to be agreed before the designer and the Lua script both grow around it. |

## Scope

| Tree | Role in this plan |
| --- | --- |
| `elements/003-lithium/src/managers/reports/` | Manager 24. Shell, canvas, rulers, properties |
| `elements/003-lithium/src/managers/reports/model/` | Pure units, document, undo. Vitest imports these |
| `elements/003-lithium/tests/unit/` | Tests for the model and, later, `invokeScript` |
| `elements/003-lithium/tests/fixtures/reporting/` | Definition, datasources, expected intermediate, expected HTML |
| [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md) | Format version 1 contract, written in Phase 1. This plan stays the gate list |
| `elements/002-helium/acuranzo/migrations/` | Lookup repair, QueryRefs, `Reports.Render`, lua-pdfkit, lua-zint, d3.lua |
| `elements/001-hydrogen/hydrogen/src/` | No new C. Lithium calls the existing Reporting endpoint. The script does not. |

Runtime truth for the manager id is `manager-loader.js` and
`config/lithium.json`: id **24**, key `024.Report Manager`, module
`src/managers/reports/reports.js`. The shell fetches
`reports.html`, starts from `createBlank()`, and imports and
exports the definition file. `#reports-stage` lists that
definition. `destroy` and `cleanup` release the same listeners.
[`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
names Report Manager as id 24. Id 12 is Calendar. Do not renumber
the manager. Phase 4 is complete (2026-10-31). Andrew confirmed
the shell on `https://lithium.500courses.com`. Phase 5 is complete
(2026-10-31). Andrew confirmed the page on
`https://lithium.500courses.com`. Letter is larger than a 4 in
page. Fixture guides sit inset 0.25 in. Phases 6 and 7 are
complete (2026-10-31). The bands resize, Undo and Redo are
registered shortcuts, and each band body draws the section
grid. Phase 8 is complete (2026-10-31). Andrew accepted
the rulers, the corner cycle, and the unit name. The design
surface is a stack of bands (2026-11-01). Each band is drawn
once. The first band carries the top ruler, measuring the
content width from 0. Each band carries a left ruler that
starts at 0. The corner cycles inches, centimetres, and
points and leaves the file unit alone. Paper height and
margin guides stay off the surface. Phase 9 is complete
(2026-10-31). Andrew said the view scale works pretty well.
Fit width, 100%, and zoom are on the toolbar.
`--reports-scale` scales the band stack, the grid, and the rulers.
Phase 10 is complete (2026-10-31). Text adds a static
item on the selected band. Drag and the corner commit on
pointerup. Phase 10-UI is complete (2026-10-31). The
left pane lists reports. Report, Editor, JSON, and
Revisions are the right-hand pages. Text sits on the
third toolbar. Manager chrome, below, is that shell.

## Goals

1. A designer that places bands and items on a page, with a dot or
   line grid, resizable bands, and rulers that track the pointer.
2. A versioned JSON definition that round-trips through a file and,
   after Phase 18, through the `reports` table.
3. One Lua program, `Reports.Render`. Compose maps datasources onto
   the definition and assigns pages. Emit turns that intermediate
   document into the requested stage. `pdf` is the default.
4. PDF is the default export. HTML, SVG, and CSV are the other script
   stages, in the phase-index order. SVG is one document with page
   breaks marked. PNG is that SVG rasterized by Lithium through
   `image_scale`. A label-sized report is one page. HTML is an export
   and the preview medium at once.
5. PDF from lua-pdfkit, barcodes from lua-zint, charts from d3.lua.
   The script embeds image bytes it has already resolved. Lithium calls
   `POST /api/reporting/image_scale` when a raster is required.
   Hydrogen does not grow a PDF, barcode, or chart library.
6. Charts and barcodes are first-class items. They render as SVG in the
   HTML, preview, and SVG paths, and the end state for PDF is the
   vector objects the zint and d3 ports emit natively. Any raster
   stopgap before that lands is named per phase, not assumed.
7. CSV as the tabular download. A native spreadsheet writer waits.
8. Report authors write Lua functions in the definition JSON.
   Each function names its return type. `Reports.Render` compiles
   and runs them, then formats a number or a datetime with the item
   mask. A field formula receives the current record. A header,
   footer, or break receives the context for that band. Group and
   report totals are known before those headers are formatted.
9. The existing lookup families stay the catalog of object types,
   attributes, and page sizes.
10. Built-in aggregates: count, unique, sum, avg, weighted mean,
    mean, mode, and median.
11. Chart items bind data with full scope flexibility: the band's own
    datasource, a named datasource, the current group, or a custom
    function, placed on any band.
12. Image elements carry their own resolution. A repeated image is
    stored once in the PDF and each placement references it.
13. A number, date, aggregate, or parameter prints through a format
    mask. Null is blank. Align and font are honored. Text clips to
    its box. A group header stays with its first detail row.
14. A datasource may name a QueryRef and the report's parameters.
    The canvas still designs against sample rows. Phase 29
    runs the query.

## Out of scope until a later plan

- Crosstabs, subreports, packages, shapes, and memos. The lookup rows
    can stay. The palette hides them. Chart keys 42 and 44, and barcode
    keys 43 and 45, are v1.
- Chart interactivity. The exported chart is static SVG or a PDF
    vector object; hover, zoom, and tooltips are a later idea.
- A user's own SVG entering a PDF as a vector. Phase 71's native
  output serves charts and barcodes first; a user-supplied SVG
  rasterizes through `image_scale` until a later plan extends the
  ports' native output to imported art.
- A native XLS writer. CSV is the spreadsheet stand-in.
- A SQL catalog inside the designer. A datasource is a JSON sample
  or a named QueryRef. The designer does not edit SQL.
- Browser PDF generation (`jsPDF`) and a PDF thumbnail viewer.
- Mailing a report. Mail Relay already exists for mail.
- A new Hydrogen subsystem, config letter, or C route. PDF, barcodes,
  and charts stay in Lua. Lithium calls the Reporting endpoint that
  already shipped when a raster is required.
- Rewriting CATCHUP, Course Builder, or Argent's financial queries.
- Measuring wrapped text inside Lua. v1 bands have a fixed height.
- Handing a user formula the Hydrogen host API. `H` stays in
  `Reports.Render`. `require` stays the loader for lua-pdfkit,
  lua-zint, and d3.lua.
- Datasource payloads over the conduit script param cap (256 KiB
  default, `ClientInvokeMaxParamsBytes`).

## What already exists

### Lithium

Manager 24 is the designer shell. It shows the report name,
Import, and Export. New is the list navigator. The stage lists the definition and draws
a stack of bands at 96 CSS pixels per inch. Paper height
stays in the file and stays off the design surface. There is
no page rectangle and no margin guide. Each band is drawn
once, at its stored height, in array order. The top ruler
is the first row of the first measured band, above that
band's role strip. The strip sits in the content column. A
ruler-colored pad under the corner keeps the left ruler
flush with the corner. The body draws the section grid.
Undo is Ctrl+Z and Redo is Ctrl+Y. The first measured band
carries the only top ruler. That ruler measures the content
width, page width minus the left and right margins, and
starts at 0. Every measured band carries its own left ruler.
Each starts at 0 at the top of that band's body and runs
for that body's height. The page margins are not drawn, so
the shared origin is that content corner. The bar is 28 CSS
pixels. The 1 inch tick is 96 CSS pixels from that ruler's
own origin. The vertical mouse line reads the top ruler. The
horizontal line reads the band under the pointer and hides
on leave. A band drag grows that band and its left ruler,
draws the ticks for the new length while the pointer moves,
and inverts the span on that ruler. The corner cycles inches, centimetres,
and points and leaves the file unit alone. The letters are
`#2a2a2a`. There is no right or bottom ruler. Phase 8 is
complete (2026-10-31). Its close-out still describes the
sheet. This paragraph is the live surface.
Phase 9 is complete (2026-10-31). Andrew said the view
scale works pretty well. Fit width, 100%, zoom in, and
zoom out sit on the toolbar. The wheel over the band stack,
rulers included, takes the same step while Ctrl or Cmd is
held. A bare wheel still
scrolls. `--reports-scale` scales the band stack, the grid,
and the rulers. 100% is 96 CSS pixels per inch. Fit width uses
the content width plus the 28 CSS pixel bar. The scale
stays off the definition. Phase 10 is complete
(2026-10-31). Andrew said it works pretty well. Text
adds a static item on the selected band. The canvas
shows the text and clips it to the box. Drag and the
corner commit on pointerup. Snap, on the item toolbar
before Clone, starts on. It lands a band resize and an
item move or resize on the current grid when the pointer
stops, and draws a light preview of that landing while
the pointer moves. The choice stays off the definition.
Delete and Undo restore the
item. Phase 10-UI is complete (2026-10-31). Andrew said the
changes are working. The
left pane lists reports. Report is the first page. It
edits the name, the title, and the list summary. Editor keeps
the stage. JSON edits the definition with `history`
removed. Revisions lists the saved revisions of the open
report. Undo, Redo, and Clear show only on Editor.
Snap sits before Clone and starts on. Clone and Delete sit with Text. Phase 17 is complete
(2026-10-31). Andrew reported QueryRefs 165–169 applied.
Phase 18 is complete (2026-10-31). Andrew said the
changes are working. The manager calls 165, 166, 167,
and 168. Andrew reported Delete Report applied. The list calls QueryRef 170. Category is a reports row column. The Revisions page lists saved revisions through QueryRef 171, copies one onto a new report, and reverts through QueryRef 172. acuranzo_1433.lua stores the signed-in account on insert 168. acuranzo_1428.lua through acuranzo_1435.lua are applied on Andrew's 2026-10-31 check. The next Acuranzo file is 1436. The next caller-facing QueryRef is 173.
Phase 11 has not started.
`conduit.js` can run QueryRefs.
`invokeScript` is not in the client. Scripting Manager does not call
`POST /api/conduit/script`. CATCHUP Phases 7–10 are the planned home
for that helper. Phase 25 of this plan adds it if those phases have
not landed, and reuses it if they have.

`closeManager` calls `destroy`, otherwise `cleanup`, otherwise
`teardown`. The report manager implements `destroy` and `cleanup`
as the same release, so either path drops the shell listeners.

### Helium

| Object | Where | What it holds |
| --- | --- | --- |
| `reports` table | [`acuranzo_1015.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua) | `(report_id, rev_id)` primary key. Columns `report`, `thumbnail`, `name`, `design`, `scripts`, `parameters`, `summary`, plus the usual audit. Created 2025-09-13. Andrew applied QueryRefs 165–169 (`acuranzo_1422.lua` through `acuranzo_1426.lua`) on 2026-10-31. The manager calls 165, 166, 167, 168, and 170. Andrew reported Delete Report applied. The list calls QueryRef 170. Category is a reports row column. [`acuranzo_1428.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1428.lua) through [`acuranzo_1432.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1432.lua) add that column and rewrite 165–168. Andrew reported the manager working on 2026-10-31. That check is the apply report for 1428–1435. [`acuranzo_1433.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1433.lua) rewrites 168 so `created_id` is the signed-in account. [`acuranzo_1434.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1434.lua) is QueryRef 171, List Report Revisions. [`acuranzo_1435.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1435.lua) is QueryRef 172, Delete Newer Report Revisions. The next Acuranzo file is 1436. The next caller-facing QueryRef is 173. 169 waits for Phase 51. Tabulator `groupBy` stays off. |
| Lookup 053 | [`acuranzo_1086.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1086.lua) | Report Object Types, keys 0–60. |
| Lookup 054 | [`acuranzo_1087.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1087.lua) | Report Object Attributes, keys 0–40. |
| Lookup 057 | [`acuranzo_1090.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1090.lua) | Two page-size rows. |

Lithium QueryRefs 053 and 054 are not these lookup ids. QueryRef 053
([`acuranzo_1146.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1146.lua))
is Get Themes and reads lookup 41. QueryRef 054
([`acuranzo_1148.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1148.lua))
is Get Icons and reads lookup 40. The reader for a lookup id is
QueryRef 026, Get Lookup, parameter `LOOKUPID`
([`acuranzo_1117.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1117.lua)).

Lookup 053 `value_int` groups the rows. This reading is from the
names, and Phase 0 confirms it:

| value_int | Group | Examples |
| --- | --- | --- |
| 0 | Structural | Report Top, Package, Report Bottom, Report, Section |
| 1 | Static | Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode, SubReport |
| 2 | Field | Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode |
| 3 | Calculated | Counter, Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode |
| 4 | Aggregate | Text through Barcode |
| 5 | CrossTab | Text through Barcode |
| 6 | Band | Package through Package Footer, plus Ruler |
| 7 | System | DateTime, User, Server, Version |

`key_idx` is the identity. `sort_seq` repeats inside a group (Field
Shape and Field Chart are both 6). Persist `key_idx`.

Lookup 054 is the property catalog. Keys 0–5 are position (Top, Left,
Width, Height, Align, Fit). Key 7 is Field, key 8 is Object Datasource.
Keys 15–17 are font family, size, and style. Keys 18–19 are report
title and template. Keys 20–24 and 40 are the grid. Keys 25–28 are the
ruler. Keys 30–39 are page size, orientation, units, and margins.

Lookup 053 carries chart keys 42 (Static Chart), 44 (Field Chart), 46
(Calculated Chart), 40 (Aggregate Chart), and 54 (CrossTab Chart), and
barcode keys 43 (Static Barcode) and 45 (Field Barcode). No lookup 054
row describes a chart or a barcode, and none of the chart or barcode
types advertises font or field attributes. Chart and barcode options
therefore live in their own panels beside the property panel, which is
what removes the need to wait on Phases 11–13 for them.
Most object types in lookup 053 advertise only
`attributes: [0, 1, 2, 3, 4, 14, 24, 29]`
(Top, Left, Width, Height, Align, Rotation, Grid Background Color,
Link). Page Header and Detail advertise a longer list that includes
fonts. A field cannot show its datasource or its font with the lists
as they stand. Phases 11–13 repair the v1 types.

Known seed defects, fixed in Phases 11–13:

- Lookup 053 key 7 (System Server) and key 16 (Group Footer) store
  `"Iion"` where the other rows store `"icon"`.
- Lookup 053 key 52 (CrossTab DateTime) has an unclosed `<fa>` tag.
- Lookup 057 key 1 is labeled `US Letter Legal` and its collection is
  US Letter landscape (`11 in` by `8.5 in`, page size `US Letter`).
- Lookup 057 has no Legal, Tabloid, A4, or A3 row (Phase 13 adds
  Legal and Tabloid in both orientations, A4 and A3 portrait).
- Lookup 054 font list is Helvetica, Open Sans, Cairo, Courier New,
  Lexend. Vanadium is the product face and is absent.
- The seed has no Page Number or Page Count type. Keys 0–60 are
  taken. Phase 0 assigned Page Number to key 61 and Page Count to
  key 62. Phase 11 writes them. Re-check the max key before writing
  the file.

Number snapshot on 2026-10-08, for orientation only:

- Highest Acuranzo file: `acuranzo_1385.lua`.
- Highest lookup id: **068**.
- Caller-facing QueryRefs in the low series run through **155**
  (`acuranzo_1381.lua`). `acuranzo_1145.lua` also sets
  `cfg.QUERY_REF = "1145"` for auth test data. Do not treat 156 as
  free without listing every `cfg.QUERY_REF` again.

Script seeds copy the `scripts` insert shape in
[`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua)
(`System.Info`). That row is `invokable = 0` and `mcp_access = 1`
because it is an MCP tool. `Reports.Render` is the opposite:
`invokable = 1`, `mcp_access = 0`. Phase 0 confirmed that.
Primary key of `scripts` is `(group_name, script_name)`.

### Retired Acuranzo client

The TMS WEB Core app is at `/mnt/extra/Projects/acuranzo/`. Pascal
sources are not on this machine. `Acuranzo-Client-Source.zip` contains
the `.dpr` and the README, not `Units/`. The useful record is the
compiled program `AcuranzoClient_6_1_5045.js` and
`CSS/AcuranzoClient-Reports.css`. `UnitReports.html` in that tree is
empty. The `.dpr` names `UnitReports.pas` as the form.

What that build actually did:

- A report list with new, clone, delete, search, upload, download,
  mail, and print controls, plus an info pane (SunEditor) and a JSON
  pane.
- A designer. `Reports.jsonData` held `Report` (page and grid
  properties, keys matching lookup labels such as `"Page Width"`) and
  `Sections[]`. Each section had `Name`, `Title`, `Link`, `Height`,
  grid properties, and `Items[]` with `Name`, `Title`, `Link`, `Top`,
  `Left`, `Width`, `Height`.
- Sections rendered as a header bar plus a resizable body. `Ruler`
  sections were skipped. A synthetic Report Top and Report Bottom
  framed the canvas.
- `getReportPixels` parsed strings like `"8.5 in"` at 96 CSS pixels
  per inch, and also `mm`, `cm`, and `px`.
- `addBackgroundPattern` drew an SVG tile: dots as circles, grid /
  dashed / dotted as paths. A `ResizeObserver` redrew it. Grid style,
  spacing, color, and weight were CSS variables.
- `createRuler` drew an SVG ruler in inches or centimetres, with a
  `.cursor-line` that followed `mousemove` and hid on `mouseleave`.
- Items used absolute `top` / `left` / `width` / `height`. Section
  resize and item drag used interact.js. Zoom used a separate
  panzoom object (`pzReportDesigner`).
- Selection filtered the property grid with lookup 053's attribute
  list. The compiled code reads `Lookups[-53]` for the family header.
  Do not assume Lithium's lookup cache uses a negative id. Read
  `src/shared/lookups.js` in the phase that binds the panel.
- Undo stored a string diff of the JSON and reapplied it.
- A PDF viewer (pdf.js) with thumbnails, zoom, rotate, crop, and
  column layouts. `jsPDF` was loaded on the page. This plan does not
  bring either library back.
- This pass did not find a write of the `reports` row. `report_id`
  appears as a hidden list column. Upload and download were the
  portable path that is visible in the compiled program. Treat the
  table as unused storage, not as a format we have to match byte for
  byte.

The behavior worth keeping is the page, the bands, the grid, the
rulers, and direct manipulation. The JSON keys are worth replacing.
Label strings break when a lookup caption changes. Format version 1
stores `key_idx` and stable item ids.

## Approved locks

Phase 0 approved these on 2026-10-10. Later phases implement this
text.

### Three documents

1. **Definition.** The file the designer exports. Format version 1.
2. **Datasources.** A JSON object. Each key is a datasource name from
   the definition. Each value is an array of objects. Zero keys is a
   legal static report.
3. **Intermediate.** The compose result. Pages of bands of items, with
   text already substituted and boxes in physical units. Emit reads
   this and does not see the original datasources.

### Definition shape

```json
{
  "format": 1,
  "name": "Orders",
  "title": "Orders",
  "page": {
    "sizeKey": 0,
    "orientation": "portrait",
    "units": "in",
    "width": "8.5 in",
    "height": "11 in",
    "margin": {
      "left": "0.25 in",
      "right": "0.25 in",
      "top": "0.25 in",
      "bottom": "0.25 in"
    }
  },
  "grid": {
    "visible": true,
    "style": "dots",
    "spacing": "0.125 in",
    "weight": "0.5",
    "color": "silver",
    "background": "white"
  },
  "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "Normal" },
  "datasources": [
    { "name": "orders", "kind": "json", "shape": "list" }
  ],
  "bands": [
    {
      "id": "ph",
      "typeKey": 13,
      "height": "0.4 in",
      "items": []
    },
    {
      "id": "detail",
      "typeKey": 15,
      "datasource": "orders",
      "height": "0.25 in",
      "items": [
        {
          "id": "i1",
          "typeKey": 20,
          "field": "customer",
          "box": {
            "left": "0 in",
            "top": "0 in",
            "width": "2 in",
            "height": "0.2 in"
          },
          "align": "Left"
        }
      ]
    }
  ],
  "functions": []
}
```

A field text item stores `align`. Omitted align is `Left`. A number,
datetime, aggregate, or parameter item also stores `mask`. An item
stores `font` only when it overrides the definition font, in the same
shape. The style captions below are legal in both places.

`history` stores undo and redo diffs on this definition. It is
omitted when both lists are empty. Each entry is a list of JSON
Pointer edits: `set`, `del`, or `ins`. `ins` is one added list
entry. A changed value stores that value, not the rest of the
report. The other keys are the current report. There is no cap.
Clear removes `history` and leaves the current report. That is
the production step. Export, import, and a later save of
`reports.report` keep the trail, and opening that file or
revision restores Undo and Redo. A bad `history` does not open.
Compose, preview, and the emitters skip `history` and leave it
out of the intermediate and the emitted file. Later edits commit
through this same trail. `exportDefinition` keeps the key.
`format` stays 1. The contract note is
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).

`typeKey` is lookup 053 `key_idx`. Lengths are strings with a unit
suffix (`in`, `mm`, `cm`, `pt`, `px`). The designer converts at
96 CSS pixels per inch for the unzoomed view. Emitted HTML uses the
physical unit in CSS so print does not depend on that 96.

v1 `typeKey` allowlist:

| key_idx | Role |
| --- | --- |
| 12 | Report header |
| 13 | Page header |
| 14 | Group header. One group field. |
| 15 | Detail. One datasource name. |
| 16 | Group footer |
| 17 | Page footer |
| 18 | Report footer |
| 0 | Static text |
| 20 | Field text |
| 21 | Field number |
| 24 | Field datetime |
| 5 | System datetime |
| 6 | System user |
| 42 | Static chart |
| 44 | Field chart |
| 43 | Static barcode |
| 45 | Field barcode |
| 61 | Page number |
| 62 | Page count |
| 63 | Parameter. One declared report parameter. |
| 28 | Calculated text. Names a function whose `returns` is `text`. |
| 29 | Calculated number. Names a function whose `returns` is `number`. The item mask formats the raw number. |
| 32 | Calculated datetime. Names a function whose `returns` is `datetime`. The item mask formats the raw value. |
| 34 | Aggregate text. Mode, min, max, first, and last of a text value. |
| 35 | Aggregate number. |
| 38 | Aggregate datetime. Min, max, first, and last of a datetime. |

Rulers, Report Top, and Report Bottom are canvas chrome. They are not
bands in the file. Grid and ruler settings on the definition are view
defaults, stored so the next session opens the same page.

Chart and barcode items add a `chart` or `barcode` object described
under Charts and in Phase 54. Their series, scope, and options bind to
datasources, and their option panels are bespoke, not lookup 054 rows.

`functions` holds the author's Lua. The shape is in the next section.
An early definition stores `[]`.

A group band names one field or, from Phase 45, one Lua function that
returns the key. Compose sorts that key with Lua's ordinary `<`
unless the band sets `sort` to false, in which case it breaks only
when consecutive keys differ. Nested groups are later.

### Value format

Compose turns a value into text before it builds the intermediate
document. Emit, including CSV, prints that text and does not format
again. The item property is `mask`, a string. It is not a lookup
054 key. Text fields omit it. An image item's `format` is the
raster it stores (`png`, or `jpeg` when the source is already
JPEG). It is not a mask, and it is not an XO payload.

Null and a missing value are an empty string. The mask is not
applied. The page never shows `nil` or `null`. A column that is
absent from the row warns once per item, not once per row. A present
column whose value is null does not warn.

An omitted mask prints a number as a plain decimal, with no thousands
separator and no scientific notation, and a datetime as `yyyy-mm-dd`
when it has no time and as `yyyy-mm-dd hh:nn` when it does. Anything
else is the string value.

A number mask uses `#` for an optional digit, `0` for a required
digit, `,` as the thousands separator, and `.` as the decimal point.
A trailing `%` multiplies by 100 and suffixes `%`. Wrapping the
pattern in parentheses prints a negative in parentheses and drops the
minus sign. A positive value prints without the parentheses. A leading
`$` is a literal prefix. `#,##0.00`,
`$#,##0.00`, `(#,##0.00)`, and `0%` are the v1 set's shapes. There
is no locale. The comma and the dot mean the same thing on every
machine.

A date mask uses `yyyy`, `yy`, `mmm`, `mm`, `d`, `dd`, `hh`, `nn`,
and `ss`. `mm` is the month. `nn` is the minutes. `mmm` is the
English abbreviation `Jan` through `Dec`. `d` is the day without a
leading zero. `dd` is the day from 01 to 31. `hh` is the hour from
00 to 23. `yyyy-mm-dd` and `d mmm yyyy` are enough for v1. The value
may be `yyyy-mm-dd`, `yyyy-mm-ddThh:nn:ss`, or a number of unix
seconds. A string is the civil time written in it, with no zone
shift. A number is unix seconds, formatted in UTC. A mask that omits
the time tokens drops the time. A mask that asks for time on a
date-only value uses `00:00:00`. Anything else warns once and prints
the raw value.

A number rounds half away from zero. `1.235` with `#,##0.00` prints
`1.24`. `-1.235` prints `-1.24`. Zero prints as the mask says,
including `0.00`. Blank-if-zero is lookup 054 key 10 and stays off
the v1 attribute lists.

Aggregate numbers use the same mask. A parameter item uses it too.
A calculated number and a calculated datetime use the same mask on
the raw value the function returned. The function does not format
the text. An aggregate reduces those raw values, and the aggregate
item's own mask formats the reduction. A mask that does not parse
warns once, and the raw value is printed.

| Value | Mask | Prints |
| --- | --- | --- |
| null | `#,##0.00` | empty string |
| `1234.5` | omitted | `1234.5` |
| `1234.5` | `#,##0.00` | `1,234.50` |
| `1.235` | `#,##0.00` | `1.24` |
| `-1.235` | `#,##0.00` | `-1.24` |
| `-1234.5` | `(#,##0.00)` | `(1,234.50)` |
| `1234.5` | `(#,##0.00)` | `1,234.50` |
| `1234.5` | `$#,##0.00` | `$1,234.50` |
| `0.25` | `0%` | `25%` |
| `5` | `0.00` | `5.00` |
| `0` | `#,##0.00` | `0.00` |
| `no` | `#,##0.00` | `no`, and one warning |
| `2026-03-05` | omitted | `2026-03-05` |
| `2026-03-05T14:07:09` | omitted | `2026-03-05 14:07` |
| `2026-03-05T14:07:09` | `d mmm yyyy` | `5 Mar 2026` |
| `2026-03-05` | `yyyy-mm-dd hh:nn` | `2026-03-05 00:00` |
| `1772719629` | `yyyy-mm-dd hh:nn` | `2026-03-05 14:07` |

System datetime is not a field value. Compose reads one instant for
the whole generation, and every system-datetime item uses it. When
`params.now` is a unix number, that is the instant. When the call
omits it, the instant is `H.system.now()`. A normal preview omits
`params.now`. The Phase 22 fixture sends `1772719629`, the same
instant as the mask row above. `params.now` is a call parameter. It
is not a report parameter, it is not stored on the definition, and
it is not a field of `params._hydrogen`.

**Timezone (lock 2026-11-03, Phase 22).** System datetime, and any
datetime item that formats an instant, carries `timezone`: `User` |
`UTC` | `Explicit`. `Explicit` stores an IANA name on the item. The
report stores a default IANA zone (may be empty). `User` uses
`params._hydrogen.tz` (JWT `tz` claim), else the report default, else
UTC plus an info log entry. `UTC` ignores both. Mid-session drift of
mint-time `tzoffset` is accepted. Scheduled / JWT-less runs rely on
Explicit or the report default. IANA zones resolve through luatz
(Phase 22a) reading host tzdata; before 22a, a zone equal to
`_hydrogen.tz` uses `_hydrogen.tzoffset` and any other IANA zone
falls back to UTC + info. A civil-string field value is never shifted.

**Mask language (lock 2026-11-03, Phase 22b).** Luxon tokens are the
mask language everywhere (Profile, designer, compose). The Phase 20
date dialect above (`mm` month, `nn` minutes, `hh` hour) is retired in
22b. v1 subset: `yyyy yy MMMM MMM MM M dd d EEEE EEE HH H mm m ss s
SSS`; `W`/`WW` ISO week (first week containing a Thursday); `n`/`nn`
local week deferred, first day of week Sunday until a Profile option;
literals in `'...'`. Locale macros (`D`, `t`, `f`, …) and zone-name
tokens are out until needed. Omitted masks become `yyyy-MM-dd` /
`yyyy-MM-dd HH:mm`. Presets live in editable Lookup 077; the
picker writes the mask string onto the item. Number masks unchanged.

### Page rules

Align and the font keys are real. Compose reads Align and font
family, size, and style from the item, then from the definition
font. On an item, `align` is `Left`, `Center`, or `Right`. Omitted
means `Left`. `font`, when present, is `{ family, size, style }`
and overrides the definition font. `style` is `Normal`, `Bold`,
`Italic`, or `Bold + Italic`. Those four are the v1 set. Underline,
superscript, subscript, and strikethrough stay in Later. Phase 12
sets lookup 054 key 4's list to `Left`, `Center`, and `Right`, and
key 17's list to the four style captions. HTML uses the family,
the size, and the style. PDF uses align, size, and style as bold
and italic flags, and keeps the standard face the port already
carries. Embedding Vanadium stays in Later. Rotation, link, and
conditional color stay in Later.

Lookup 054 keys 9, 10, 11, and 12 (Number Format, Blank if Zero,
Zero if Blank, Negative Parentheses) stay off every v1 attribute
list. The `mask` string is the format.

Text clips to the item box. The canvas clips the same way, so the
cutoff is visible while designing. The band does not grow. Measuring
wrapped text stays in Later.

A group header is not left alone at the bottom of a page. If the
header plus one detail row do not fit in the remaining content box,
both move to the next page. If they do not fit on an empty page
either, compose warns and places them anyway. Heights are fixed, so
this is arithmetic.

A page header or footer sees the group that is open on that page and
the detail rows placed on that page. Page totals are reduced from
those rows after the page list exists. Both the page header and the
page footer can show them.

Compose is not a single pass. Group and report totals are known
before the headers that display them are formatted.

1. Resolve rows. An array in `sources` wins. A query datasource with
   no array waits for Phase 29, and until that phase it warns and
   contributes no rows.
2. Evaluate each detail-row function. Store the raw value. Format
   each detail item from that raw value and the item mask. A detail
   `ctx` has no aggregates, so a total is a header or footer item.
3. Reduce each group, and the whole report, from raw field values
   and those raw function results. Format the group header, the
   group footer, the report header, and the report footer from
   those totals. A calculated item on those bands may read
   `ctx.aggregates`. Its return is masked by its own type, and it
   is not fed back into the totals.
4. Paginate on fixed heights and the stored text. A group header
   that does not fit with its first detail row moves with that row.
   The header is not repeated on a later page. Its summary is every
   row of the group, including rows that land on a later page.
5. Fill `page.count` and `page.rows`. Reduce the page totals from
   the raw values of the detail rows on that page. Format the page
   header and the page footer.

Keep-together changes which rows land on a page, so step 5 reads the
finished list.

### Datasource kinds

`kind` `json` is an array passed in `sources`, or the session sample
while designing. That array is not stored in the definition. `kind`
`query` names a caller-facing QueryRef and the binds that fill it.
Sample rows still design the canvas. A real run uses the QueryRef.

```json
{
  "parameters": [
    { "name": "from", "type": "date", "default": "2026-01-01" }
  ],
  "datasources": [
    {
      "name": "orders",
      "kind": "query",
      "queryRef": 120,
      "params": { "from": { "parameter": "from" } },
      "shape": "list"
    }
  ]
}
```

`parameters` live on the definition. Each one has `name`, `type`
(`text`, `number`, `date`, or `boolean`), and `default`. A bind is
`{ "parameter": "<name>" }` or `{ "literal": "<value>" }`. The
`reports.parameters` column stays `{}`. The run strip's current
values are session state, passed as `params.parameters`, and they
override the defaults. Saved presets wait with the parameter form.

An array in `sources` wins, which is how fixtures and the canvas
sample run. When that array is absent, Phase 29 runs the
QueryRef. It loads the installed row the way
[`argent_2059.lua`](/elements/002-helium/argent/migrations/argent_2059.lua)
does: `SELECT code FROM queries WHERE query_ref = :QREF AND
query_type_a28 = 1`, then `H.altquery_sync(params._hydrogen.database, code, binds)`
with the resolved binds. `query_type_a28 = 1` is the installed
query. The database name is the session claim. When that name is
empty or is not a configured connection, compose adds a warn entry
and returns no rows for that datasource. The worker login is the
one configured for that connection. The script does not send the
session JWT. Row filters, detail sort, and code-to-caption columns
stay on the query. The report already sorts a group by its key.

### User Lua

Report authors write functions. The source is stored on the definition
and runs inside `Reports.Render`. It is not a separate `scripts` row.

```json
{
  "name": "pack_code",
  "returns": "text",
  "source": "return function(ctx)\n  return ctx.row.month\nend"
}
```

`name` is a Lua identifier, unique in the report. `returns` is the
value type below. `source` is a text chunk that returns a function
of one argument. The chunk starts with `return`. A bare
`function(ctx)` is not a Lua chunk. `Reports.Render` compiles it
with `load(source, name, "t", env)` under Lua 5.5. Mode `"t"`
accepts source text.

`env` contains `string`, `math`, `table`, `utf8`, `tonumber`,
`tostring`, `type`, `pairs`, `ipairs`, `next`, `select`, `pcall`,
`error`, `date` (`os.date`), and `time` (`os.time`). The chunk's
environment is that table, so the formula sees those names and the
other functions in the same report. It does not see the worker
globals. `H` stays in `Reports.Render`. The scripting progress hook
already bounds the worker by `max_runtime_seconds`. This plan adds
no hook and no C.

`returns` is the value type compose expects. The v1 value types are
`text`, `number`, `datetime`, and `boolean`. A chart scope of
`custom` uses `rows`. Lookup 053 stays the object catalog: a
calculated number is key 29, and a memo is a later object type.
Image, memo, and shape are not function returns in v1. Parameter
type `date` is a datetime value.

| `returns` | Who may name the function | What compose does with the raw value |
| --- | --- | --- |
| `text` | Calculated text, key 28. A group key. | Prints the string. No mask. |
| `number` | Calculated number, key 29. A group key. An aggregate. | The item mask formats the display. An aggregate reduces the raw number. |
| `datetime` | Calculated datetime, key 32. | The datetime mask formats the display. Min, max, first, and last use that same mask. |
| `boolean` | `pageBreak` and `lineBreak`. | True or false. It is not a printed item. |
| `rows` | Chart scope `custom`. | The rows the chart plots. No mask. |

A missing `returns` means `text`. The panel shows `text` and notes
the omission. Creating a function writes `returns`. The item type
and `returns` must agree. A calculated number names a function
whose `returns` is `number`. That function returns the raw number,
such as `1234.5`. Compose applies the item's `mask` with the Value
format rules, so `1234.5` with `#,##0.00` prints `1,234.50`. The
function does not insert the separators. A datetime function
returns a value the datetime mask already accepts. A text function
returns the string that is printed.

A value of the wrong type warns once per item, and that call prints
empty. A compile error stops compose and returns the message. A
runtime error on one call is a warning and an empty value, so one
bad row does not drop the rest of the report. An unknown function
name is a compose error.

Every `ctx` includes `parameters`, the run values with defaults
filled in. `ctx` otherwise depends on where the function runs:

| Call | What else `ctx` holds |
| --- | --- |
| Calculated item on a detail | `row` (the current record), `prev`, `index` (1-based in the detail source), `band`. No aggregates. |
| Group header or footer | `row` is the first row of the group. `prev` is the last row of the previous group, or nil. `index` is that first row's index in the detail source. `rows` is the group's records. `group.key`. `aggregates` for the whole group. `band`. |
| Page header or footer | `page.number`, `page.count`, `group` for the group open on that page, `page.rows`, and `aggregates` for those rows. |
| Report header or footer | `report.name`, and `aggregates` for every detail row. |
| Group, page, or line break | `row`, `prev`, and `index`. |

A calculated item stores `"function": "pack_code"`. The item does
not store the source.

Breaks, implemented in Phases 45, 46, and 47:

- `groupKey` names a text or number function. A text key compares
  as a string. A number key compares as a number, so the display
  mask does not split one value into two groups. The one-field
  group remains the path with no Lua. One level.
- `pageBreak` names a boolean function. A true result starts a new
  page before the current row. Any other value warns once and
  counts as false. Fixed-height paging still applies.
- `lineBreak` names a boolean function. A true result inserts one
  blank detail-height before the current row. Any other value
  warns once and counts as false. A newline inside a calculated
  value is kept by HTML (`white-space: pre-line`) and by the PDF
  text drawer. The stored band height does not change. Overflow
  clips.

The pack-date fixture in Phase 33 is an invented pattern (month
letter, week digit, day), standing in for the grower-specific codes
on a PTI label. It is not a PTI specification.

### Primitive aggregates

Lookup 053 key 35 (Aggregate Number) stores `op` and either `field`
or `function`. `weighted` also stores `weight`. The engine computes
these. An author who needs a different reduction writes a header or
footer function and reads `ctx.aggregates`.

| op | Result |
| --- | --- |
| `count` | Rows in the band's scope. Empty scope is 0. |
| `unique` | Distinct values. Numbers compare as numbers. Text compares as text. A datetime compares as its raw value. Empty scope is 0. |
| `sum` | Sum of the raw numeric values. |
| `avg` | Arithmetic mean. |
| `mean` | The same arithmetic mean as `avg`. |
| `weighted` | Sum of `value * weight`, divided by the sum of `weight`. `value` is the field or the function result. `weight` is a field. A zero sum of weights, or no numeric pair left, prints an empty string and adds one warn entry. |
| `median` | Middle value after a numeric sort. Even count: mean of the two central numbers. |
| `mode` | Most common value. Text uses key 34 (Aggregate Text). A number uses key 35. A tie keeps the smallest value under Lua `<` and records a warning. |
| `min` | Smallest value. Numbers compare as numbers. Text compares with Lua `<`, which is code-point order. A datetime uses the rule below. |
| `max` | Largest value, by the same comparison as `min`. |
| `first` | First non-null value in the scope's row order. This is not `min`. |
| `last` | Last non-null value in that same row order. This is not `max`. |
| `filled` | How many values are non-null. Empty scope is 0. |

An aggregate names a row column with `field`, or a detail function
with `function`. Exactly one of those is set. The function runs on
each detail row with that row's context. The reduction uses the raw
return values, not the masked text. `weight` stays a field name.

| Input | Ops the designer offers |
| --- | --- |
| Text, or a text function | `count`, `filled`, `unique`, `min`, `max`, `first`, `last`, `mode` |
| Number, or a number function | `count`, `filled`, `unique`, `sum`, `avg`, `mean`, `weighted`, `median`, `mode`, `min`, `max`, `first`, `last` |
| Datetime, or a datetime function | `count`, `filled`, `unique`, `min`, `max`, `first`, `last` |

`count`, `filled`, and `unique` are numbers and use key 35. So do
`sum`, `avg`, `mean`, `weighted`, and `median`. `mode`, `min`,
`max`, `first`, and `last` keep the input's type: text uses key 34,
a number uses key 35, and a datetime uses key 38. A text result has
no mask. A number result uses the number mask. A datetime result
uses the datetime mask.

`ctx.aggregates` stores the raw result. The mask is applied when
the aggregate item is formatted, so a header formula can read two
raw results and subtract them. A range is that formula. There is
no `range` op, and no standard-deviation op.

Nulls are skipped by every op except `count`. A skipped null does
not warn. `count` still counts the row. `first` and `last` walk
the scope in its row order: the query's order, then the group sort
when the band sorts. Page scope walks the rows in the order they
were placed on that page. An empty `min`, `max`, `first`, or
`last` prints an empty string.

A datetime is either a civil string or a unix number, using the
same two forms as Value format. The first non-null value picks
which one this total uses. A later value of the other kind warns
once per item and is skipped. Strings compare as written, which
is chronological for `yyyy-mm-dd` and `yyyy-mm-ddThh:nn:ss`.
Numbers compare as numbers.

A field has no declared type, so a value that does not fit the op
warns and is skipped at runtime. A function has `returns`, so the
designer hides the ops that type cannot support. Boolean functions
and `rows` functions are not aggregate inputs.

Group scope is the current group. Report scope is every detail row.
Page scope is the detail rows placed on that page. The group header
and the group footer show the same group totals. The report header
and the report footer show the same report totals. The page header
and the page footer show the same page totals. A non-numeric value
in a numeric op warns and is skipped. An empty numeric result is an
empty string. Phase 38 places the results on `ctx.aggregates` for
the header and footer formulas of that scope, keyed by the op and
the field or function name.

### Charts

A chart item is lookup 053 key 42 (Static Chart) or key 44 (Field
Chart). A Static Chart plots a datasource with fixed options. A Field
Chart uses scope `record`. The current record of the chart's window
supplies `keyField`. The chart does not repeat on every detail row.
A field barcode does. Both chart kinds carry a `chart` object.

```json
{
  "id": "c1",
  "typeKey": 44,
  "box": {
    "left": "0 in",
    "top": "0 in",
    "width": "6 in",
    "height": "3 in"
  },
  "chart": {
    "datasource": "orders",
    "scope": "group",
    "keyField": "grower",
    "categoryField": "date",
    "series": [
      { "field": "amount", "label": "Amount", "role": "bar",
        "stack": "s1", "axis": "left", "color": "Color-A" },
      { "field": "target", "label": "Target", "role": "line",
        "axis": "right", "weight": 4 }
    ],
    "options": {
      "title": "Orders by day",
      "legend": true,
      "legendPosition": "top-left",
      "yLeft": { "label": "Amount", "min": 0, "max": 100, "ticksEvery": 10 },
      "yRight": { "label": "Target", "ticksEvery": 5 },
      "grid": true,
      "gradient": true,
      "roundedBars": true,
      "dpi": 150
    }
  }
}
```

**Series first.** There is no single chart type to pick up front. The
author adds series, and each series picks a role: `bar` (optionally a
stacked member), `line`, `area`, or the category series of a pie. The
chart's shape falls out of the roles — all `bar` is a bar chart, `bar`
members sharing a `stack` are a stacked bar, all `line` is a line
chart, and a `line` beside `bar` on two `axis` sides is the combo.
`kind` survives as optional shorthand for the common cases and is
derived at compose otherwise. The combo case is first-class, not an
exception: the fixture set includes a stacked bar with an overlaid
line, which is also the shape of the D3 port's own demo chart.

**Scopes.** All four ship in v1. Phases 59 and 60 draw `band`,
`group`, and `record`. Phase 64 draws `custom` by calling the one
compiler Phase 33 owns:

| scope | Data the chart plots |
| --- | --- |
| `band` | Every row of `datasource`, which defaults to the band's own. |
| `group` | The rows of the current group. |
| `record` | The rows of `datasource` whose `keyField` equals the current record's. |
| `custom` | The rows a function from `functions[]` returns for the current context. That function's `returns` is `rows`. |

`datasource` may name any datasource on the definition, so a chart can
plot something completely different from the band it sits on. A chart
may be placed on any band. It draws on the first instance of its band
inside its window: once per group on a group band, once per page on a
detail or page band, once per report on a report band.

That window is the chart rule. A chart on a detail band is drawn
once on the page, on the first detail of that page, not once per
row. Scope `record` uses that first row. Scope `band` plots the
named datasource. A field barcode is a different item and repeats
on every detail row.

**Ports.** The SVG form comes from d3.lua. The end state for PDF is
the port's native PDF output (Phase 71): vector objects lua-pdfkit
places directly. Until Phase 71, Lithium rasterizes the chart SVG
through `image_scale` at the chart's own `dpi` option (a chart
defaults higher than text, because axis labels suffer at 72) and
passes the PNG in `params.rasters`. The script embeds those bytes.
The ports stay SVG producers for the HTML, preview, and SVG paths.
PNG export is Lithium rasterizing that SVG. Composing happens once;
the chart's data is resolved per the scope above and then drawn in
whichever form the stage needs.

**Pie.** Pie is in v1. The port does not have an arc yet. The
port as it stands is `scaleBand`, `scaleLinear`,
`axisLeft`/`axisRight`/`axisBottom`, `stack`, `curveBasis`, `line`,
and `range`. Phase 57 names the arc this plan will add. Phase 63
adds that arc in the port and previews one pie. Everything else in
the v1 set is already in the port.

**Fonts.** A chart uses the definition's face in the SVG paths and a
standard face in the PDF, the same rule as text.

### Intermediate shape

Compose output is JSON. One page object per page. Each page has the
bands that landed on it, in order. Item text is the substituted
string, already passed through the mask. Boxes stay in physical
units relative to the band. Emit reads `text`, `align`, `font`, and
`box`, and clips to the box. Emit does not format again.

```json
{
  "id": "amount",
  "typeKey": 21,
  "text": "1,234.50",
  "align": "Left",
  "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "Normal" },
  "box": { "left": "0 in", "top": "0 in", "width": "2 in", "height": "0.2 in" }
}
```

A group break starts a group header and ends with a group footer.
Page header and page footer repeat. Report header is on page 1.
Report footer is on the last page.

Page breaks use fixed band heights against the content box (page size
minus margins). An item keeps the height stored on it. Text that does
not fit is the emit format's clipping problem. Lua does not measure
glyphs in v1.

### Emit

`params.stage` selects the stage on `Reports.Render`:

| stage | Result |
| --- | --- |
| `pdf` | Default. Base64 PDF bytes inside the JSON result when they fit. |
| `html` | `{ "html", "log" }`. One section per page with `break-after: page`. Export and preview medium. |
| `svg` | One SVG document. Page breaks are marked. |
| `csv` | Phase 49. One flat row per detail record. Data only. |
| `compose` | Intermediate JSON via `H.set_result_json`. |
| `check` | Phase 35. Compile the posted functions and return diagnostics. No rows are read. |

`stage=html` returns `{ "html": "<section...>", "log": [] }`. Each page is one section at the definition page size, with `box-sizing: border-box` and padding set to the four margins (top, right, bottom, left). Bands stack in that content box at their stored heights. An item is absolutely positioned from its box. A stored length drops the space between the number and the unit, so `0 in` is `0in`. Text is escaped. `overflow: hidden` and `white-space: pre-line` clip to the box and keep newlines. Align and the font family, size, and style come from the item, then the definition font. `Normal`, `Bold`, `Italic`, and `Bold + Italic` map to weight 400 or 700 and style normal or italic. A call that omits `stage` runs `compose` until Phase 31. The detail skeleton is [`/elements/003-lithium/tests/fixtures/reporting/detail.html`](/elements/003-lithium/tests/fixtures/reporting/detail.html).

There is no script stage `png`. Phase 67 posts the SVG result to
`image_scale`.

PDF is the default stage. HTML is an export and the preview medium.
The preview is that HTML, rendered on purpose into a sandboxed iframe,
with `sanitizeHtml` before `srcdoc`. The parent page never assigns it
to `innerHTML`. Preview HTML's Run button does this. Phase 24 is
complete (2026-11-04): Andrew confirmed the rendered page. The paper
sits in the Editor stage and takes its size from the first section.
Auto-render was declined in Phase 25 (2026-11-04). Run is the only
trigger. The generation log stays Phase 26. Charts and barcodes are SVG in the HTML,
preview, and SVG paths. In the PDF they are native vectors at
Phase 71. Until then, Lithium rasterizes the item SVG through
`image_scale` and passes the PNG bytes back in `params.rasters`,
keyed by item id. The script embeds those bytes. It does not call
`image_scale`. A missing raster omits the item and adds a warning.

**The result cap is 1 MiB, and config cannot raise it.**
`scoreboard_update_result_json` starts at `SCOREBOARD_RESULT_JSON_MAX`
(1 MiB) and lets `ClientInvokeMaxResultBytes` only shrink that.
`H.set_result_json` returns no Lua values on success or on rejection.
A rejected body is a log line, and the job is left without a result.
The script measures the encoded JSON before it calls.

- A PDF that fits returns `{ "pdf_base64": "...", "log": [] }`.
  Base64 puts the ceiling near three quarters of a megabyte of PDF.
- A PDF that does not fit returns a small table:
  `{ "error": "result_too_large", "cap": N, "size": N, "log": [] }`.
  Compose already ran. Delivery failed. Raising the config key past
  1 MiB does nothing. Changing the ceiling is a C change, and this
  plan does not add C.
- `H.set_result(type, location)` stores two strings. It does not store
  the PDF. A stored-output path waits on a store. Hydrogen has no
  generic file or blob store. `/api/files/local` is gcode-specific.
  Recorded in "Later, not scheduled".


### Generation log

Every result includes `log`, an array of `{ "level", "message" }`.
`level` is `info`, `warn`, or `error`. Where this plan says a warning,
that is one entry with `level` `warn`. A note that does not change the
output is `info`. A failure that still returns a document is `warn`.
A failure that returns `error` also appends one `error` entry and does
not throw out of the worker.

Lithium counts the three levels. It does not expect a second counts
object. Phase 19 starts returning `log`. Phase 26 (complete
2026-11-04) shows the entries
on a Log tab and flags that tab with the three counts. The tab is
aligned text in CodeMirror, Vanadium Mono, not a JSON viewer. The
last Run fills it, from Compose or from Preview HTML. The flag is
the worst level shown with the counts. The
counts are the last generation's session state. They are not columns
on `reports`. Switching reports clears them.

`Reports.Render` is one invokable row. lua-pdfkit, lua-zint, and
d3.lua are other rows, loaded with `require("group.script")`. Author
formulas are not rows. Hydrogen's DB searcher splits on the first dot:
group, then the rest of the name as `script_name`. `AllowDBModuleLoad`
has to be on for those three libraries. `require("lua-pdfkit")` is
rejected because it has no dot. The entry points are `lua-pdfkit.init`
and `zint.barcode`; the d3.lua entry point is pinned at the Phase 56
probe.

### Lua libraries

All three ports are pure Lua. None is in this repo yet. They were
written to run inside a host that has no native modules and no `io`.

| Library | Where it lives today | What Reports.Render calls |
| --- | --- | --- |
| lua-pdfkit | `/home/asimard/DA/triton4/triton-toolkit-5b12870/tasks/lua-pdfkit/solution/lua-pdfkit/` (pure Lua; its own repo planned) | `PDFDocument.new`, then `doc:finish()`, then `doc:output()` for the bytes. Port of PDFKit 0.19.1. JPEG and PNG from raw bytes or a `data:` URI. |
| lua-zint | `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/` (pure Lua; its own repo planned) | `zint.barcode(opts)` returns an SVG string. Symbologies in the port: QR, Code 39, Code 128, UPC-A, GS1-128. |
| d3.lua | Poseidon job artifact `/home/asimard/DA/poseidon1/jobs/lua-run10/` (pure Lua; its own repo planned). Validated byte-identical to D3.js on the reference pair `d3_chart.js` / `lua_chart.lua` | `d3.create_root`, `d3.scaleBand`, `d3.scaleLinear`, `d3.axisLeft` / `d3.axisRight` / `d3.axisBottom`, `d3.stack`, `d3.curveBasis`, `d3.line`, `d3.range`. Entry point pinned at the Phase 56 probe. |

luatz (Phase 22a) is a fourth pack: pure Lua, group `luatz`, seeded
as `luatz.*` rows. It reads zoneinfo from host tzdata
(`/usr/share/zoneinfo`) with read-only `io.open`; no zoneinfo is
bundled. Data refreshes with the host image's `tzdata` package; code
refreshes by a later migration that bumps the pinned version (see
22a Update / refresh).

**Status:** None of the three libraries is in this repository. They are
pure-Lua ports Andrew owns, each moving to its own repository: the zint
and pdfkit ports ship `tests/test.sh` plus `PORTING-NOTES.md` /
`AUTHOR-NOTES.md`, and the D3 port's validation is byte-equivalence
against D3.js itself. Each library gets its own seed phase and probe
(Phases 30, 53, and 56), and the packet records the repo commit or
version it was generated from. The path in the table is the fallback
when that repository does not exist yet. The seed phase stops and
asks when neither the repository nor the path is there. If a port
reaches LuaRocks, that helps
only if Hydrogen's sandbox can read `package.path`; the seeded
`group.script` module is the path this plan builds on, and the probe
confirms which one is live.

All three are producers. HTML, preview, and SVG use the SVG those
ports return. PNG is Lithium rasterizing that SVG. PDF embeds the
ports' native output at Phase 71. Until then, charts and barcodes
reach the PDF only when Lithium passes rasters in `params.rasters`,
the stopgap named in Phases 55 and 65. Photographs and imported art
stay on that raster path.

The sandbox nils `io` unless `Sandbox.AllowIo` is set. Emit does not
turn that on. lua-pdfkit writes the document with `output()` and never
needs a file. Passing `opts.output` to zint would call `io.open`, so
Reports.Render leaves that field unset and uses the returned string.
Image and font paths inside lua-pdfkit do call `io.open`. The report
path passes image bytes, and the first PDF faces are the standard
ones carried as AFM data. A phase that needs `io` stops and asks.

- `H.set_result_json` is capped by `ClientInvokeMaxResultBytes`
  (default 1 MiB). A text report fits. A PDF is base64 inside that JSON,
  so the ceiling is about three quarters of a megabyte of PDF. Fixtures
  in this plan stay under the cap. That ceiling is the compile-time
  `SCOREBOARD_RESULT_JSON_MAX`. The config key can only shrink it.

**Note:** `ClientInvokeMaxParamsBytes` (default 256 KiB, confirmed in
`config_scripting.c` and `conduit/script/script.c`) is a separate cap
on the conduit **script params** JSON, not on HTTP response bodies. The 256 KiB
figure in this plan refers to the params cap, which is the binding constraint
for a definition + datasource passed inline to `Reports.Render`.

**Raising the params cap to 10 MB is config-only.** Three things were
verified against the source:

- The enforcement point compares the serialized params length against
  `Scripting.ClientInvokeMaxParamsBytes` (`conduit/script/script.c`, both
  the raw and merged-params checks). There is no fixed buffer in that
  path, and the config field is an `int`, so 10 MiB fits.
- The API POST body ceiling is already **10 MiB**: `API_MAX_POST_SIZE`
  is `10240 * 1024` (`src/api/api_utils.h`). The HTTP layer never capped
  conduit script at 256 KiB — the scripting config is the only gate.
  (The header comment saying "64KB default" is stale and wrong.)
- The scoreboard strdups `params_json` per job and holds it until the
  entry is pruned, so the cap is also a memory dial: 10 MB per in-flight
  job. Fine for a few authenticated users on an appliance.

The recommendation is `Scripting.ClientInvokeMaxParamsBytes: 10485760`
in hydrogen.json, which matches the body ceiling exactly and leaves
embedded-base64 definitions workable inline. It is operator config and
needs no new C. Raising it does **not** lift the result ceiling.
`ClientInvokeMaxResultBytes` can only shrink `SCOREBOARD_RESULT_JSON_MAX`. A datasource or a rendered output past
either cap is still deferred to "Later, not scheduled" for the
stored-blob path.

### Images

Hydrogen's Reporting subsystem is the image scaler. It is disabled
until the operator sets `Reporting.Enabled`. Implementation plan:
[`/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md`](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md).
Endpoint:
[`/docs/H/api/reporting/reporting_endpoints.md`](/docs/H/api/reporting/reporting_endpoints.md).

`POST /api/reporting/image_scale` takes a base64 image plus width,
height, `units` (`px` or `pt`), `dpi`, and a format. ImageMagick
(MagickWand) decodes and scales. Format `xo` is a FlateDecode PDF
XObject payload (width, height, color space, optional alpha) built so
a PDF writer can insert the streams. lua-pdfkit accepts JPEG and PNG. Photographs are those bytes.
Phase 52 embeds each unique image once, through `doc:image`, and
every later placement references that object. The script does not
call this endpoint, and it does not grow an XO embedder. Lithium
calls `image_scale` for SVG rasterization (Phase 67, and charts and
barcodes until Phase 71). A PNG that comes back from that call
follows the same rule: one object, then references.

**SVG input and the raster paths.** `image_scale` documents SVG as a
supported input, with `dpi` applied before the read so rasterization
honors it, and `png` as a supported output. That one fact carries the
whole PNG emit (Phase 67), and it is also where a user's own imported
art goes. A user-supplied SVG placed as a static image is rasterized
this way; it does not enter the PDF as a vector until a later plan
extends the ports' native output to imported art.

**Image element options.** Photographs are raster. The item stores
`dpi` (default 72) and `format` (`png`, or `jpeg` when the stored
bytes are JPEG). Embedding a file or a paste opens a designer panel,
not a browser confirm. The panel computes a suggested pixel size
from the physical box and that dpi, rounding half away from zero,
and shows the width and the height. The author can change either
number. Each side is at least 1 pixel. A side that rounds to zero,
or that the author sets below 1, is kept at 1. The panel states
that confirming stores those pixels and does not keep the original
file. Confirm calls `image_scale` unless
the chosen size is the source's own pixel size, in which case the
original bytes are stored. Changing the box or the dpi later does
not resample. Opening the panel again does. The script embeds the
stored bytes and does not call `image_scale`. An image that repeats
is one PDF image object when every placement is handed the same
stored bytes (`doc._imageRegistry`). Different stored bytes are two
objects.

**Where the bytes come from.** All three source kinds are v1, and they
are all resolved to bytes at the start of generation, before any item
is placed:

| kind | Stored on the item | Resolved how | Trade-off |
| --- | --- | --- | --- |
| `embedded` | Base64 data URI in the definition, deduplicated in a content-hash-keyed registry | Already in hand | Portable, one file; makes the definition big |
| `reference` | A document id | `H.altquery_sync` on the session database reads `documents` (`acuranzo_1010.lua`: `(doc_id, rev_id)`, `file_data TEXT_BIG`, `file_name`) | Swapping the document changes the report without editing it; the report is no longer portable alone |
| `url` | A URL | `H.http.request_sync` at the start of generation, the same call the endpoint step uses | Live and small; the URL must stay available, and it slows every generation |

The designer's image browser loads a file from the local filesystem
or takes a paste from the clipboard. Both go through the confirm
panel above, and confirming stores them as `embedded`. A
`reference` picker lists documents through the Phase 17 read QueryRef.
A failed reference or an unreachable URL is a warning and an empty
image box.

Which kind an author picks is a size decision. Embedded base64 is the
reason a definition can outgrow the 256 KiB params cap; reference and
url kinds keep the definition small. Phase 27 previews an over-cap
definition by saving it first and rendering by `report_id`. The
params cap decides the path. It does not drop the preview quietly.

The script does not call `image_scale`. `params._hydrogen` carries
claims and does not carry the bearer token, and `H.system_token`
does not exist. Lithium holds the session JWT and is the only caller
of that endpoint. Reference bytes come from `H.altquery_sync` on
`params._hydrogen.database` and the Phase 17 document QueryRef, the
same database rule as a query datasource. URL bytes come from
`H.http.request_sync` against that URL, with no Hydrogen JWT. The
worker fetches whatever absolute `http://` or `https://` URL the
item stores. No new C wrapper is added.

The designer and the PDF emitter use `pt` (1/72 inch), matching
`DefaultDPI` 72. The on-screen designer stays at 96 CSS pixels per
inch for the unzoomed view. Those are different conversions of the
same physical unit.

`POST /api/reporting/image_scale` returns:
`{ "success": true, "image": "<base64>", "format": "png",
"mime_type": "image/png", "width": N, "height": N,
"input_format": "png", "input_dimensions": { "width": N, "height": N } }`.
Errors return HTTP 400/401/503 with `{ "success": false, "error": "...",
"details": "..." }`. On any non-200, the Lithium caller toasts and
omits that raster. A photograph that fails to resolve is a script
warning and an empty box (Phase 51).

### Call shape

```json
{
  "script": "Reports.Render",
  "params": {
    "stage": "html",
    "definition": { "format": 1 },
    "parameters": { "from": "2026-01-01" },
    "sources": { "orders": [{ "customer": "North" }] }
  },
  "wait": true,
  "timeout_seconds": 15
}
```

A stored report may send `report_id` and `rev_id` instead of
`definition`. The script loads the row through the Phase 17 QueryRef.
`stage` is optional and defaults to `pdf`. `params.now`, when it is
a unix number, is the generation instant from Value format. A
fixture may send it. A preview omits it. Client params stay under
`ClientInvokeMaxParamsBytes` (256 KiB unless the operator changed it).
A fixture in this plan fits. A larger datasource is a later phase that
stores the JSON and passes an id.

`params._hydrogen` is reserved. The client must not send it.

### `reports` columns

| Column | v1 use |
| --- | --- |
| `report_id`, `rev_id` | Identity. A save appends a revision. |
| `name` | List label. |
| `summary` | Short description. |
| `report` | Definition JSON text. |
| `parameters` | JSON object. Declarations live in the definition. v1 stores `{}`. |
| `scripts` | Unused in v1. Store `[]`. Author Lua lives in `report`. |
| `design` | Unused. Leave null. The old column name is ambiguous and this pass found no writer. |
| `thumbnail` | Unused in v1. |
| `collection` | Standard audit JSON. The designer does not read it. |

### Defaults

Compose and the designer share this table. A phase that adds a
property adds its row in that same sitting. Chart options other than
`dpi` get their rows in Phase 57. An embedded image stores the bytes
the author confirmed in Phase 50. There is no separate
downsample-cap property. Reuse is not a property. Same stored bytes
are one object.

The property panel shows the effective value. When the file omits the
key, the control still shows the default and notes that the file omits
it. The note is a label. Import accepts the omission. Compose uses the
value the panel showed. The panel does not reject a file for leaving
a key out.

Creating a report, a band, or an item writes the stored defaults into
the definition, so a person editing the JSON can see the value. A key
whose default is "omit" stays off the object. Import and export leave
an omitted key omitted until the operator edits that control. Editing
writes the key. Minimum band height stays `0.15 in`. That is a clamp,
not a default.

| Object | Property | On create | When the key is absent |
| --- | --- | --- | --- |
| Report | `name`, `title` | `Report` | `Report` |
| Report | `page.sizeKey` | `0` | Lookup 057 key 0, US Letter portrait, `8.5 in` by `11 in` |
| Report | `page.orientation` | `portrait` | `portrait` |
| Report | `page.units` | `in` | `in` |
| Report | each margin | `0.25 in` | `0.25 in` |
| Report | `grid.visible` | `true` | `true` |
| Report | `grid.style` | `dots` | `dots` |
| Report | `grid.spacing` | `0.125 in` | `0.125 in` |
| Report | `grid.weight` | `0.5` | `0.5` |
| Report | `grid.color` | `silver` | `silver` |
| Report | `grid.background` | `white` | `white` |
| Report | `font` | Vanadium Sans, `10 pt`, `Normal` | the same |
| Report | `datasources`, `functions` | `[]` | `[]` |
| Function | `returns` | `text`, or the type of the item or break that added it | `text`. The panel says so |
| Function | `source` | `return function(ctx)\n  return ""\nend` for text. Number returns `0`. Datetime returns nil. Boolean returns `false`. `rows` returns `{}` | empty source, which checks clean |
| Report | bands | page header, detail, page footer | a missing band is not invented |
| Band | height for report header, page header, page footer, report footer | `0.4 in` | `0.4 in` |
| Band | height for group header, detail, group footer | `0.25 in` | `0.25 in` |
| Item | `align` | `Left` | `Left` |
| Item | `font` | omitted | the report font. The panel says so |
| Item | `box` | `1.5 in` by `0.2 in`, centered in the band | that box |
| Item | static `text` | `Text` | `Text` |
| Item | `mask` on number, datetime, calculated number, calculated datetime, aggregate number, aggregate datetime, parameter | `""` | the omitted-mask rule in Value format. A datetime uses the datetime rule. The panel says which rule |
| Item | `mask` on text and aggregate text | omitted | the string itself. The panel shows no mask |
| Image | `dpi` | `72` | `72` |
| Image | `format` | `png` | `png` |
| Chart | `dpi` | `150` | `150` |
| Barcode | raster `dpi` | `300` | `300` |
| Call | `stage` | not stored on the definition | `pdf` |
| Call | `now` | not stored on the definition | `H.system.now()` when the call omits it |

### Designer rules

- No new npm package for v1, except `@codemirror/lint` in Phase 35
  and `@melloware/coloris` (the Pavillion colour picker) in Phase 14.
  Pointer events do the dragging and the resize. Zoom is a CSS
  variable on the canvas.
- Palette and property editors read lookups 053, 054, and 057.
  Captions and icons come from those rows. The file stores keys.
  The item buttons are the third editor toolbar in Manager chrome.
  Hidden types stay off it. Adding an item or a band is one of
  those buttons. Phases 10, 16, 22, 23, 28, 34, 38, 50, 54, and
  58 add a button. Phase 10 shipped Text on a single row.
  Phase 10-UI moved that button onto the third toolbar.
- The property panel shows the Defaults table, including a key the
  file omits. It is the Editor-page column in Manager chrome.
  The canvas uses the faces Lithium already loads.
  Vanadium Sans is the SPA face in
  [`/elements/003-lithium/src/styles/base.css`](/elements/003-lithium/src/styles/base.css).
- Sample datasource JSON in the designer is session state. It is how
  field items show a column name, including after a QueryRef run.
  It is not written into the definition.
- Import and export of the definition file work before the database
  save exists. The file is the artifact Phase 4 already proves.
  The file keeps `history` when the trail is non-empty. New omits
  the key. Import of a file that has `history` restores Undo and
  Redo.
- Split files before 1000 lines. Phase 10-UI added the table
  host, the JSON page, and the toolbars, and split before 1000
  lines. The shell is `reports.js`. Pointer edits are
  `reports-pointer.js`. The JSON page is `reports-json.js`. The
  list is `reports-table.js`. Later files stay `reports.html`,
  `reports.css`, `canvas.js`, `rulers.js`, `grid.js`,
  `properties.js`, `functions.js`, `chart-panel.js`, and
  `model/units.js`, `model/document.js`, `model/history.js`,
  `model/history-diff.js`.
- **DOMPurify is in `package.json`** (v3.3.2). Preview HTML is sanitized
  with `sanitizeHtml` before assignment to a sandboxed iframe `srcdoc`.
  The Run button landed in the Phase 24 sitting. Phase 25 is complete
  (2026-11-04): Run is the only trigger, and auto-render was declined.
  Barcode SVG must survive that sanitizer (Phase 54). This is
  the one place HTML from a script crosses into the DOM. Even though
  the preview HTML is fully generated by `Reports.Render` and there is
  no external HTML surface, the sanitizer stays as defense-in-depth
  for the iframe.

### Manager chrome

Andrew set this on 2026-10-31, after Phase 10, and named the
build Phase 10-UI. That label sits between phases 10 and 11.
It does not change the count of seventy-two phases, numbered
0 through 71. Phase 11 stays the lookup 053 packet. The shell
below is what Phase 10-UI built. Status stays in progress
until he checks `https://lithium.500courses.com`. The
page-size control and the property panel are still absent.
Phase 14 fills both.

The module follows Query Manager
([`/elements/003-lithium/src/managers/queries/`](/elements/003-lithium/src/managers/queries/)).

The left pane is a LithiumTable of `reports`, with the navigator
under it. The manager imports the Tabulator stylesheet, so the
list draws when Reports is the first manager opened. Add,
duplicate, and delete of a report row live on that navigator. New is not a designer toolbar button. A splitter
separates the panes. The main toolbar's first button collapses the
left pane, the same control Query Manager uses. Collapsed, the
Editor page is the full width.

The right pane's main toolbar stays visible on every page. The
three right-pane toolbars sit 1px apart. The first keeps its
top margin.

- Collapse, then the page tabs. Report is first. Editor is the
  page that opens. JSON is third. A later phase adds a tab on
  this bar. Preview is Phase 25. That tab's tools show only
  while it is the active page.
- Import and Export of the definition file. Export still writes
  `history` when the trail is non-empty. The Phase 4 file
  round-trip stays.

The Report page edits metadata that is already stored. Name and
title are on the definition and commit through the same history
trail. Summary is the `summary` column on the reports row.
Category is the `category` column on that same row. Neither is
a definition key. Tabulator `groupBy` stays off. A typed
category does not survive a reload until
[`acuranzo_1428.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1428.lua)
through
[`acuranzo_1432.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1432.lua)
are applied.

Undo, Redo, and Clear sit on the Editor toolbar, one bar below
the main toolbar, and show only on that page. They use the
definition history trail. A canvas edit, a property edit, a meta
edit, and a JSON edit are the same trail. Ctrl+Z and Ctrl+Y stay
registered for manager 24. Ctrl+Shift+Z stays Toggle zoom. The
buttons are hidden on Report and on JSON. CodeMirror still owns
Ctrl+Z while the JSON editor is focused.

Save and Cancel are the slot footer, through `ManagerEditHelper`,
the same path Query Manager uses. The definition is an external
editor on that helper, the way Query Manager registers CodeMirror.
The designer stays read-only until the row enters edit mode. A
change while editing marks the row dirty and enables Save and
Cancel. Opening another row does not mark it dirty. Save writes
the open definition, trail included, onto the row.
List is QueryRef 165, search is 166, open is 167, and save is
168. Andrew applied those on 2026-10-31. Save appends a
revision. `REPORT_ID` 0 allocates a report. Duplicate still
has no query. List Delete calls
[`acuranzo_1427.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1427.lua),
QueryRef 170. Andrew reported that file applied. Confirm first.
The statement removes every revision of that `report_id`. A
draft with no saved id is removed locally and does not call
170. Canvas item Delete does not confirm. Import and Export
still write the definition file. Navigator Add starts a local
blank report and enters edit mode. Name, category, and summary
use the string and text cell editors. Category is sent as
`STRING.CATEGORY`. It does not persist until 1428–1432 are
applied. Deploy the SPA before 1432, or together with it,
because `:CATEGORY` then becomes required. QueryRef
169 is the document image read and is not called here.

The Editor page is the current stage: a stack of bands, each
drawn once at its stored height, with the grid and the items.
Paper height stays in the file and stays off this surface.
There is no page rectangle and no margin guide. The first
measured band carries the top ruler above its descriptor, at
the content width, starting at 0. The descriptor sits in the
content column, and the left ruler of that band starts at 0
at the body, flush under the corner. Every measured band
carries a left ruler that starts at 0 at the top of its body.
Moving or resizing an item highlights that box on the top
ruler and on that band's left ruler, and the crosshair keeps
moving for the whole drag. The vertical line reads the top
ruler. The horizontal line reads the band under the pointer.
A band resize grows that band and its left ruler, draws the
ticks for the new length while the pointer moves, and
highlights that ruler. Snap starts on. A band resize and an
item move or resize land on the current grid when the pointer
stops. There is no right or bottom ruler.

The second toolbar shows only on the Editor page:

- Category buttons. A category appears when it has at least one v1
  item. Static is first and starts selected. Field, the database
  fields, arrives with Phase 15. Later categories follow lookup
  053 `value_int`: Calculated, Aggregate, System, and Band. Hidden
  types stay off every category. Switching category changes the
  third toolbar and leaves the definition alone.
- Undo, Redo, and Clear, at the right end of this bar.

The right end of the main toolbar holds the view controls, and
they show only on the Editor page: the page-size control, Fit
width, zoom out, the percent, 100%, and zoom in. Page size uses
lookup 057 and writes `sizeKey`, width, height, and margins, the
same write as Phase 14. The control is absent until that lookup
is loaded. The wheel zooms only while Ctrl or Cmd is held and
the pointer is over the band stack, rulers included.
A bare wheel still scrolls.

The third toolbar shows only on the Editor page. It lists the
item buttons for the selected category, then Clone, then Delete.
Static shows Text. The button stays. Dragging that icon onto a
band drops the same static-text rectangle, centered on the
pointer, then clamped to the band. Clone copies the selected item onto the same
band, assigns a new id, keeps the box, and selects the copy.
Delete is this button and the Delete key. It removes the item.
It does not remove the report row. Both are disabled when no
item is selected. A phase that adds a palette choice adds its
button on this bar, with the items.

The JSON page hides the second toolbar, the third toolbar, the
page-size control, and the zoom controls. It is a CodeMirror JSON
editor, the same stack as the Query Manager collection editor,
with a `LithiumEditorFooter`. The buffer is the open definition
with `history` removed. Folds start collapsed. Collapse all and
Expand all sit on the main toolbar and show only on this page.
A parsed edit commits one history step when the editor blurs or
the page changes. CodeMirror undo covers the buffer while it is
focused. A parse error toasts and leaves the definition. The
write keeps the existing trail and does not put `history` back
into the buffer.

The property panel is Phase 14, built on the shared
`LithiumProperties` component (see Property panel below). It is a
column on the left of the Editor canvas, right of the report table,
with its own splitter. It is hidden on the other pages, and it is
hidden until that phase fills it. When it ships, the read-only
summary below the canvas (`.reports-summary` from
`paintReportStage`) is removed. Its page, margin, grid, and font
facts become panel rows, and its band and item list becomes the
panel's Outline section. Chart and barcode options stay
beside it. The panel and the page-size control edit the same
page keys.

A control is shown only on the page that uses it. The JSON page
does not show Text, Delete, Clone, zoom, or page size. The
Editor page does not show the JSON fold buttons.

Selection, the view scale, and the ruler unit stay off the
definition.

### Property panel (Lock, 2026-11-02)

- **Shared component.** `LithiumProperties` lives under
  `/elements/003-lithium/src/properties/`, beside `src/tables/`
  (`LithiumTable`). It is built on Tabulator and knows nothing about
  reports. Reports wires it in `reports-panel.js`. Styles and other
  managers may reuse it later.
- **Layout.** One line per property: name, then value, with a
  draggable splitter between the two columns. A multi-line name or
  value centres both sides vertically. Rows are condensed. Sections
  collapse, each with its icon. A search box filters by label, by
  section, and by stable id. The panel width follows the same
  splitter as the other Lithium panes.
- **Placement.** Left of the Editor canvas and right of the report
  table, inside the right pane, with its own `lithium-splitter`. The
  table's collapse button does not collapse the panel. Canvas notes
  that explain why nothing is drawn (page or grid errors) stay on the
  canvas.
- **Outline section.** The first section is a navigator, not property
  rows: report, then each band, then its items, as in the old summary.
  Clicking an entry selects that band or item on the canvas, and the
  canvas selection highlights its entry. Report selects nothing and
  shows the report and page properties.
- **Name and title.** Open. Whether 054 keys 18–19 edit name and title
  in the panel, on the Report page, or both on one model is not
  locked. Ask Andrew in Phase 14.
- **Sections.** Lookup 074 holds the 053 palette groups and lookup
  075 holds the 054 attribute sections (Phase 13b; the ids are
  re-checked on disk the day 13b is written). Each is plain rows:
  `key_idx`, `value_txt` label, `sort_seq` order, and `collection`
  with `icon` and optional `lang`. A 053 row's group is its existing
  `value_int`. A 054 row gains a numeric `section` key in its
  collection. `Group` stays until Lithium reads `section`.
- **Icons.** Section icons live on 074 and 075. Per-row icons stay on
  the item's own row as `icon` (053, 054, 057). The panel shows the
  row icon when present, otherwise none.
- **Ids, not labels.** Sections and properties are addressed by
  `key_idx`. Favourites, hidden rows, order, and open sections store
  ids. A label is display only.
- **Labels and `lang`.** One resolver returns a display string from a
  row: exact locale under `lang`, then the same language, then the
  plain label (`Default`). Lists keep their stored values; a
  `lang` entry may hold a `List` of captions matched by position.
  The profile language wins over the browser language. v1 ships the
  resolver and English rows; translating rows is Later.
- **Empty selection** shows the report and page properties. Page size
  picks a lookup 057 row.
- **Editors.** Phase 14: Text, Number, List, Boolean, and Color
  (Coloris, ported from Pavillion). Phase 14b: Date on the existing
  Flatpickr (`shared/flatpickr-picker.js`). Field and Datasource are
  Phase 15. `None` is read-only. An unknown editor toasts and skips
  the row. Later editors (icon chooser, formula, image, chart,
  barcode) plug into the same component.
- **Commits** go through the report model, one Undo step per
  committed edit.
- **Preferences (Phase 14b).** Favourites, hidden properties, order
  within a section, open sections, and the splitter position are
  stored on the user profile, keyed by panel id and row id.

### Formula editor

The function editor is a CodeMirror 6 `EditorView` from
[`/elements/003-lithium/src/core/codemirror.js`](/elements/003-lithium/src/core/codemirror.js),
built with `buildEditorExtensions` and `language: 'lua'`. The Lua
personality is the in-repo stream language, `lua()` in that file:
keywords, comments, long strings, numbers, and builtins. The
Scripting manager already edits scripts with it. Reports uses the
same personality. It does not use `src/init/codemirror-init.js`,
and it does not add a second Lua mode.

The buffer also has line numbers, folding, search, bracket matching,
`--` comment continuation, and a `LithiumEditorFooter`. The face is
Vanadium Mono, same as the other editors.

Syntax diagnostics use `@codemirror/lint`. That package is already
in the lockfile transitively. Phase 35 adds it to the direct
dependencies so the editor owns the import. The messages come from
Lua 5.5 `load` via `stage=check`, not from a JavaScript parser and
not from luacheck. The editor debounces a check of the open
function. A compile error underlines the line `load` reports. Empty
source is clean. Typing is never blocked, and a bad chunk can still
be saved. Compose remains the gate that refuses to render it.

Report undo records the function source when the editor blurs, not
on each keystroke. That blur commits one diff on `history`, the
same trail as a band resize. CodeMirror keeps its own undo while
the buffer is focused.

An undefined name is a runtime event in Lua. The editor does not
flag `io`, `H`, or a misspelled field while the author types. Those
fail when the line runs, and Phase 37 turns that into a warning and
an empty value.

The Functions list and the item share one buffer. `functions.js`
lists every function on the report: its name, its `returns`, and
the items and bands that name it. Selecting a row opens the
CodeMirror buffer. A calculated item, a group key, a break, and a
chart scope of `custom` store the function name only. The property
picker lists functions whose `returns` matches that use, and Edit
focuses the same buffer. A helper that no item names still appears
in the list. Renaming a function updates those references. Adding
a function from the list writes `returns` `text` and the text chunk
from the Defaults table. Adding one from a calculated item, a
break, or a custom chart writes that use's type and chunk.

### v1 render behavior

Given a definition and a source array, compose. An array in
`sources` wins. A query datasource with no array waits for
Phase 29 to fill it, and until that phase it warns and
contributes no rows:

- emits a static report when `sources` is empty and the detail band
  has no datasource
- repeats the detail band once per row
- substitutes field items from the row, already passed through the
  value-format rules, with null as an empty string
- honors Align and the font keys, and clips text to the item box
- prints system user from `params._hydrogen.username` (Phase 22)
- prints system datetime from the one generation instant, shifted
  by `params._hydrogen.tzoffset` minutes, then the item mask
  (Phase 22)
- prints page number and page count as integers through the number
  mask (Phase 23), and a Page label (053 key 64) from its
  `template` with `{n}` and `{c}` (default `Page {n} of {c}`)
- prints a parameter item from `params.parameters` or the definition
  default
- breaks pages on fixed heights, keeping a group header with its
  first detail row
- opens and closes one group level when a group band is present

The order is the five steps under Page rules. Phase 19 emits the
structural intermediate. Phase 20 applies the mask. Phase 21
paginates. Phase 24 emits HTML. Barcodes (Phase 54), charts
(Phases 59–63), user formulas (Phase 33), primitive aggregates
(Phase 38), and custom breaks (Phases 45–47) extend it.
Emit HTML reproduces the page list. The same intermediate fixture is
the oracle for both the Lua result and the preview.

## Phase 0 — Design lock

### Goal

Agree the locks above. No source edits.

### Entry gate

This file is the plan under discussion.

### Work items

- [x] 0.1 Confirm format version 1 stores lookup `key_idx` and unit
      strings, and that the retired label-keyed JSON is evidence, not
      the file we write.
- [x] 0.2 Confirm the `reports` column roles, including `design` and
      `thumbnail` left unused.
- [x] 0.3 Confirm the v1 type allowlist, and whether Page Number
      and Page Count are lookup 053 keys 61 and 62.
- [x] 0.4 Confirm Vanadium Sans (and any other face) is added to
      lookup 054's font list in Phase 12.
- [x] 0.5 Confirm `Reports.Render` stages `pdf` (default), `html`,
      `svg`, `csv`, `compose`, and `check`. PNG is the manager
      posting the SVG to `image_scale`. Confirm that
      lua-pdfkit, lua-zint, and d3.lua are seeded as `require`
      libraries (`lua-pdfkit.*` with group `lua-pdfkit`, `zint.*`
      with group `zint`, and `require("d3.lua")` with group `d3`
      and script name `lua`), `invokable = 0`, one seed phase and
      probe per library.
- [x] 0.6 Confirm PDF as the default stage. The build order is the
      phase index: compose and HTML, then PDF, then the later
      exports. Fixed band heights. Lua does not measure glyphs.
      CSV is flat data and stands in for a spreadsheet. HTML is
      both an export and the preview medium. SVG is a document
      export. PNG is Lithium rasterizing that SVG, and a
      label-sized report is one file.
- [x] 0.7 Confirm no new Hydrogen C. Lithium calls
      `POST /api/reporting/image_scale`. The script does not. PNG
      uses that endpoint's SVG input. Charts and barcodes reach
      the PDF as native vectors in Phases 68–71. Until Phase 71,
      Lithium may pass rasters in `params.rasters`. The one new
      direct npm dependency is `@codemirror/lint`.
- [x] 0.8 Confirm this plan waits behind CATCHUP until Andrew pulls
      it in, and that Phase 25 may add `invokeScript` if CATCHUP
      Phases 7–10 have not.
- [x] 0.9 Confirm manager 24, append-only revisions, sandboxed
      preview, the 256 KiB param cap, and the 1 MiB result cap.
      The result cap is compile-time. Config can only shrink it.
      A PDF that fits is base64 inside the JSON. A PDF that does
      not fit returns a small error table. `H.set_result` with a
      location is later work and needs a store.
- [x] 0.10 Confirm author Lua lives in `definition.functions` and
      runs in the environment in User Lua. A detail formula sees
      the current record. Headers, footers, and breaks see the band
      context. Confirm the primitive ops, with `avg` and `mean` as
      the same arithmetic mean, and a line-break predicate that
      inserts one blank detail-height. The formula editor is
      CodeMirror 6 with the in-repo Lua personality, and syntax
      diagnostics come from `stage=check`.
- [x] 0.11 Confirm the chart locks: series-first roles with the combo
      as a first-class case, all four scopes in v1, a bespoke chart
      panel rather than lookup 054 rows, and Phase 57 as the chart
      design lock.
- [x] 0.12 Confirm the library homes. Each port lives in its own
      repository with its tests and porting notes, the seed packet
      records the commit or version, and LuaRocks is a possibility
      only if the sandbox can read `package.path`.
- [x] 0.13 Confirm image elements carry `dpi` and `format`. The
      author confirms the stored pixel size at embed time. The
      original file is not kept. A repeated image stores once per
      distinct byte string.
- [x] 0.14 Confirm the phase order: paging, then the system and
       page items, then HTML and PDF, then formulas, totals, and
       CSV, then images, barcodes, and charts, then the native PDF
       objects.
- [x] 0.15 Confirm the image source model: embedded, reference, and
       url are all v1, resolved at the start of generation, with the
       definition's base64 registry deduplicated by content hash.
- [x] 0.16 The effort column matches the phase index. The native PDF
       work is Phases 68, 69, 70, and 71, one sitting each.
- [x] 0.17 The 2026-10-10 amendments are applied to this file and
      match the approval.

### Done means

The locks in this file are the ones Andrew approved.

### Exit gate

Andrew's explicit approval of Phase 0. No source edited.

### Status

**complete — approved 2026-10-10.** Andrew approved the locks in this
file. No source was edited. This close-out records that approval. It
does not reopen it.

### Accomplished

The locks above are the approved contract.

- One lookup family per migration. Object types are Phase 11, fonts
  are Phase 12, and page sizes are Phase 13.
- Page Number is lookup 053 key 61. Page Count is key 62.
  Parameter is key 63.
- A value has a format mask. Null is blank. Align and font are
  honored. Text clips to its box. A group header stays with its
  first detail row. Page bands see the open group and the rows on
  the page. A datasource may name a QueryRef and the report's
  parameters.
- `align` is `Left`, `Center`, or `Right`. Font style is `Normal`,
  `Bold`, `Italic`, or `Bold + Italic`. Lookup 054 keys 9–12 stay
  off the v1 attribute lists. Compose resolves rows, formats text,
  paginates, then fills page fields. A query datasource reads
  `queries.code` by `query_ref` and runs `H.altquery_sync` on
  `params._hydrogen.database`. The mask table in Value format is
  the oracle.
- Script stages are `pdf` (default), `html`, `svg`, `csv`, `compose`,
  and `check`. PNG is a manager step. Native PDF objects are
  Phases 68–71.
- The Defaults table is the omitted-key contract. The panel shows
  the effective value. New writes the stored defaults into the file.
- Compose uses one generation instant. A fixture sends `params.now`.
  A preview omits it and compose uses `H.system.now()`.
- A file load and a paste both go through the embed panel. Each
  stored side is at least 1 pixel. Photographs are JPEG or PNG.
  `image_scale` can return an XO payload, and this plan does not
  use it.

### Lessons learned

- The retired designer is real: page, bands, SVG dot grid, rulers
  with a mouse line, item drag, property grid, JSON undo. Persistence
  to `reports` was not found. Pascal is not on disk.
- Lookup 053/054/057 are rich and uneven. Attribute lists on most
  types omit the field and the font. Page sizes stop at two rows and
  the second is mislabeled.
- PDF and barcodes are pure Lua ports Andrew already has
  (lua-pdfkit, lua-zint). Hydrogen require is `group.script` with
  the split on the first dot. Image sizing is the existing
  ImageMagick endpoint. That endpoint can return an XO payload.
  This plan does not use it. Photographs are JPEG or PNG.
  The script result cap is 1 MiB.
- Author formulas are source in the definition, compiled with
  `load` into a small environment. `require` remains the loader for
  lua-pdfkit and lua-zint. Built-in aggregates are engine ops.
  `avg` and `mean` are the arithmetic mean.
- A close-out that names only Status and the Working Log drops what
  the next session needs. Accomplished, Lessons learned, and Handoff
  are written before the phase is marked complete. The repo copy is
  [`/docs/PLANNING.md`](/docs/PLANNING.md).

### Handoff

Phase 1 is next, and only when Andrew pulls reporting in. CATCHUP
stays P0 until then. Re-check the next free Acuranzo file, QueryRef,
and lookup id on disk before numbering a packet. Hand the packet
over. Do not apply it. Phase 68 still asks before a native PDF
payload field is pinned. A fixture that needs a fixed clock sends
`params.now`. A preview omits it.

---

## Later, not scheduled

Pull one of these into a numbered phase only by amending this file.

| Topic | Why it waits |
| --- | --- |
| Memos and shapes | Each is a `typeKey` with its own box behavior. Images are Phases 50–52. |
| Other barcode keys | Phase 54 places lookup 053 keys 43 and 45. Aggregate, calculated, and crosstab barcode keys stay hidden. |
| Chart kinds beyond the v1 set | Pie is in v1. Phase 63 extends the port. Anything past bar, stacked bar, line, dual-axis, combos, and pie is a port extension plus a panel pass. |
| Chart interactivity | Hover, zoom, and tooltips need scripted SVG, and the exports are static files. |
| Crosstabs | A second layout engine. |
| Subreports and packages | Nested definitions and their own datasources. |
| Multiple group levels | v1 is one level. The key is a field, or one Lua function from Phase 45. |
| Related datasources | v1 is one list. A link between two lists is a new contract. |
| Stored blobs past the caps | A datasource over the params cap, or a PDF whose base64 JSON exceeds 1 MiB. The params cap is operator config, up to the 10 MiB POST ceiling. The result cap is the compile-time 1 MiB scoreboard ceiling. `H.set_result(type, location)` stores two strings and needs a store Hydrogen does not have. The gcode upload endpoint is not that store. |
| Parameter form | Declarations and the run strip are in this plan. Saved presets, lookup-driven pickers, and validation beyond type wait here. |
| Native XLS | CSV is Phase 49. A spreadsheet writer waits until that path is the boring one. The retired client loaded SheetJS in the browser. |
| Thumbnails | `thumbnail` stays null until a preview snapshot has a home. |
| Text measurement and stretching bands | v1 clips to the item box, on the canvas and in emit. Growing a band needs a font engine shared by compose and the designer. |
| Vanadium in PDF | The v1 PDF face is Helvetica. Embedding the product face needs the font bytes inside the sandbox, with `io` left off. |
| Mail and scheduled runs | Mail Relay and the script scheduler. `Reports.Render` stays invokable. Scheduling is a separate Lithium module covering reports, ad hoc queries, and emails. This plan has no scheduler UI and no `scripts` schedule row. |
| Report permissions and sharing | Who can generate or view which report. Hydrogen JWT auth exists; a row-level ACL model is not defined here. This waits on the permissions pass; the report writer does not block on it. |
| Text search and filtering of report output | Full-text search over rendered HTML or PDF. Which rows come back stays on the query. |
| Incremental and delta reports | "Rows changed since the last run." Needs a watermark column on the datasource and a "last run" marker. v1 is a full snapshot. |
| Conditional formatting | Text color, background, or font overrides driven by a field value or a formula. A style rule is a condition plus a set of property changes. v1 renders unstyled values. |
| Data filtering | The query's parameters decide which rows come back. A report-side filter expression stays here. v1 renders every row it was given. |
| Detail row sorting | The query owns order. v1 also sorts one group by its key. A designer sort of the detail rows stays here. |
| Property panel icon chooser | An icon editor for `icon` values. Phase 14 shows icons; choosing one waits. |
| Lithium-wide zoom | Named Ctrl+wheel zooms per surface, after Pavillion's Zooms lock, with a zoom page in the User Profile. The property panel is one target. |
| LithiumProperties in other managers | Styles and others reuse the component after Phase 14b. |
| Row translations | The resolver ships in Phase 14. Filling `lang` on lookup rows, and lookup 047's `en-US` seed bug, wait here. |
| Template inheritance | A base definition that another definition extends, overriding bands or items by id. v1 definitions are standalone. |

## Working log

### 2026-10-10 — Plan locked

Phase 0 is approved. The locks in this file are the contract. Phase 1
is next when Andrew pulls reporting in. CATCHUP stays P0 until then.

### 2026-10-30 — Milestone files

Andrew approved splitting the phase bodies into seven files under
[`/docs/Li/plans/reporting/`](/docs/Li/plans/reporting/). Phase numbers
stayed 0 through 71. The locks, Phase 0, the phase index, the backlog,
and this log stayed in this file. Two sentences that said "this file"
now name this file, because the phase that contains them moved:
Phase 17's column lock, and Phase 57's chart contract. The formula
editor lock names `src/init/codemirror-init.js` without a link,
because that file is gone. Phase 1 is next, in `01-designer.md`.

### 2026-10-30 — Sitting rules

Andrew set the launch operating model. Each phase is a new chat
(`cdli && grok`). The Sitting section in this file is that model.
Tests land with the work. He applies migrations. He checks
`https://lithium.500courses.com`. Sidequests stay unnumbered. The
session writes Status, Accomplished, Lessons learned, and Handoff,
then stops. Opening the chat approves that one phase. The Priority
band now says this plan is the open implementation. Phase 0's
handoff still says CATCHUP stays P0. That sentence is the
2026-10-10 close-out. Phase 1 has not started.

### 2026-10-30 — Phase 1 fixtures

Phase 1 is in progress. The contract note is
[`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md). Ten JSON files
are in
[`/elements/003-lithium/tests/fixtures/reporting/`](/elements/003-lithium/tests/fixtures/reporting/).
`npm test` exited 0: 59 files, 1216 passed. Andrew has not yet read
the note. The intermediate keys pinned there are `pages`, `bands`,
`items`, and a sibling `log`. Phase 2 waits.

### 2026-10-30 — Phase 1 accepted, Phase 2 complete

Andrew accepted the contract note and said to continue. Phase 1 is
complete. Phase 2 landed
[`/elements/003-lithium/src/managers/reports/model/units.js`](/elements/003-lithium/src/managers/reports/model/units.js)
and
[`/elements/003-lithium/tests/unit/reports-units.test.js`](/elements/003-lithium/tests/unit/reports-units.test.js).
One inch is 96 CSS pixels. The three fixture pages measure `8 in` by
`10.5 in` (768 by 1008 CSS pixels). An omitted margin is `0.25 in`.
`npm test` exited 0: 60 files, 1223 passed. `npm run lint` exited 0.
`npx eslint` on `units.js` also exited 0. Phase 3 is next.

### 2026-10-31 — Phase 3 complete

Phase 3 landed
[`/elements/003-lithium/src/managers/reports/model/document.js`](/elements/003-lithium/src/managers/reports/model/document.js)
and
[`/elements/003-lithium/src/managers/reports/model/history.js`](/elements/003-lithium/src/managers/reports/model/history.js).
Import keeps omitted keys. `createBlank` writes Letter, the grid,
the font, and bands `ph`, `detail`, and `pf`. Detail has no
datasource. Generated ids are `b1` and `i1`. Undo restores the
detail fixture after a field move, with the same item id. A new
commit drops the redo tail. `convertLength` is on `units.js`.
`npm test` exited 0: 62 files, 1238 passed. `npm run lint` exited 0.
`npx eslint` on the three model files also exited 0. Phase 4 is next.

### 2026-10-31 — Phase 4 in progress

The Report Manager shell replaces the placeholder. New calls
`createBlank()`. Import reads a local JSON file and shows the
name. A bad file toasts the model message and leaves the open
document. Export downloads `<name>.report.json`. An omitted key
stays omitted. `destroy` and `cleanup` release the same listeners.
[`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
names Reports as id 24.
`npm test` exited 0: 63 files, 1254 passed. `npm run lint`,
`npm run lint:css`, and `templates:copy` exited 0. `npx eslint`
on the shell files exited 0. Work item 4.6 is waiting on Andrew
at `https://lithium.500courses.com`. Do not start Phase 5.

### 2026-10-31 — Phase 4 stage list

Andrew opened Report Manager. The log shows manager 24 loaded and
`[Reports] New report` twice. The stage was empty, so New looked
idle. `#reports-stage` now lists the open definition. A blank
report shows Letter portrait size 0, margins `0.25 in`, grid
`dots, 0.125 in`, font Vanadium Sans `10 pt` `Normal`, and bands
`ph`, `detail`, and `pf`. Import of the detail fixture shows
`Orders`, `orders`, and `customer`. New restores the blank lines.
`npm test` exited 0: 63 files, 1258 passed. `npm run lint`,
`npm run lint:css`, and `templates:copy` exited 0. `npx eslint`
on the shell files exited 0. Work item 4.6 stays open until the
next deploy. Phase 5 still paints the page. Do not start Phase 5.

### 2026-10-31 — Phase 4 complete

Andrew confirmed the shell on `https://lithium.500courses.com`.
The stage lists the blank definition. Import of the detail
fixture shows `Orders`. New restores the blank lines. Export and
re-import keep the name. A bad file toasts the model message and
the name stays. Work item 4.6 is checked. Phase 4 Status is
complete. Phase 5 is next and has not started.

### 2026-10-31 — Phase 5 in progress

The stage draws one page from the open definition. Unzoomed scale
is 96 CSS pixels per inch. Letter is `816px` by `1056px`. A `4 in`
page is `384px` by `384px`. Fixture margins inset `24px`. An
omitted page stays omitted. A length with no unit toasts and draws
no sheet. `loadReport` is the redraw hook. Bands, the grid, and
rulers are later phases.
`npm test` exited 0: 63 files, 1261 passed. `npm run lint`,
`npm run lint:css`, and `templates:copy` exited 0. `npx eslint`
on the page files exited 0. Work item 5.3 is waiting on Andrew
at `https://lithium.500courses.com`. Do not start Phase 6.

### 2026-10-31 — Phase 5 complete

Andrew said the page works on `https://lithium.500courses.com`.
Letter portrait is larger than the 4 in card. The fixture guides
sit inset 0.25 in. Work item 5.3 is checked. Phase 5 Status is
complete. Phase 6 is next and has not started. The earlier gate
stands: `npm test` exited 0 (63 files, 1261 passed).

### 2026-10-31 — Phase 6 in progress

Each band is a label strip and a body of the stored height, in
array order, inside the margin guides. The strip is chrome. The
resting body height is a CSS variable. `element.style.height` is
the drag preview. Mouseup commits through `resizeBand` and
history. The floor is `0.15 in`. Undo, the toolbar button or
Ctrl+Z, restores the previous height. New and Import reset
history. `npm test` exited 0 (64 files, 1270 passed).
`npm run lint`, `npm run lint:css`, and `templates:copy` exited
0. `npx eslint` on the band files exited 0. Work items 6.1–6.3
are checked. Work item 6.4 is waiting on Andrew at
`https://lithium.500courses.com`. Do not start Phase 7.

### 2026-10-31 — Phase 7 in progress

The test database was offline, so the Phase 6 browser check was
not run. Andrew said to continue. Phase 6 stays in progress.
Each band body draws an SVG pattern: dots, grid, dotted, dashed,
or none. The tile origin is the band's top-left. Spacing follows
the unit string. An omitted grid paints `dots` at `0.125 in`
(12 CSS pixels) and the key stays omitted. A `ResizeObserver`
redraws when the band size changes. `destroy` and `cleanup`
disconnect it. `npm test` exited 0 (65 files, 1266 passed, 12
skipped). The skipped cases are the auth integration file.
`https://lithium.500courses.com` did not answer. `npm run lint`,
`npm run lint:css`, and `templates:copy` exited 0. `npx eslint`
on the grid files exited 0. Work items 7.1–7.3 are checked.
Work item 7.4 is waiting on Andrew. Do not start Phase 8.

### 2026-10-31 — Phase 7 still in progress

The alternate database was up. Andrew reported that a drag, Undo,
and New behave. The export sentence of 6.4 was not in that
report, so Phase 6 stays in progress. He also reported that no
dot pattern was showing. A circle of radius `0.5` centered on
the tile corner is clipped away. The painter now draws a full
dot on each spacing point. The painted radius is at least 1 CSS
pixel. The stored weight stays `0.5`. Redo sits beside Undo.
Ctrl+Shift+Z and Ctrl+Y redo. A new edit drops that tail.
`npm test` exited 0 (65 files, 1266 passed, 12 skipped).
`npm run lint` and `templates:copy` exited 0. `npx eslint` on
the grid files exited 0. Work item 7.4 stays open until he sees
the dots after the next deploy. Do not start Phase 8.

### 2026-10-31 — Phases 6 and 7 complete

Andrew confirmed Undo, Redo, export, import, and the dots on
the alternate database. He asked that the undo and redo keys
be registered with the shortcut list. Ctrl+Z undoes and Ctrl+Y
redoes. Both are registered for manager 24 and run only while
that slot is visible. Ctrl+Shift+Z stays Toggle zoom. Closing
the slot unregisters them. `npm test` exited 0 (66 files,
1269 passed, 12 skipped). `npm run lint` and `templates:copy`
exited 0. Phase 8 has not started.

### 2026-10-31 — Tooltip origin

Andrew saw the Resize band tooltip at the top left of the window.
A tooltip stays hidden when its anchor has no box, and when the
resolved point is within 8 CSS pixels of both window edges. The
visible class is applied after that point is accepted. Removing
the trigger drops the tooltip. `npm test` exited 0 (67 files,
1275 passed, 12 skipped). `npm run lint` exited 0. Phase 8 has
not started. The undo history file shape is still unpinned.

### 2026-10-31 — History diffs on the definition

Undo and redo are diffs on the open definition, under `history`.
`undo` and `redo` are lists of JSON Pointer edits. An empty trail
omits the key. Export and import keep it, so a later save of
`reports.report` keeps it too. Clear asks, then removes `history`
and leaves the current report. `npm test` exited 0 (67 files,
1278 passed, 12 skipped). `npm run lint` and `templates:copy`
exited 0. Phase 8 has not started.

### 2026-10-31 — History stores one list entry

Andrew asked whether a large report would keep a full copy on
every undo step. The trail is diffs. A list that grows or shrinks
stores one `ins` or `del` for that entry. A replaced picture stores
only that picture's previous bytes. Neighboring pictures stay out
of the step. `npm test` exited 0 (67 files, 1279 passed, 12
skipped). `npm run lint` exited 0. Phase 8 has not started.

### 2026-10-31 — History kept on export

Andrew confirmed export and import keep the undo and redo trail.
The contract note and the definition lock now say how later work
keeps that trail: one diff list, no cap, Clear before a production
save, compose and the emitters skip `history`, and a picture
replacement stores that picture's previous bytes. Phase 18 stores
the open definition, trail included. Phase 8 has not started.

### 2026-10-31 — Phase 8 in progress

Top and left rulers sit on the sheet. The origin is the top-left
of the white page. The labeled `1` on an inch ruler is 96 CSS
pixels from that corner. Inches tick at eighths. Centimetres tick
at millimetres. Millimetres label every 10, points every 72, and
pixels every 96. Those three scales are view chrome. The
definition stores no ruler key. An omitted unit paints inches. An
unknown unit toasts and draws the page with no rulers. The mouse
line follows the pointer on the page and hides when the pointer
leaves. `destroy` and `cleanup` disconnect the listeners. Export
of a new report matches `createBlank` and has bands `ph`,
`detail`, and `pf`. A band body of stored height `1 in` matches
the `0`–`1` gap. The label strip and the `0.25 in` margin sit
between the page edge and that body. `npm test` exited 0 (68
files, 1288 passed, 12 skipped). The skipped cases are the auth
integration file. `https://lithium.500courses.com` did not answer.
`npm run lint`, `npm run lint:css`, and `templates:copy` exited
0. `npx eslint` on the report files exited 0. Work items 8.1–8.3
are checked. Work item 8.4 is waiting on Andrew. Do not start
Phase 9.

### 2026-10-31 — Phase 8 rulers shortened

Andrew saw the first rulers and said they work. The bars were
too large. Major, half-step, and minor ticks are three quarters
of the first lengths (21, 13.5, and 7.5 CSS pixels). The outer
blank band loses three quarters of its depth, so the bar is 28
CSS pixels. A horizontal major label sits 2 pixels to the left
of its tick, top aligned with the tick top. A vertical major
label sits 2 pixels above its tick, left aligned with the
tick's outer end. Zero stays unlabeled. Dragging a band edge
inverts the vertical ruler between the old edge and the new
edge. Release clears that span. Right and bottom rulers, and a
corner unit toggle, are named and not built. `npm test` exited
0 (68 files, 1290 passed, 12 skipped). `npm run lint`,
`npm run lint:css`, and `templates:copy` exited 0. `npx eslint`
on the report files exited 0. Work item 8.4 stays open until he
sees the shorter rulers and the drag span. Do not start Phase 9.

### 2026-10-31 — Phase 8 ruler unit cycle

Andrew left the extra edge rulers for later and asked for
the corner to cycle inches, centimetres, and points. The
corner is a button. Each click advances `in`, then `cm`,
then `pt`, then `in`, and both rulers relabel at the same
physical scale. The `1` centimetre mark is `toCssPx('1 cm')`,
about 37.8 CSS pixels. The `72` point mark is 96 CSS pixels.
The file keeps `page.units`. New and Import show the file
unit again. A millimetre or pixel file stays in that unit
until the first click, which starts at inches. The choice
lasts for the open document. Undo and redo of a band edit
keep it. The trail records the band edit. Export of a new
report still matches `createBlank`. Right and bottom rulers
stay later. `npm test` exited 0 (68 files, 1292 passed, 12
skipped). The skipped cases are the auth integration file.
`https://lithium.500courses.com` did not answer. `npm run
lint`, `npm run lint:css`, and `templates:copy` exited 0.
`npx eslint` on the report files exited 0.
`reports-rulers.test.js` has 9 tests.
`reports-shell.test.js` has 34 tests. Work item 8.4 stays
open until he sees the shorter rulers, the drag span, and
the corner cycle. Do not start Phase 9.

### 2026-10-31 — Phase 8 corner label

Andrew said the corner cycles and the unit name was missing.
The button inherited `--text-primary` (`#E0E0E0`) onto the
ruler face (`#ececec`). The letters are now `#2a2a2a`, the
same ink as the ticks. Work item 8.4 stays open. Do not start
Phase 9.

### 2026-10-31 — Phase 8 complete

Andrew said he likes the unit name and asked if the phase is
wrapped up. He had said the first rulers work, accepted the
28 CSS pixel bar, and said the corner cycles. Work items
8.1–8.4 are checked. Phase 8 is complete. Right and bottom
rulers stay later. `npm test` exited 0 (68 files, 1292
passed, 12 skipped). The skipped cases are the auth
integration file. `https://lithium.500courses.com` did not
answer that run. `npm run lint` and `npm run lint:css`
exited 0. Phase 9 has not started.

### 2026-10-31 — Phase 9 in progress

`--reports-scale` on the canvas scales the page, the grid, and
the rulers together. 100% is 96 CSS pixels per inch. The 1 inch
tick stays at layout `x1=96`. A Letter page stays `816px` wide.
Fit-width is `floor(100 * paneWidth / sheetWidth) / 100`. The
sheet is the page plus the 28 pixel bar, `844`. A `1000` pixel
pane returns `1.18`. A `600` pixel pane returns `0.71`. Zoom
steps by `0.1` and the buttons stop at `0.1` and `4`. Fit width
follows a later resize. 100% and zoom keep the chosen scale.
New and Import fit again. The corner cycle and undo keep the
open scale. The scale stays off the definition, off history,
and off settings. Export of a zoomed blank report matches
`createBlank()`. The mouse line and a band drag divide the
pointer by the scale. Fit uses the pane width. A shorter pane
scrolls vertically. `npm test` exited 0 (69 files, 1300 passed,
12 skipped). The skipped cases are the auth integration file.
The server check for `https://lithium.500courses.com` was
aborted. `npm run lint`, `npm run lint:css`, and `templates:copy`
exited 0. `templates:copy` copied 137 templates. `npx eslint` on
the report files exited 0. `reports-scale.test.js` has 5 tests.
`reports-shell.test.js` has 36 tests. `reports-rulers.test.js`
has 10 tests. Work items 9.1 and 9.2 are checked. Work item 9.3
is waiting on Andrew. Do not start Phase 10.

### 2026-10-31 — Phase 9 wheel step

Andrew asked for the wheel to zoom the report, at five percent
per bump, and for the buttons to use that same step. Scroll up
over `#reports-page` zooms in. That wheel does not scroll the
stage. A wheel on the summary, on the rulers, or with no
vertical travel still scrolls. The step is `0.05`. From `1.18`
one zoom in is `1.23`. The buttons still stop at `0.1` and `4`.
`npm test` exited 0 (69 files, 1301 passed, 12 skipped). The
skipped cases are the auth integration file. The server check
for `https://lithium.500courses.com` was aborted. `npm run lint`,
`npm run lint:css`, and `npx eslint` on the report files exited
0. `reports-shell.test.js` has 37 tests. Work item 9.3 stays
open. Do not start Phase 10.

### 2026-10-31 — Phase 9 complete

Andrew said the view scale works pretty well and asked if
that covers Phase 9. Work items 9.1–9.3 are checked. Fit
width, 100%, the zoom buttons, and the wheel over the page
are the phase. The step is five percent. The scale stays
off the definition. Phase 9 is complete. Phase 10 has not
started. It places a static text item. The earlier gate
stands: `npm test` exited 0 (69 files, 1301 passed, 12
skipped).

### 2026-10-31 — Phase 10 in progress

Text adds a static item on the selected band. The box is
`1.5 in` by `0.2 in`, centered, `align` `Left`, `text`
`Text`. Font and mask stay omitted. Letter detail places
it at `3.25 in`, `0.025 in`. Drag moves `left` and `top`.
The bottom-right corner changes `width` and `height` and
keeps the top left. Pointerup commits through `moveItem`
and `resizeItem`, which clamp to the band. `addItem` and
import leave the box as written. A resize of `0.3 in` at
top `0.025 in` on a `0.25 in` detail band stores
`0.225 in`. The resize floor is 1 CSS pixel, stored
`0.010417 in`. Delete is the toolbar and the Delete key
for manager 24. Undo restores the item and selects it
again. The canvas clips the text to the box. The ink is
`#2a2a2a`. The band height stays stored. Emit clip stays
with Phase 24. Only `typeKey` 0 is painted. `npm test`
exited 0 (70 files, 1309 passed, 12 skipped). The skipped
cases are the auth integration file. The Hydrogen server
at `https://lithium.500courses.com` was not available.
`npm run lint`, `npm run lint:css`, and `templates:copy`
exited 0. `templates:copy` copied 137 templates.
`npx eslint` on `src/managers/reports` and the two report
unit files exited 0. `reports-items.test.js` has 7 tests.
`reports-document.test.js` has 10 tests. Work items
10.1–10.4 and 10.6 are checked. Work item 10.5 is waiting
on Andrew. Do not start Phase 11.

### 2026-10-31 — Phase 10 complete

Andrew said static text works pretty well and asked to
close Phase 10. Work items 10.1–10.6 are checked. Text on
Detail adds, drags, resizes from the corner, undoes, and
exports. The file has `typeKey` 0, `text` "Text", `align`
`Left`, and `1.5 in` by `0.2 in`. Letter detail places it
at `3.25 in`, `0.025 in`. Font and mask stay omitted.
Delete removes the item and Undo brings it back. A long
string is cut off inside the box. The earlier gate stands:
`npm test` exited 0 (70 files, 1309 passed, 12 skipped).
Phase 11 has not started.

### 2026-10-31 — Manager chrome

Andrew asked for the Report Manager shell before the next
phase. The lock is Manager chrome, under Designer rules.
The running shell is unchanged. The left pane is a
LithiumTable of `reports`, with navigator add, duplicate,
and delete. The right pane has Editor and JSON pages.
Editor keeps the current stage. JSON is a CodeMirror
buffer of the definition with `history` removed, folds
collapsed on open, and a commit on blur. Undo, Redo, and
Clear stay on the main toolbar and use that trail. Zoom
and page size sit at the right end of the main toolbar
and show only on Editor. The second toolbar, Editor only,
switches category and holds Delete and Clone. The third
toolbar, Editor only, holds the item buttons. Static
shows Text. Save and Cancel are the slot footer through
`ManagerEditHelper`. The table QueryRefs stay Phase 17
and Phase 18. The property panel is the Editor column in
Phase 14. Build the chrome as a sidequest before Phase
14. No new phase number. Phase 11 has not started.

### 2026-10-31 — Phase 10-UI in progress

Andrew named the manager chrome Phase 10-UI and asked to build
it. The count stays seventy-two phases, numbered 0 through 71.
Phase 10-UI sits between 10 and 11. The left pane is a
LithiumTable of `reports` with no QueryRef. Name and summary
are the visible columns. Save, duplicate, and delete of a row
toast and do not store or remove a row. Navigator Add calls
`newReport()`. There is no New button. The right pane has
Editor and JSON. Editor keeps the stage, the rulers, and the
grid. Zoom sits at the right end of the main toolbar and shows
only on Editor. The page-size control is absent. Undo, Redo,
Clear, Import, and Export stay on the main toolbar. The second
toolbar, Editor only, is Static, Delete, and Clone. Clone
copies the selected item onto the same band, assigns a new id,
keeps the box, and selects the copy. The third toolbar, Editor
only, is Text. JSON is CodeMirror. The buffer omits `history`,
folds start collapsed, and a parsed edit commits one history
step on blur or when the page changes. A parse error toasts
once and leaves the definition. The property panel is absent.
`npm test` exited 0 (71 files, 1318 passed, 12 skipped). The
skipped cases are the auth integration file.
`npm run lint`, `npm run lint:css`, and `npx eslint` on
`src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `reports.js` is 857
lines. `reports-pointer.js` is 220. `reports-json.js` is 195.
`reports-table.js` is 225. `reports-chrome.test.js` has 8
tests. `reports-document.test.js` has 11. Status stays in
progress. The check is
`https://lithium.500courses.com`. Do not mark Phase 10-UI
complete. Do not start Phase 11.

### 2026-10-31 — Phase 10-UI finishing touches

Andrew asked for the toolbar moves, a Report page, editable
list fields, and the wheel modifier. Undo, Redo, and Clear
moved to the Editor toolbar. Clone then Delete moved onto
the item toolbar with Text. Report is the first tab. It
edits definition `name` and `title`, and the row `summary`.
There is no group column. Name and summary cells use `input`
and `textarea`. The local coltypes had omitted those editors,
so a cell never opened. Save, duplicate, and delete of a row
still toast and do not store or remove a row. The wheel over
the page zooms only while Ctrl or Cmd is held. A bare wheel
scrolls. Overlapping clones stay on the same box. `npm test`
exited 0 (71 files, 1319 passed, 12 skipped). The skipped
cases are the auth integration file. `npm run lint`,
`npm run lint:css`, and `npx eslint` on
`src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `reports.js` is 956
lines. `reports-table.js` is 263. `reports-chrome.test.js`
has 9 tests. Status stays in progress. The check is
`https://lithium.500courses.com`. Do not mark Phase 10-UI
complete. Do not start Phase 11.

### 2026-10-31 — Report QueryRefs handed over

There was no reports list, search, get, or insert QueryRef.
`acuranzo_1015.lua` only creates the table. Scripts 089 and 090
are the script list and search. The packet is
`acuranzo_1422.lua` through `acuranzo_1426.lua`, QueryRefs
165 List Reports, 166 Search Reports, 167 Get Report,
168 Insert Report Revision, and 169 Get Document Image.
`TYPE_SQL`. Reverse deletes that row only. Not applied.
The manager is not pointed at them, so Save still refuses.
The toolbar name span is gone. The Report tab name field
remains. `npm test` exited 0 (71 files, 1319 passed, 12
skipped). `npm run lint`, `npm run lint:css`, and `npx eslint`
on `src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `luacheck` on the five
files exited 0. `get_migration.sh` expanded them on
postgresql, mysql, mariadb, sqlite, db2, firebird, and mssql
with no unsubstituted macros. Each parameter name is used
once. Phase 17 stays in progress until the apply. Phase 10-UI
stays in progress. Do not mark either complete. Do not start
Phase 11. The next Acuranzo file is 1427. The next
caller-facing QueryRef is 170.

### 2026-10-31 — Report list wired

Andrew reported `acuranzo_1422.lua` through `acuranzo_1426.lua`
applied. Phase 17 is complete. QueryRefs 165 List Reports,
166 Search Reports, 167 Get Report, 168 Insert Report
Revision, and 169 Get Document Image are the applied numbers.
He asked to wire the manager. Phase 16 has not started.
The list calls 165, search calls 166, open calls 167, and
save calls 168. `REPORT_ID` 0 allocates a report and stores
revision 1. Any other id appends the next revision. The
returned ids land on the list row. The saved JSON is the
open definition, history included when the trail is
non-empty. Clear omits `history`. A failed insert toasts
the conduit error and leaves the row dirty. Duplicate and
delete still have no query. QueryRef 169 is not called.
`npm test` exited 0 (72 files, 1325 passed, 12 skipped).
`npm run lint` and `npm run lint:css` exited 0. `npx eslint`
on `src/managers/reports` and the report unit tests exited 0.
`reports-save.test.js` has 6 tests. `reports.js` is 965
lines. `reports-save.js` is 250. `reports-table.js` is 267.
Phase 18 stays in progress until the save check on
`https://lithium.500courses.com`. Phase 10-UI stays in
progress. Do not mark either complete. Do not start
Phase 11. Do not start Phase 19. The next Acuranzo file
is 1427. The next caller-facing QueryRef is 170.

### 2026-10-31 — Delete Report handed over

There was no reports delete. `acuranzo_1422.lua` through
`acuranzo_1426.lua` list, search, get, insert, and read a
document image. `acuranzo_1427.lua` is QueryRef 170, Delete
Report, `TYPE_SQL`. One integer `REPORT_ID`, used once.
The statement deletes every revision of that `report_id`,
so the report leaves the list. Deleting only the open
`rev_id` would put the previous revision back on the list.
Document rows stay. Reverse deletes the QueryRef
`TYPE_SQL` row only. Not applied. The manager is not
pointed at it, so list Delete still refuses. Canvas item
Delete is unchanged. `luacheck` on the file exited 0.
`get_migration.sh` expanded it on postgresql, mysql,
mariadb, sqlite, db2, firebird, and mssql with no
unsubstituted macros. MySQL and MariaDB cast the id as
`signed`. MSSQL casts it as `INT`. The others cast it as integer. Phase 18
stays in progress. Phase 10-UI stays in progress. Do not
mark either complete. Do not start Phase 11. Do not start
Phase 16. Do not start Phase 19. The next Acuranzo file
is 1428. The next caller-facing QueryRef is 171.

### 2026-10-31 — List delete, category, and the designer punch list

Andrew said save is kind of working and the report contents
are remembered. That is a partial check of work item 18.4.
He then asked for seven follow-ups.

List Delete calls QueryRef 170. Confirm says every revision
is removed. A saved id sends `INTEGER.REPORT_ID` once. A
draft with no id is removed locally. A failed call keeps
the row. Canvas item Delete is unchanged and does not
confirm.

Category is a nullable `reports` column. It is not a
definition key, and Tabulator `groupBy` stays off. The list
shows it after name. Save sends `STRING.CATEGORY`.
[`acuranzo_1428.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1428.lua)
adds the column.
[`acuranzo_1429.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1429.lua)
through
[`acuranzo_1432.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1432.lua)
rewrite QueryRefs 165–168 and do not take a new caller ref.
They are not applied. A typed category does not survive a
reload until those files are applied, the payload is
rebuilt, and Hydrogen is restarted. Deploy this SPA before
1432, or together with it, because `:CATEGORY` becomes
required. Search still matches `UPPER(name)` and uses
`:SEARCH` once.

Reports imports the Tabulator stylesheet with the manager.
The three right-pane toolbars sit 1px apart. The Text
button stays. Dragging it onto a band drops the same
1.5 in by 0.2 in rectangle, centered on the pointer, then
clamped to the band. Moving or resizing an item highlights
that box on both rulers, and the crosshair keeps moving
for the drag. The designer stays read-only until the row
enters edit mode. An edit marks the row dirty. Opening
another row does not.

`npm test` exited 0 (72 files, 1331 passed, 12 skipped).
The skipped cases are the auth integration file.
`npm run lint`, `npm run lint:css`, and `npx eslint` on
`src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `luacheck` on
1428–1432 exited 0. `get_migration.sh` expanded them on
postgresql, mysql, mariadb, sqlite, db2, firebird, and
mssql. The inner SQL was peeled. Each named parameter is
used once. PostgreSQL casts `:CATEGORY` as `text` after
`:NAME`. MySQL and MariaDB cast it as `char(255)`.
`reports.js` is 988 lines. `reports-save.js` is 278.
`reports-table.js` is 328. `reports-save.test.js` has 8
tests. `reports-chrome.test.js` has 10.
`reports-items.test.js` has 10. The Acuranzo index is
357 files and 1906 statements.

Phase 18 stays in progress. Work item 18.4 is still his
save, reload, and `rev_id` check. Phase 10-UI stays in
progress. Do not mark either complete. Do not start
Phase 11. Do not start Phase 16. Do not start Phase 19.
The next Acuranzo file is 1433. The next caller-facing
QueryRef is 171. Overlapping clones are still waiting on
his choice. The site check was not run from here.

### 2026-10-31 — JSON edits follow the row lock

Andrew asked whether a revisions browser was already a phase,
and for JSON edits to follow the same edit lock. Phase 18
saves a new revision and opens the latest one. It does not
list older revisions, name who saved them, copy one to a new
report, or drop newer revisions. That browser is not started.

The JSON page stays read-only until the row is in edit mode.
A parsed change in the buffer marks the row dirty, so Save
can enable while that page is open. Leaving JSON for the
Editor commits the buffer as one history step and redraws
the page. Undo returns the previous definition. A buffer
that does not parse leaves the definition and toasts once.
`npm test` exited 0 (72 files, 1332 passed, 12 skipped).
The skipped cases are the auth integration file.
`npx eslint` on the touched report files exited 0.
`reports.js` is 989 lines. `reports-json.js` is 228.
`reports-table.js` is 332. `reports-chrome.test.js` has 11
tests. Phase 18 stays in progress. Phase 10-UI stays in
progress. Do not mark either complete. Do not start
Phase 11. Do not start Phase 16. Do not start Phase 19.

### 2026-10-31 — Revisions tab

Andrew agreed Phase 18 had been brought forward, ahead of
Phase 16, and asked for the Revisions tab in that work.

The fourth page is Revisions. Editor still opens. Report
stays first. A saved report lists its revisions through
QueryRef 171: the revision number, when it was saved, and
who saved it. Choosing a row does not load the canvas. A
draft says to save first and does not call the server.
Copy confirms, loads that revision through 167, and inserts
it as a new report through 168 with `REPORT_ID` 0. The new
list row is not selected. Revert confirms, loads the body
first, then QueryRef 172 deletes every newer revision and
the kept body is imported. The edit snapshot is taken
again so Save is not dirty. Revert stays disabled on the
newest revision. Both buttons wait for edit mode. QueryRef
170 still deletes every revision of the report and is not
used here.

`acuranzo_1433.lua` rewrites 168 so `created_id` is the
signed-in account. `:USERID` is used once. `updated_id`
stays 0. `acuranzo_1434.lua` is QueryRef 171.
`acuranzo_1435.lua` is QueryRef 172. They are not applied.
Deploy this SPA before 1432 and 1433, or together with
them. Then apply 1428 through 1432, then 1433, then 1434
and 1435 in either order. Reverse 1433 before 1432. A
revision saved before 1433 still shows Unknown. The tab
toasts when 171 is missing.

`npm test` exited 0 (73 files, 1335 passed, 12 skipped).
The skipped cases are the auth integration file.
`npm run lint`, `npm run lint:css`, and `npx eslint` on
`src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `luacheck` on
1433–1435 exited 0. The expand in this sitting peeled each
named parameter once on postgresql, mysql, mariadb,
sqlite, firebird, and mssql. `reports.js` is 982 lines.
`reports-save.js` is 301. `reports-revisions.js` is 398.
`reports-pages.js` is 43. `reports-json.js` is 228.
`reports-chrome.test.js` has 11 tests.
`reports-revisions.test.js` has 3. `reports-save.test.js`
has 8. The Acuranzo index is 360 files and 1918
statements.

Phase 18 stays in progress. Work item 18.4 is still his
save, reload, and `rev_id` check. Work item 18.6 is the
tab. Phase 10-UI stays in progress. Do not mark either
complete. Do not start Phase 11. Do not start Phase 16.
Do not start Phase 19. The next Acuranzo file is 1436.
The next caller-facing QueryRef is 173. Overlapping clones
are still waiting on his choice. The site check was not
run from here.

### 2026-10-31 — Phase 10-UI and Phase 18 closed

Andrew said the changes are working and asked to wrap the
sitting up. Phase 10-UI and Phase 18 are complete.
Work items 10-UI.4 and 18.4 are checked. His check on
`https://lithium.500courses.com` is also the apply report
for `acuranzo_1428.lua` through `acuranzo_1435.lua`. He
did not recite the file numbers. Duplicate still has no
query. QueryRef 169 stays unused until Phase 51. The next
Acuranzo file is 1436. The next caller-facing QueryRef is
173. Phase 16 has not started. Phase 11 has not started.
Do not start Phase 11, Phase 16, or Phase 19 until he
names one. Identical overlapping clones still have no
mark. He did not pick one.

### 2026-11-01 — Band stack sidequest

Andrew stopped Phase 11 before it started. The design surface
was a full Letter sheet. It is now a stack of bands at the
content width. Each band is drawn once at its stored height.
Detail, group, and page bands repeat at compose time, so the
paper height stays in the file and stays off the surface.
There is no page rectangle and no margin guide. The role strip
sits above the measure. The first measured band carries the
only top ruler. It measures the content width from 0. Every
measured band carries a left ruler that starts at 0 at the top
of its body and runs for that body's height. The corner sits
where those two meet and still cycles inches, centimetres, and
points without writing the file. The vertical mouse line reads
the top ruler. The horizontal line reads the band under the
pointer. Resizing a band grows that band and its own left
ruler, and inverts the span from the old height to the new
one. Fit width uses the content width plus the 28 CSS pixel
bar. A 1000 CSS pixel pane around a Letter content box of 768
is 1.25. Ctrl or Cmd plus the wheel over the stack, rulers
included, still zooms. A bare wheel still scrolls. An unknown
page unit draws the bands with no rulers and toasts. Export
gains no ruler band. Phase 8 stays complete. Its close-out
still describes the sheet. This entry amends the live lock.

`npm test` exited 0 (73 files, 1335 passed, 12 skipped).
The skipped cases are the auth integration file.
`npm run lint` and `npm run lint:css` exited 0.
`templates:copy` copied 137 templates.
`reports-shell.test.js` has 37 tests.
`reports-rulers.test.js` has 10. `reports-items.test.js`
has 10. `reports-scale.test.js` has 5. `reports.js` is
984 lines.

Phase 11 has not started. The 500 Courses check is Andrew's.
This sitting does not deploy and does not log in. Do not
start Phase 11, Phase 16, or Phase 19.

### 2026-11-01 — Ruler order, live ticks, and snap

Andrew reviewed the band stack. The top ruler is now the first
row of the first measured band, above that band's descriptor.
The descriptor sits in the content column. A ruler-colored pad
under the corner keeps the left ruler flush with the corner.
That left ruler still starts at 0 at the top of the body, so
the content origin is (0, 0). The page margins stay off the
surface. Growing a band redraws the left-ruler ticks for the
new length while the pointer moves. The highlight still marks
the span. Snap is a button on the item toolbar, immediately
before Clone. It starts on. A band resize and an item move or
resize land on the current grid when the pointer stops. The
default grid is 0.125 in. A light line or ghost shows that
landing during the drag and hides when the pointer is already
on a grid point. The band floor stays 0.15 in, so a nearer
grid point below that floor does not win. Snap off leaves the
pointer landing alone and draws no preview. The choice stays
off the definition. The committed lengths are what history
stores. Phase 8 stays complete. Its close-out still describes
the sheet. This entry amends the live lock.

`npm test` exited 0 (74 files, 1339 passed, 12 skipped).
The skipped cases are the auth integration file.
`npm run lint` and `npm run lint:css` exited 0.
`templates:copy` copied 137 templates.
`reports-snap.test.js` has 4 tests. `reports-shell.test.js`
has 37. `reports-rulers.test.js` has 10. `reports-items.test.js`
has 10. `reports.js` is 987 lines.

Phase 11 has not started. The 500 Courses check is Andrew's.
This sitting does not deploy and does not log in. Do not
start Phase 11, Phase 16, or Phase 19.

### 2026-11-02 — Phase 11 packet

Andrew named Phase 11. He confirmed text is native to the item, so no
new lookup 054 text key. `acuranzo_1436.lua` repairs lookup 053: icon
keys 7 and 16, the key 52 tag, new keys 61 Page Number, 62 Page Count,
63 Parameter, and the v1 attribute lists (position 0–5 and 14, font
15–17, field 7 and datasource 8 off static text). No caller-facing
QueryRef. Phase 11 is in progress until Andrew reports the apply. The
next Acuranzo file is 1437. The next caller-facing QueryRef stays 173.

### 2026-11-02 — Phase 11 closed

Andrew reported `acuranzo_1436.lua` applied. He confirmed lookup 053 in
the Lookups manager: keys 7 and 16 store `icon`, key 52's tag is closed,
keys 61–63 exist, and the v1 attribute lists match. Test 31 and Test 98
pass. Phase 11 is complete. Phase 12 has not started and waits for
Andrew to name it. The next Acuranzo file is 1437. The next
caller-facing QueryRef is 173.

### 2026-11-02 — Phase 12 packet

Andrew named Phase 12 and added Vanadium Mono, so the phase goal now
reads two families, one migration. `acuranzo_1437.lua` updates lookup
054: key 15 lists Vanadium Sans, Vanadium Mono, then the existing faces,
with hint `Default: Vanadium Sans`; key 4 lists Left, Center, Right; key
17 lists the four v1 styles. No caller-facing QueryRef. Phase 12 is in
progress until Andrew reports the apply. The next Acuranzo file is 1438.
The next caller-facing QueryRef stays 173.

### 2026-11-02 — Phase 12 closed

Andrew reported `acuranzo_1437.lua` applied and confirmed lookup 054 in
the Lookups manager. Phase 12 is complete. Phase 13 has not started and
waits for Andrew to name it. The next Acuranzo file is 1438. The next
caller-facing QueryRef is 173. Side fix the same day, outside the
plan: Hydrogen `tests/lib/log_output.sh` 4.7.3 stores the ▇▇ icon glyph
literally, so a C locale no longer prints `\u2587`.

### 2026-11-02 — Phase 13 packet

Wrote `acuranzo_1438.lua` for lookup 057 only. Key 1 is relabeled
`US Letter Landscape`. Keys 2–7 add US Legal and Tabloid in portrait
and landscape (inches, 0.25 in margins) and A4 and A3 portrait (mm,
6 mm margins), per Andrew. `Tabloid` and `A3` Page Size strings are
not yet in lookup 054 key 30. Phase 13 is in progress until Andrew
reports the apply. The next Acuranzo file is 1439. The next
caller-facing QueryRef stays 173.

### 2026-11-02 — Phase 13 closed

Andrew reported `acuranzo_1438.lua` applied and the lookup 057 checks
good. Phase 13 is complete. A4 and A3 landscape were skipped by
Andrew's choice and can come in a later lookup-only file. Phase 14 has
not started and waits for Andrew to name it. The next Acuranzo file is
1439. The next caller-facing QueryRef is 173.

### 2026-11-02 — Property panel weave

With Andrew, amended the plan for the property panel. Added the
Property panel lock: a shared Tabulator `LithiumProperties`
component, sections and icons from lookup data, ids rather than
labels, a `lang` label resolver where the profile language wins,
and Coloris as the one new v1 package. Added labeled Phase 13b
(section lookups 074 and 075 plus a `section` key on 054) and
Phase 14b (favourites, hidden rows, order, remembered state, Date
editor, LITHIUM-PROPS docs). Phase 14 now needs 13b. The icon
chooser, Lithium-wide zoom, reuse in other managers, and row
translations went to Later. No code changed. The next Acuranzo
file is 1439.

### 2026-11-02 — Property panel placement

Andrew placed the property panel left of the canvas and right of the
report table, with its own splitter. The below-canvas summary goes
when the panel ships. Its band and item list becomes the panel's
Outline section (Phase 14.8). Name and title ownership stays open.

### 2026-11-02 — Phase 13b packet

Andrew approved the 074/075 labels, icons, and order, the Structural
row for 074 key 0, and leaving 054 `sort_seq` alone. Re-checked disk:
lookups 074 and 075 and files 1439–1441 were free. Wrote
`acuranzo_1439.lua` (lookup 074 Report Palette Groups: lookup 0
registry row plus keys 0–7, `key_idx` = 053 `value_int`, icon in
`collection`), `acuranzo_1440.lua` (lookup 075 Report Attribute
Sections: registry row plus keys 1–9, the nine 054 `Group` strings),
and `acuranzo_1441.lua` (adds `"section": <075 key>` to 054 keys 0–40,
keeps `Group`; reverse restores the 1087 JSON as amended by 1437).
Lookup 053 and its lowercase `group` are unchanged. Outline stays
hard-coded in the panel. Key 24 stays in Grid. Phase 13b is in
progress until Andrew reports the apply. The next Acuranzo file is
1442. The next caller-facing QueryRef stays 173.

### 2026-11-02 — Phase 13b closed

Andrew reported `acuranzo_1441.lua` applied, the last of the 1439–1441
packet. Phase 13b is complete: lookup 074 (palette groups, keys 0–7
with Structural) and lookup 075 (attribute sections, keys 1–9) exist,
and every 054 row carries a `section` with `Group` kept. 054
`sort_seq`, 053, and its `group` string are unchanged; the Outline
stays hard-coded in the panel. Phase 14 (property panel) is next in a
fresh chat; its entry gate is met. The next Acuranzo file is 1442. The
next caller-facing QueryRef is 173.

### 2026-11-02 — Phase 14 started

Andrew chose to continue into Phase 14 in the same Sitting after 13b
closed. Added `src/properties/sections.js` (section grouping by 054
`section`, `Group` fallback, 075 `sort_seq` order; the `lang` label and
list-caption resolver) with unit tests. No panel UI yet. Four decisions
wait on Andrew: 054 key 24, key 29, key 30, the colour editor for keys
21 and 24, and name/title ownership. No migrations. The next Acuranzo
file is 1442. The next caller-facing QueryRef is 173.

### 2026-11-02 — Phase 14 decisions locked, panel built

Andrew locked five decisions: key 24 is report/page only (Lithium drops
it from item lists); key 29 Link is text; key 30 fills from 057; keys
21 and 24 use the Color editor in Lithium; name and title edit in both
the panel and the Report page form, one Undo step each. Deferred
lookup-only cleanup (not written): drop 24 from the 053 item lists and
set the 21/24 `Editor` to `Color`. Lookups load through QueryRef 26
(the plan's 026). Built `report-lookups.js`, `report-properties.js`,
`reports-panel.js`, and `src/properties/lithium-properties.js`; the
Outline and panel sit left of the canvas and `.reports-summary` is
gone. Added `@melloware/coloris`. 14.1, 14.3, 14.5, 14.8 ticked; 14.2,
14.4, 14.6 partial; 14.7 waits on Andrew's browser check. No
migrations. The next Acuranzo file is 1442. The next caller-facing
QueryRef is 173.

### 2026-11-02 — Property panel chrome pass

Andrew's first look. Fixed the polish-pass regression: the panel CSS
sat outside `@layer managers` and made Tabulator cells block-level
(`display: flex`), so name and value stacked, and visible default
notes read as extra rows. Added a framed panel (top bar with Search,
Outline, Expand all, Collapse all; bottom bar with 14b Favourites and
Hidden stubs), a resizable scrolling Outline, chrome-free editors,
carets, and right-aligned counts. Caret shown; the whole section
header stays clickable. Phase 14 stays in progress on 14.7.

### 2026-11-02 — Property panel second pass

The Expand all and Collapse all buttons only reached the first
section. Each Tabulator `group.show()`/`hide()` rebuilds the groups,
so the rest of one `getGroups()` loop acted on stale groups. Each group
is now looked up again before it is toggled. Bands can now be
selected in both directions: the selection has a band scope
(`_bandScope`, `selectionScope()`), so the Outline highlights a band
row and the panel shows the band's rows (Height until 053 lists more).
Toggles use the shared `.lithium-nav-btn-active` class (Search,
Outline, Grid, Snap). Static is a plain button like Text. Other
changes: a black gutter behind the right splitter, the stage closer to
it, `--space-2` between stacked toolbars, the value column refitted on
resize, and tighter section headers. Phase 14 stays in progress on
14.7.

### 2026-11-02 — Property panel third pass

- **Wide section-header spacing:** caused by the Tabulator theme rule
  `.tabulator-row.tabulator-group span { margin-left: 10px }`, which
  applies to every header span. The panel now overrides it.
- **Name/value split:** a column resize or a container resize now runs
  a full `redraw(true)`, unless a cell is being edited. This refits the
  value column and re-measures rows whose names wrap.
- **Outline divider:** now a 6px bar in the frame colour, and the bar
  is the drag target.
- **Search:** has a manual clear X (the `lithium-header-filter-clear`
  class).
- **Backgrounds and gaps:** the area around the panel uses
  `--bg-secondary` to match the table. The editor gap matches the gap
  around the panel.

Phase 14 is still In progress on 14.7.

### 2026-11-02 — Property panel fourth pass

- **Outline:** rows use `--font-size-us` (9px) with 0px top and bottom
  padding.
- **Colours:** the grid uses LithiumTable's colours. Rows are
  `--bg-row-odd` and `--bg-row-even`. Section headers are
  `--accent-alt-secondary` with a `--bg-row-border` top and bottom
  border and `--text-primary` text.
- **Value width:** fixed. A Tabulator drag changes only the name
  column, so the value column never followed it live. On release,
  `fitColumns` re-split the width by `widthGrow`. The table now uses
  `fitData` with `resizableColumnFit` (live), and `_fitValue` sets
  value = holder `clientWidth` − name width (floor 60) on build,
  release and container resize.

Phase 14 is still In progress on 14.7.

### 2026-11-02 — Name/value split sticks

The split snapped back after a drag. With `fitData`,
`table.redraw(true)` calls `reinitializeColumnWidths`, which puts both
columns back to their definition widths (120 and 160). `_refit` ran
`_fitValue` before the redraw, so the redraw undid it. The dragged name
width is now stored (`_nameWidth`, set on `columnResized`). `_refit`
runs the redraw first, then puts the name width back, then sets
value = holder `clientWidth` − name (floor 60), then calls
`normalizeHeight()` on each row. Phase 14 is still In progress.

Props splitter gutter is now `--bg-primary` with no vertical margin, so it reads black like the left navigator gutter. Hover and drag still use `--accent-primary`.

The gutter now follows Andrew's model. `.reports-editor-split` is `--bg-primary`, and `.reports-props` (its margin is now padding) and `.reports-stage` are `--bg-secondary`. The splitter is transparent with no margin, so the dark band runs the full height between the toolbar and the footer.

### 2026-11-02 — PP chrome done; layout prefs persisted

Andrew says the chrome tweaks are finished ("the UI looks lovely"). The PP
layout is now saved to localStorage, on each change:
- The PP frame width goes through `PanelStateManager('lithium_reports_props')`
  (the same pattern as the list panel).
- The other settings are one JSON blob, `lithium_reports_props_prefs`, holding
  `{ showSearch, showOutline, nameWidth, outlineHeight, closedSections[] }`.
  `LithiumProperties` takes it in as `state` and reports changes through
  `onStateChange`.
- Search text is not saved; no other Lithium filter keeps its text.
- Closed sections are stored by section caption, so the choice holds across
  objects and now also across reloads.

These prefs can count toward 14b or stay in 14. Gate 14.7 looks met, but
Phase 14 stays In progress until Andrew says to close it.

### 2026-11-03 — PP and editor tweak pass (Phase 14 still In progress)

Client-only; no migration. Property panel: Up/Down step the outline;
Tab/Shift+Tab move between value editors and skip read-only rows (the
Object/type row no longer traps Tab); the bottom bar gains right-aligned
Prev/Next (`fa-angle-up`/`fa-angle-down`); expand/collapse/prev/next use
tighter padding; group headers keep full width when every section is closed.
Report editor: an End of Report marker (no height) with its own outline entry
and PP section; the sheet rounds the corners of the first and last elements;
a Ruler toggle beside Grid and Snap (session only); a Ruler section on the
Report with top/left/right/bottom Booleans stored as `doc.rulers` (a side is
on unless `false`); clicking any ruler selects the Report. Vitest 1381 passed
(1372 before, plus 9 new). Phase 14 stays In progress until Andrew says to
close it.

### 2026-11-03 — Rounding, inward ruler ticks, Crosshair toggle (Phase 14 still In progress)

After the round-5 browser check: the editor stage, the report sheet and page
clip, and the PP top/bottom bars now use `--border-radius-sm`, the same
variable `lithium-toolbar.css` uses for the toolbars above them (no new
radii). Right-ruler ticks start at its left edge and bottom-ruler ticks at its
top edge, both facing the page; labels follow. A fourth toggle after Grid,
Snap, and Ruler, labelled **Crosshair** (the name the code already uses for
the `.reports-cursor` lines), shows or hides the cursor guide lines; session
only, like Grid and Ruler. Vitest 1383 passed. Phase 14 stays In progress.

### 2026-11-03 — Right ruler grows during a band drag (Phase 14 still In progress)

While a band edge was dragged, only the left ruler was resized and redrawn,
so the right ruler showed the highlight but no new ticks or numbers past its
old length. `_moveBandDrag` now resizes every side ruler in the band (left and
right). The bottom ruler runs across the page width, which a band drag does
not change, so it was not affected. Vitest 1383 passed.

### 2026-11-03 — Phase 14 complete

Andrew closed Phase 14 ("Lets wrap this phase"). Shipped: the shared
`LithiumProperties` panel; chrome polish and shared toolbar rounding
(`--border-radius-sm`); layout prefs in localStorage; Outline arrow keys,
Tab between editors, Prev/Next; the End of Report marker; rulers on four
sides saved as `doc.rulers` with inward right/bottom ticks, a Ruler toggle,
and ruler click selects the report; a Crosshair toggle; and the band-drag
right ruler fix. No migration; next Acuranzo file 1442, next QueryRef 173.
Unit suite 1383 passed. Accomplished, Lessons learned, and Handoff are in
[`reporting/02-save.md`](/docs/Li/plans/reporting/02-save.md). Next: Phase
14b (property panel preferences), in a fresh chat, when Andrew names it.

### 2026-11-03 — acuranzo_1442 drafted (054 Color, 053 key 24)

Phase 14b closed. Andrew chose to do the deferred lookup cleanup before
Phase 15. Drafted `acuranzo_1442.lua`: 054 keys 21 and 24 get `Editor:
Color`, and 054 key 24 leaves the 053 item `attributes` lists. The
reverse restores 1441/1436 JSON. Waiting for Andrew to apply. After
apply: remove the Lithium special-cases for 21/24 and verify Coloris
opens and saves on keys 21, 24, and 25. The next Acuranzo file is 1443.
The next caller-facing QueryRef is 173.

### 2026-11-03 — acuranzo_1442 applied; Color editor from lookup

Andrew reported it
applied. Lithium now reads the Color editor from 054 `Editor` (the
`COLOR_KEYS` override is gone) and takes item attribute lists from 053
as-is (`REPORT_ONLY_KEYS` is gone). Coloris fix: the picker opens on
click, not focus, so the Color editor now clicks its new input when it
renders, and it commits on Coloris `close` as well as `change`, so
closing without a change no longer leaves the cell stuck in edit.
A chrome test checks the picker opens and the value saves once.
`npm test`: 81 files, 1398 passed, 12 skipped. ESLint at baseline (163
src, 8 tests). Stylelint clean. Browser check for keys 21, 24, and 25
waits for Andrew. Phase 15 not started. The next Acuranzo file is 1443.
The next caller-facing QueryRef is 173.

### 2026-11-03 — Retained colour swatches on the Coloris picker

Retained colour swatches (Andrew: "swatches shown on all instances
of the color picker", after Pavillion). New `src/shared/coloris-picker.js`
is Lithium's one Coloris entry, ported from Pavillion's
`ui/src/common/coloris.js`: swatches are the theme's
`--lithium-picker-swatches` list (none defined yet) then the user's own,
capped at 24, saved on the profile at `colorPicker.swatches`. Under the
swatches a row holds + (add the current colour), − (remove a colour the
user added) and ✕ (Coloris's close, moved there). Round swatches, Hex /
RGB / HSL toggle. `src/styles/vendor-coloris.css` holds the styles. The
property panel's Color editor now goes through it. New
`coloris-picker.test.js` (5 tests). `npm test`: 82 files, 1403 passed,
12 skipped. ESLint at baseline (163 src, 8 tests). Stylelint clean.
Pavillion's Ctrl+wheel picker zoom is not ported. Phase 15 not started.
No migration; next Acuranzo file 1443.

### 2026-11-03 — Coloris swatches wrapped; Phase 15 started

Coloris swatches and polish are done (Andrew: "ok for now"). Fixes after
his browser check: Coloris's own wrapping layout restored so the preview
circle and value field share a row; + / − / ✕ white when enabled and
#9a9a9a when disabled; value field at 12px so rgb()/hsl() fit.

Phase 15 is In progress. First slice: `src/managers/reports/reports-sample.js`
(`parseSample` gives rows and union-of-keys columns, `datasourceNames`,
`fieldItemLabel`, `unboundFields`, `FIELD_TYPES` 20/21/24) and
`tests/unit/reports-sample.test.js` (6 tests). `addItem` already gives
mask types an empty `mask`. Next: Field category on the second toolbar
with Text / Number / Datetime on the third, a sample-data import, canvas
paint of field labels, the Field editor list, and the export refusal.
ESLint at baseline (163 src, 8 tests).

### 2026-11-03 — Phase 15 code complete; browser check pending

Field category on the second toolbar; Text / Number / Datetime fields,
Sample import and Paste on the third. Session-only sample (never in the
definition or export) feeds the panel's Field list; Datasource lists
`datasources`. Fields paint their bound name, or a grey italic caption
when unbound. Export refuses unbound fields with a toast. Mask is
editable on mask types. New `reports-fields.js` mixin. 84 files, 1413
passed, 12 skipped; ESLint baseline; stylelint clean. 15.1–15.3 and 15.5
checked; 15.4 (browser) waits for Andrew. Phase stays In progress.


### 2026-11-03 — Phase 15 follow-ups: group toggle, band floor, audit draft

Element group buttons (Static / Field) are radio-style and lit with
`--accent-toggle` when chosen. The choice is saved on the profile at
`reports.category` (default Static) and restored on load. `resizeBand`
now refuses to go below the bottom of the band's lowest item
(`bandContentBottom`), for both the drag and the panel Height row.
Reports footer audit info needs QueryRef #165 to return the audit
columns: draft `acuranzo_1443.lua` written, waiting for Andrew to apply;
the footer is not wired yet. 85 files, 1417 passed, 12 skipped; ESLint
baseline; stylelint clean. Phase stays In progress.

2026-11-03 — Andrew applied `acuranzo_1443.lua`. Reports footer now
wires `AuditInfoFooter` after save/cancel (Query Manager pattern), fed
through `reportAuditRow`: `updated_*` when set, else `created_*`. Row
selection and save both refresh it. 84 files, 1418 passed; ESLint
baseline; stylelint clean on src. Phase 15 stays In progress; browser
check is Andrew's.

2026-11-03 — Phase 15 complete. Andrew checked fields, the sample
datasource, the band content floor, and the audit footer in the
browser: "Everything looks good. I think we're ready to wrap Phase
15." 15.4 passes. Phase 16 (one group level) is next and has not
started.


### 2026-11-03 — Flatpickr-ce parked on Phase 22

Andrew: park the Flatpickr community-edition swap on the date-work
phase, not as a Phase 16 sidequest. Note sits under Phase 22 in
[`03-preview.md`](/docs/Li/plans/reporting/03-preview.md): alias
`"flatpickr": "npm:flatpickr-ce@4.9.0"` (exact pin), smoke
FlatpickrPicker / Date editor / Profile date-formats when that phase
opens.

**2026-11-03 — Phase 16 started (one group level).** Field toolbar gets
a Group button (`#reports-group-btn`): one click adds a group header
(typeKey 14, `field: ""`) just before Detail and a group footer
(typeKey 16) just after it, both `0.25 in`. The header is selected. A
second click toasts "One group level only" and changes nothing. Model:
`addGroup` / `hasGroup` in `model/document.js`. Property panel: the
group header shows Height plus Field (054 key 7, sample columns when a
sample is loaded, else typed); the footer shows Height only and shares
the header's field. Export keys match `grouped.definition.json` for gh
and gf. New `reports-group.test.js` (3 tests). `npm test`: 87 files,
1424 passed, 12 skipped. ESLint clean on reports + new test. Stylelint
clean. 16.3 browser check waits for Andrew. Phase 16 In progress.

**2026-11-03 — Phase 16 widened (Andrew): bands palette, delete, multi-group.**
New Bands category (`#reports-bands-btn`) beside Static / Field, with
Report Header, Page Header, Group, Detail, Page Footer, Report Footer.
`addBandInOrder` puts each at its canvas rank (rh, ph, gh…, detail,
gf…, pf, rf). Generated ids are short per type (rh1, ph1, gh1, d1, gf1,
pf1, rf1). `b1` is gone. Group no longer refuses a second level: each
new group is innermost (gh1 gh2 detail gf2 gf1). Pairing is by nesting
(`groupPartner`), so no new definition keys, and gh/gf keys still match
`grouped.definition.json`. The band strip shows the group field (the
footer shows its header's) and a trash button at the far right.
`removeBand` deletes a group pair together. The Group button moved
from Field to Bands. `npm test`: 87 files, 1426 passed, 12 skipped.
ESLint clean on reports and the touched tests. The full-tree count is
unchanged at 171 (pre-existing). Stylelint clean. New icons need
`npm run icon:registry` (the build runs it). Browser 16.3 / 16.8 wait
for Andrew.

**2026-11-03 — Phase 16 browser tweaks (Andrew).** The first band strip
spans the ruler column like the others (the headed band hides its ruler
pad). `createBlank` now uses ph1, d1, pf1. `defaultBandId` picks the
first detail by type, not by the id `detail`. Imported files keep their
ids. `canRemoveBand` keeps the last detail: no trash on it, and
`removeBand` throws. The Text / Number / Datetime field buttons are
draggable and drop onto a band under the pointer with an empty binding
(kind `reports-field:<typeKey>`). Bands buttons stay click-only. Tests
that built on `createBlank` moved to the new ids. `npm test`: 87 files,
1427 passed, 12 skipped. ESLint clean on reports and the reports tests.
Stylelint clean. Phase 16 In progress until Andrew's browser check.

**2026-11-03 — Phase 16 complete.** Andrew: "Seems to be working pretty
well. Let's close Phase 16." Groups (multi-level), Bands palette, band
delete with pair and last-detail rules, short ids, and field drag-drop
are in. Band trash buttons hide outside edit mode. 87 files, 1427 passed, 12 skipped. Phases 17 and 18 were
already complete (2026-10-31), so the next not-started phase is 19
(Compose skeleton). No migration; next Acuranzo file 1444; next
caller-facing QueryRef 173.

**2026-11-03 — Phase 19 started; acuranzo_1444 drafted.** Andrew widened
the scope: compose every band role, any count, multi-level groups (not
one level). `Reports.Render` (group Reports, invokable 1, mcp_access 0)
takes `stage`, inline `definition`, and `sources`; `stage=compose`
returns `{ pages, log }`. Values as-is. Unknown typeKey, missing
datasource, and empty field warn; format ≠ 1 and other stages return
an error body. Added the bands and warnings fixtures. All five fixtures
match under a local Lua 5.5 harness. Known gap: Hydrogen encodes an
empty Lua table as `{}`, so empty `items` and `log` come back as `{}`
rather than `[]`. Next Acuranzo file 1445; next caller-facing QueryRef
173 (unused).

**2026-11-03 — Phase 19: Compose tab.** Renamed the JSON tab to
Definition (page id still `json`). Added Compose, Preview HTML, and
Preview PDF after Revisions. Compose runs `Reports.Render`
`stage=compose` on the open definition with the session sample as
sources and shows `{ pages, log }` read-only, with `{}` shown as `[]`.
HTML and PDF are placeholders. Phase 19 stays In progress until
Andrew applies acuranzo_1444 and the fixture calls match.

**2026-11-03 — Phase 19: editor toolbars.** Run moved to the start of
the right-hand cluster. Definition and Compose hide Import/Export and
show Collapse all, Expand all, and the shared font popup (Query Manager
pattern, per-page localStorage). Phase 19 stays In progress.

**2026-11-03 — Phase 19 complete.** Andrew applied acuranzo_1444 and
enabled Scripting in hydrogen-pintura.json. Compose is live in the
designer: the JSON tab is now Definition, Compose runs
`Reports.Render` `stage=compose`, and Preview HTML and Preview PDF
are placeholders. All five structural fixtures match. Next: Phase 20
Value format (amendment file 1445).

**2026-11-03 — Phase 20 started.** Wrote acuranzo_1445 (does not
edit 1444): `Reports.Render` compose routes field items through the
Value format table. Null prints blank; a bad mask or value warns once
per item and prints raw. Added the masks fixture set. The formatted
detail fixture and the masks fixture match under the local Lua 5.5
harness, and static, grouped, bands, and warnings are unchanged.
Variance: absent and null columns both print blank with no warning.
Waiting on Andrew to apply 1445. Next Acuranzo file 1446.

**2026-11-03 — Phase 19b drafted.** Andrew locked the datasource
decisions in Coder 3 chat. The Report owns datasources; the first
drives iteration. `json` rows are stored in the definition for good.
A new kind lookup (JSON, Query, CSV) gets one Acuranzo file after
1445. QueryRef picker, `Reporting` status, the demo query, and query
field and param metadata stay in Phase 28. Phase 20 stays in
progress. Next Acuranzo file 1446.

**2026-11-03 — Phase 19b started.** Report panel gains a Datasources
section (Count adds or trims; per-datasource Name and Kind; blank Name
removes). Kind stores `kindKey` (lookup 076) plus `kind` code;
Lithium falls back to JSON/Query/CSV until 076 loads. Only JSON is
accepted. Load Sample now writes `rows` into the selected band's
datasource (else the first json one; creates `data1` if none) and
needs edit mode; the session-only `_sample` is gone. Field lists the
band datasource's columns. `composeSources` sends each json
datasource its own rows; Compose strips `rows` from the definition
it sends and names the first datasource on iterating bands that have
none. Wrote acuranzo_1446 (lookup 076 Report Datasource Kinds).
Waiting on Andrew to apply 1446 and the designer check. Next
Acuranzo file 1447.

**2026-11-03 — Phases 19b and 20 complete.** Andrew ran Compose on an
embedded `json` datasource: `1,250.50` / `89.99` / `4,300.00` with
mask `#,##0.00` and `"log": []`. That closes the 19b Compose path and
the Phase 20 mask check. Andrew accepted designer check; 1446 is
applied. Datasources UI "will need work but ok for now" — polish,
group-sort enforcement, and Lua filter/custom fields follow later.
Next: Phase 21 (Paging), not started. Next Acuranzo file 1447.

**2026-11-03 — Phase 28 started ahead of 21.** Andrew: "Lets do 28
first. Need to have some data to play with before we can do much of
anything else." He waived the Phase 27 entry gate. Built 28.1 and
28.2: `kind` `query` is live, Run writes the rows into the
datasource definition, and Reporting queries are lookup 028 key 3
(already present). acuranzo_1447.lua adds QueryRef 173 (List
Reporting Queries) and acuranzo_1448.lua adds QueryRef 174 (Reporting
Demo, synthetic rows). Neither is applied yet. 28.3 (parameter item)
and the 28.4 browser check are open. Next Acuranzo file 1449. Next
caller-facing QueryRef 175.

### 2026-11-03 — Phase 28 panel polish + 28.3 Parameter item

- Query datasource: Params JSON box replaced by one row per parameter
  (`ROW_LIMIT int`, `TITLE str`, type in a muted span); Rows renamed
  Fields, a multiline `name  type` list. Name/Kind/Query unchanged.
  Run and Compose auto-run unchanged.
- 28.3 built (not parked): System palette category → Parameter item
  (053 key 63, `param` + `mask`). Acuranzo **1449** adds it to
  Reports.Render compose. Not applied. Next file **1450**.

### 2026-11-03 — Phase 28 closed; palette drag rule

- Andrew's browser check: panel polish (28.2b) and System → Parameter
  (28.3) "all looks pretty good". Acuranzo **1449** accepted. Phase 28
  complete. Next: Phase 29, Acuranzo file **1450** (not started).
- Rule (Andrew): **all future report-editor palette items (e.g. System
  → Parameter and anything similar) must support drag onto the canvas,
  not click-only.** Parameter is click-only today; drag is a deferred
  follow-up tracked under Phase 28 in reporting/04-pdf.md, not a
  Phase 28 reopen.

### 2026-11-03 — Parameter drag

- System → Parameter is now draggable onto a band (reuses the Field
  drop path, kind `reports-field:63`, item `{typeKey: 63, param: ''}`).
  Edit mode required; click still adds to the selected band. Unit test
  in reports-items. Tests 1447 pass. No migration.

### 2026-11-03 — Phase 21 start (handoff for a new Coder bot)

- **State:** Phases 0–20 (incl. 10-UI, 13b, 14b, 19b) and 28 are
  Complete. 28 ran before 21–27 on Andrew's waiver. Parameter drag
  done. Tests 1447 pass. Acuranzo 1444–1449 applied; next file
  **1450**; next caller-facing QueryRef **175**.
- **Next:** Phase 21 Paging — body and a "Start here" block in
  [`reporting/03-preview.md`](/docs/Li/plans/reporting/03-preview.md).
  Then 22 → 23 (needs paging) → 24 → 25 → 26 → 27, then 29.
- **Settled, do not re-derive:** datasources (Report owns them, first
  drives iteration, `json`/`query` rows in the definition). Read
  19b/28 Status and Handoff only if needed.
- **Constraints:** machine angrin, repo `/mnt/extra/Projects/Philement`;
  Andrew applies migrations; new amendment file, never edit applied
  ones; no commit unless asked; do not reopen 14/14b; flatpickr parked
  on Phase 22; new palette items must be canvas-draggable.
- **Paths:** `Reports.Render` in
  `elements/002-helium/acuranzo/migrations/acuranzo_1444/1445/1449.lua`;
  fixtures `elements/003-lithium/tests/fixtures/reporting/`; designer
  `elements/003-lithium/src/managers/reports/` (Compose tab
  `reports-compose.js`); contract `/docs/Li/LITHIUM-RPT.md`.

### 2026-11-03 — Phase 21 drafted (acuranzo_1450)

- Wrote acuranzo_1450 (does not edit 1449): `Reports.Render` compose
  pages on stored band heights against `page.height` less top/bottom
  margins. Page header/footer repeat; a group header moves with its
  first detail row; too big for an empty page warns and places anyway;
  group headers do not repeat. Reverse restores the 1449 body.
- Added the `overflow.*` fixture set (3 in page, header moves to page
  2 with its first row). All seven sets match under the local Lua 5.5
  harness. Phase 21 stays In progress until Andrew applies 1450 and
  the overflow call matches. Next Acuranzo file 1451; QueryRef 175 unused.

### 2026-11-03 — Phase 21 complete

- Andrew applied acuranzo_1450 and confirmed paging in the Compose tab:
  taller bands pushed content past the page limit and `pages` held more
  than one page. 21.1–21.3 ticked; Phase 21 Status Complete.
- **Next:** Phase 22 (System datetime and system user), 03-preview.md;
  swap flatpickr to `flatpickr-ce@4.9.0` when it opens. Next Acuranzo
  file **1451**; next caller-facing QueryRef **175** (unused).

### 2026-11-03 — Phase 22 rewrite; 22a / 22b added (docs only)

- Andrew locked: Luxon token subset as the only mask language; ISO
  week (first week with a Thursday); Sunday first day until Profile;
  Timezone `User` | `UTC` | `Explicit` plus report default zone; user
  zone from JWT `tz`; presets in an editable Lookup seeded with six
  masks; luatz seeded as a library pack reading host tzdata (no
  bundled zoneinfo), with an Update / refresh section (version pin,
  TBD config keys, tzdata via OS package).
- Order: 22 → 22a → 22b. Value format section amended. No code.
  Next Acuranzo file **1451**; QueryRef **175** unused; preset Lookup
  id TBD.

### 2026-11-03/04 — Phases 22, 22a, 22b packet

- 1451 (22): Render prints System Date (053 key 5) at one generation
  instant (`params.now`, else `H.system.now`) with Timezone
  User/UTC/Explicit and IANA `zone`; System User (key 6) prints
  `_hydrogen.username`, blank + one warn when empty. A User claim uses
  `_hydrogen.tzoffset`; tk 24 shifts only when `timezone` is set.
- 1452 (22a): luatz v0.4-4-ga7c0310 (MIT) seeded as group `luatz`
  (9 modules, local `tzcache.set_base_path` patch). Env hooks
  `REPORTS_TZ_ZONEINFO_PATH` (default /usr/share/zoneinfo) and
  `REPORTS_TZ_DEFAULT_ZONE`. Bad or unreadable zones print UTC + warn.
  Blocker: the sandbox leaves `io` nil unless `Sandbox.AllowIo`; until
  Andrew allows it (or a read-only zoneinfo reader), named zones print
  UTC with a warn.
- 1453 (22b): Luxon token subset in compose; Lookup 077 Report Datetime
  Format Presets (6 rows). Stored definitions are not rewritten; old
  masks convert once by hand (mmm→MMM, mm→MM, hh→HH, nn→mm, quote
  other letters). Fixtures masks/system/zones/presets match 1453.
- Lithium: System Date / System User palette buttons (click + drag),
  Timezone/Zone/Mask preset rows, report Default zone, lookup 077 load;
  `flatpickr` pinned to `npm:flatpickr-ce@4.9.0`.
- Applied 2026-11-03; see complete entry below.

### 2026-11-03 — Phases 22, 22a, 22b complete

- Andrew applied acuranzo_1451–1453 and confirmed in the designer: User
  time, UTC time, mask presets (Lookup 077), and System User username.
  "Seems to work well." "I think we can wrap up all of Phase 22."
- Status Complete for 22, 22a, and 22b. Named IANA zones still need
  Sandbox.AllowIo (or a read-only zoneinfo allow) for host tzdata;
  User+JWT uses tzoffset as locked.
- **Next:** Phase 23 (Page number and page count) in 03-preview.md.
  Next Acuranzo file **1454**; QueryRef **175** unused.

### 2026-11-03 — Phase 23 packet handed over

- acuranzo_1454: lookup 053 key 64 Page label; Render fills 61 page
  number, 62 page count (number mask) and 64 template `{n}`/`{c}` after
  paging. Reverse restores 1453 Render and deletes key 64.
- Andrew: Page label is one freeform Template string, no presets.
- Lithium: Page Number / Page Count / Page Label palette buttons (click +
  drag), Template row. Fixture `pagenum.*`.
- Next Acuranzo file **1455**; QueryRef **175** unused.

### 2026-11-04 — Phase 23 complete

- Andrew applied acuranzo_1454 and confirmed Page Number, Page Count
  and Page Label print in the designer.
- Page Label is one freeform `template` string with `{n}`/`{c}`. No
  coded presets.
- Bug: Lithium `ITEM_TYPES` in `model/document.js` omitted 64 while
  palette, drop, sample and properties had it, so `requireItemType(64)`
  threw `Item type 64 is not a v1 item.` Fix: add 64 to `ITEM_TYPES`.
  `MASK_TYPES` left alone (template, not mask). 61/62 were already in
  `ITEM_TYPES`, which is why they worked first. No migration needed.
- **Next:** Phase 24 (HTML emit) in 03-preview.md. Next Acuranzo file
  **1455**; QueryRef **175** unused.

### 2026-11-04 — Phase 24 packet handed over

- Andrew pinned the two open strings: the result field is `html`
  (`{ html, log }`), and each section is the full page with the band
  stack inset by the four margins.
- acuranzo_1455.lua copies the 1454 Render body and adds `stage=html`.
  An omitted stage still runs compose. Reverse restores the 1454 body.
  No new lookup. No new QueryRef.
- Stored lengths drop the space (`0 in` becomes `0in`). The section
  uses `box-sizing: border-box` so the margin padding sits inside the
  page size. Text is escaped. Clip is `overflow: hidden` and
  `white-space: pre-line`.
- Fixture `detail.html` is the `html` value for the detail definition
  and detail sources, including the final newline. A Lua 5.5.1 probe
  of the new body matched `detail.intermediate.formatted.json` on
  compose, then matched that skeleton. `pdf` is still rejected.
  luacheck on the migration file reported 0 warnings.
- Phase 23's index row had stayed "Not started" after the close-out.
  It is Complete. Phase 24 stays in progress until Andrew reports the
  apply and the fixture call.
- **Next:** Andrew applies acuranzo_1455.lua. Next free Acuranzo file
  **1456**. QueryRef **175** unused. Phase 25 reads `result.html`.

### 2026-11-04 — Preview HTML Run, still in Phase 24

- Andrew asked for the rendered page in this sitting, before the
  1455 apply. Font settings stay off Preview HTML. A viewer font
  would fight the inline item fonts.
- Preview HTML's Run button calls `Reports.Render` at `stage=html`
  through the existing `invokeScript`. `sanitizeHtml` runs first.
  The page is assigned to a sandboxed iframe `srcdoc`. The parent
  does not assign report HTML to `innerHTML`. An error toasts once
  and leaves the previous page. `log` is kept and not shown.
- There is no auto-render. Phase 25.3's "browser trial in 23.5"
  does not exist, so that choice stays open. The generation log
  stays Phase 26. Over-cap preview stays Phase 27.
- The live page cannot render until acuranzo_1455.lua is applied,
  because the running script still rejects `stage=html`.
- Phase 24 stays in progress until the apply and the fixture call.
  Phase 25 stays in progress: the Run path is in, auto-render and
  the browser check are not. Do not mark either phase complete.
- Preview HTML sits in the Editor stage: the same padding, and the
  paper uses the sheet's corner and shadow plus a page edge.
  Definition and Compose use that same inset under the toolbar.

### 2026-11-04 — Phase 24 complete

- Andrew said the rendered Preview HTML page "works pretty well," then
  accepted the Editor-matching chrome ("Excellent work") and asked to
  close the phase. That is the apply report for `acuranzo_1455.lua`:
  the previous script rejects `stage=html`, and the page rendered.
- The Lua 5.5.1 probe matched `detail.html`, including the final
  newline. He did not send a separate byte dump of the live result.
  No new lookup. No QueryRef. Next free Acuranzo file **1456**.
  QueryRef **175** unused. Leave `acuranzo_1455.lua` as written.
- The paper is `.reports-preview-frame` in `reports-preview.css`.
  Width and height are `--reports-preview-width` and
  `--reports-preview-height`, from the first section, else the
  definition page. Several pages use `calc(page height * section
  count)` in one frame, because `sandbox=""` cannot be measured.
  The frame is `content-box`, so the 1px edge sits outside the paper.
- Definition and Compose use the same inset as the Editor.
  `.reports-json-wrap` padding is `var(--space-2) var(--space-3)`.
  The right-pane toolbar already zeros its bottom margin, so that
  top padding is the gap under the toolbar.
- `reports.js` is 1000 lines. New preview CSS stays in
  `reports-preview.css`. `reports.css` is already over that cap.
- Phase 25 stays in progress. The Run path is in. Auto-render (25.3b)
  is undecided. The two-row browser check (25.5) is open. The
  missing-sample warn moves to Phase 26, because the screen does not
  list `log` yet.
- **Next:** Phase 25 (Preview) in 03-preview.md. Ask about auto-render
  before adding it. Do not build the generation log. Do not start
  Phase 26.

### 2026-11-04 — Phase 25 complete

- Andrew declined auto-render. Run stays the only trigger. He
  previewed the detail fixture with two sample rows and the second
  detail band rendered.
- No source change this sitting. The Run path, the sanitizer, the
  error toast, and the page chrome stay as Phase 24 left them.
  `reports-preview.test.js` and `reports-chrome.test.js` passed in
  that sitting (2 files, 22 tests).
- The missing-sample warn stays Phase 26.3. The screen still does
  not list `log`.
- No new Acuranzo file. No new QueryRef. Next free file **1456**.
  QueryRef **175** unused.
- **Next:** Phase 26 (Generation log) in 03-preview.md. The levels,
  the flag priority, and the session-state rule are pinned. The
  phrase "report header" is a band in this UI, so the panel's place
  is still open. Do not start Phase 26 until that place is chosen.
- Andrew asked which log Phase 26 shows. Compose already shows `log`
  inside the intermediate JSON. Preview HTML keeps the same kind of
  array from its own call and does not show it. There is not a second
  log type. He then chose a Log tab: a list of level and message,
  with the three counts, filled by the last Run (Compose or Preview
  HTML). The flag is the worst level on that tab. Phase 26 is ready
  and has not been built.

### 2026-11-04 — Phase 26 Log tab, in progress

- Andrew asked for the same page chrome as Definition, and for
  strictly formatted text to use CodeMirror, aligned in columns, in
  Vanadium Mono.
- The Log tab sits after Preview HTML. The buffer is aligned text:
  info, warn, and error counts, a flag column when a level fired,
  then one row per entry. Word wrap is off. `plain` adds no syntax
  colors. Compose Run and Preview HTML Run fill it. A failed run
  leaves the previous log. Another report, and New, clear it.
- The flag is the tab color and the flag column. It is not a column
  on `reports` and not a mark on the report list.
- Tests: `reports-log`, `reports-chrome`, and `reports-preview`,
  3 files, 26 passed. ESLint on the touched files passed. Stylelint
  on `reports-log.css` passed. `reports.js` is 999 lines.
- **Next:** Andrew's browser check (26.3). A missing sample shows one
  warn and the warn flag. A clean detail preview shows zeros and no
  flag. Do not mark Phase 26 complete until then. Do not start
  Phase 27.

### 2026-11-04 — Phase 26 complete

- Andrew said "Ok, I think that is working." That is the browser
  check for 26.3. He did not quote the warn line, a warn count of 1,
  or a clean-detail zero dump.
- The Log tab is aligned text in CodeMirror, Vanadium Mono. The last
  Run fills it, from Compose or from Preview HTML. The flag is the
  worst level on that tab. The counts are session state.
- `reports.js` is 1000 lines. The in-progress note said 999.
- No new Acuranzo file. No new QueryRef. Next free file **1456**.
  QueryRef **175** unused. Leave `acuranzo_1455.lua` as written.
- The call-shape lock says a stored report may send `report_id` and
  `rev_id` and the script loads the row. `acuranzo_1455.lua` still
  requires `P.definition`. Phase 27's body does not name a migration.
- **Next:** Phase 27 (Over-cap preview) in 03-preview.md. Do not start
  it in this conversation. A definition past `ClientInvokeMaxParamsBytes`
  (default 256 KiB) saves first and previews by `report_id`. One toast
  names that path. A failed save leaves the canvas unchanged. Do not
  send the session JWT. Sources stay inline. Raising the params cap
  is operator config and is not this phase.
