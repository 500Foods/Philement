<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting Plan

**Date:** 2026-10-10
**Revised:** 2026-11-05
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
Phase 16 (one group level) is complete (2026-11-03): Andrew said "Seems to be working pretty well." Phases 19, 19b, 20 and 28 are complete (2026-11-03); 28 ran ahead of 21 on Andrew's waiver. Phase 21 (Paging) is complete (2026-11-03). Acuranzo 1444–1450 are applied. Phases 22, 22a and 22b are complete (2026-11-03): Andrew applied acuranzo_1451–1453 and confirmed User time, UTC time, mask presets, and System User. Phase 23 (Page number and page count) is complete (2026-11-04): Andrew applied acuranzo_1454 and confirmed Page Number, Page Count and Page Label. **Phase 24 (HTML emit)** is complete (2026-11-04). Andrew confirmed the rendered Preview HTML page and the Editor-matching chrome. `acuranzo_1455.lua` is the applied script. `stage=html` returns `{ html, log }`. The Lua 5.5.1 probe matched `detail.html`. He did not send a separate byte dump. Preview HTML's Run button calls that stage and shows the sanitized page in a sandboxed iframe. Font settings stay off that tab. The next free Acuranzo file is **1457**. The next caller-facing QueryRef is **175**. No new lookup. Phase 25 (Preview) is complete (2026-11-04). Andrew declined auto-render. Run is the only trigger. He previewed two detail rows and the second band rendered. Phase 26 (Generation log) is complete (2026-11-04). Andrew said "Ok, I think that is working." The Log tab is aligned text in CodeMirror, Vanadium Mono, filled by the last Run. He did not quote the warn line, the warn count, or a clean-detail zero dump. Phase 27 (Over-cap preview) is complete (2026-11-04). Andrew applied acuranzo_1456.lua and asked to close the phase. Preview HTML saves a definition that does not fit in 256 KiB and calls Reports.Render with report_id and rev_id. The toast is Saved to preview. He did not quote the toast or a failed-save canvas. Phase 28 is already complete (2026-11-03). Phase 29 is complete (2026-11-04). Andrew applied `acuranzo_1457.lua` and asked to close the phase. He did not quote the two-row log line. A query datasource with no array in sources loads `query_type_a28` 3 and runs it on the session database. An array in sources still wins. Phase 38 (count, filled, unique, and sum) is complete (2026-11-04). Andrew said it works well, that 1457 has been applied, and that the aggregate functions were tested with no issues found. He did not name `acuranzo_1458.lua` or quote the fixture totals. The packet for this phase is `acuranzo_1458.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1458.lua` as written. Phase 39 (page totals) is complete (2026-11-04). Andrew said the latest migration applied is 1459, and that aggregate functions in page headers and footers work as indicated. He did not quote the page sums, the counts, or the names. The packet is `acuranzo_1459.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as written. The next free Acuranzo file is **1460**. QueryRef **175** stays unused. No new lookup. Phases 40, 41, and 42 are complete (2026-11-04). Andrew said they are fully operational and confirmed the values returned. He did not name `acuranzo_1460.lua`, `acuranzo_1461.lua`, or `acuranzo_1462.lua`, and he did not quote the fixture figures. On his eight amounts the mode tie printed -17.25 and the median was 65.995. A `#,##0.00` mask prints 66.00. The packets are those three files. Leave `acuranzo_1455.lua` through `acuranzo_1462.lua` as written. Phases 43 and 44 are complete (2026-11-04). Andrew said they look good. He did not name the files and he did not quote the fixture figures. That check is the apply report. The packets are `acuranzo_1463.lua` and `acuranzo_1464.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1464.lua` as written. The next free Acuranzo file is **1467**. QueryRef **175** stays unused. Lookup **078** is Report Shape Kinds. The next free lookup id is **079**. The next free lookup 054 key is **52**. **Phase 44a (Static shapes)** is complete (2026-11-04). Andrew said everything else looks good and asked to wrap the phase. He did not quote the fixture colours. Andrew says 1466 is the last migration he sees and it is applied. **Phase 44b (Shape fill and gradient)** is complete (2026-11-04). Andrew said the gradients render on Preview HTML, everything tested passes, and 1467 is the latest migration applied. He did not quote the fixture colours. **Phase 44c (Static number, datetime, image, and memo)** is complete (2026-11-05). Andrew said the new version renders a Font Awesome duotone icon and asked to close the phase. He did not name `acuranzo_1468.lua`, and he did not quote `1,234.50` or `2026-Mar-05 14:07`. The packet is `acuranzo_1468.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1468.lua` as written. Phases 53 and 54 are in progress (2026-11-05). lua-zint and luatz are packed in the Hydrogen payload, not seeded into `scripts`. Andrew said all outstanding migrations have been applied, so `acuranzo_1469.lua` through `acuranzo_1474.lua` are applied. Leave `acuranzo_1455.lua` through `acuranzo_1474.lua` as written. `acuranzo_1475.lua` lists Datasource before Field on lookup 053 keys 20, 21, 22, 23, 24, and 26. `acuranzo_1476.lua` amends Reports.Render so a field memo, image, and shape read that pair, and so a text, number, or datetime field uses the datasource when it is set. `acuranzo_1477.lua` adds lookup 075 keys 13–16: Barcode Placement, Barcode Text, Barcode Colors, and Barcode Symbology. 1475, 1476, and 1477 are written and not applied. The next free Acuranzo file is **1478**. QueryRef **175** stays unused. Next free lookup id **080**. Next free lookup 054 key **53**. Phase 45 has not started. Naming Phase 45 starts it. Phases 33–37, 46–48, Phase 55, PDFs, and charts stay on the temporary skip. Barcodes 53–54 are in progress. Phase 18's body is in
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
| 27 | Over-cap preview | Lithium UI | Easy | Complete |
| 28 | Query designer | Lithium UI | Medium | Complete |
| 29 | Query execution | Helium amendment | Medium | Complete |
| 30 | Seed lua-pdfkit | Helium library pack | Easy | Not started |
| 31 | PDF text | Helium amendment | Medium | Not started |
| 31-UI | Preview PDF viewer | Lithium UI | Medium | In progress |
| 32 | Result cap | Helium amendment | Easy | Not started |
| 33 | Text formula | Helium amendment | Medium | Not started |
| 34 | Formula editor | Lithium UI | Medium | Not started |
| 35 | Formula check | Lithium UI + Lua | Easy | Not started |
| 36 | Number and datetime formula | Helium amendment | Medium | Not started |
| 37 | Formula runtime errors | Helium amendment | Easy | Not started |
| 38 | Count, unique, and sum | Helium + designer | Medium | Complete |
| 39 | Page totals | Helium + designer | Medium | Complete |
| 40 | Average and weighted | Helium amendment | Medium | Complete |
| 41 | Median | Helium amendment | Medium | Complete |
| 42 | Mode | Helium amendment | Medium | Complete |
| 43 | Min and max | Helium amendment | Medium | Complete |
| 44 | First and last | Helium amendment | Medium | Complete |
| 44a | Static shapes | Helium + designer | Medium | Complete |
| 44b | Shape fill and gradient | Helium + designer | Medium | Complete |
| 44c | Static number, datetime, image, and memo | Helium + designer | Medium | Complete |
| 45 | Group-key function | Helium + designer | Medium | Not started |
| 46 | Page-break predicate | Helium + designer | Easy | Not started |
| 47 | Line-break predicate | Helium + designer | Easy | Not started |
| 48 | Header and footer context | Helium amendment | Medium | Not started |
| 49 | CSV emit | Helium amendment | Easy | Not started |
| 50 | Embedded image | Lithium UI + Lua | Medium | Not started |
| 51 | Reference and URL images | Helium amendment | Medium | Not started |
| 52 | PDF image stored once | Helium amendment | Easy | Not started |
| 53 | Seed lua-zint | Helium library pack | Easy | In progress |
| 54 | Barcode SVG | Lithium UI + Lua | Medium | In progress |
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
phases added 2026-11-02 for the property panel. Phases 44a and
44b are labeled phases added 2026-11-04 for static shapes.
Phase 44c is complete (2026-11-05). The packet is
`acuranzo_1468.lua`. Phase 31-UI is a labeled sidequest for the
Preview PDF page, in progress (2026-11-05).
None of these is an extra number in that count.

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
| 31-UI | Open Preview PDF and page through a sample file. |
| 35 | See a syntax error in the formula editor. |
| 36 | See a pack-date code and a masked number. |
| 44 | Put the built-in totals on a footer. |
| 44a | Draw a static shape on the design surface and in the HTML preview. |
| 44b | Draw a static shape in the HTML preview, including a fill and a gradient. |
| 44c | Place a static number, datetime, image, and memo. A wrapping line grows or shrinks its own row. Preview HTML shows them. |
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
| [`04-pdf.md`](/docs/Li/plans/reporting/04-pdf.md) | 28–32, and 31-UI | Run a QueryRef and download a text PDF. Phase 31-UI is the Preview PDF viewer (in progress). |
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
Phase 16 (one group level) is complete (2026-11-03): Andrew said "Seems to be working pretty well." Phases 19, 19b, 20 and 28 are complete (2026-11-03); 28 ran ahead of 21 on Andrew's waiver. Phase 21 (Paging) is complete (2026-11-03). Acuranzo 1444–1450 are applied. Phases 22, 22a and 22b are complete (2026-11-03): Andrew applied acuranzo_1451–1453 and confirmed User time, UTC time, mask presets, and System User. Phase 23 (Page number and page count) is complete (2026-11-04): Andrew applied acuranzo_1454 and confirmed Page Number, Page Count and Page Label. **Phase 24 (HTML emit)** is complete (2026-11-04). Andrew confirmed the rendered Preview HTML page and the Editor-matching chrome. `acuranzo_1455.lua` is the applied script. `stage=html` returns `{ html, log }`. The Lua 5.5.1 probe matched `detail.html`. He did not send a separate byte dump. Preview HTML's Run button calls that stage and shows the sanitized page in a sandboxed iframe. Font settings stay off that tab. The next free Acuranzo file is **1457**. The next caller-facing QueryRef is **175**. No new lookup. Phase 25 (Preview) is complete (2026-11-04). Andrew declined auto-render. Run is the only trigger. He previewed two detail rows and the second band rendered. Phase 26 (Generation log) is complete (2026-11-04). Andrew said "Ok, I think that is working." The Log tab is aligned text in CodeMirror, Vanadium Mono, filled by the last Run. He did not quote the warn line, the warn count, or a clean-detail zero dump. Phase 27 (Over-cap preview) is complete (2026-11-04). Andrew applied acuranzo_1456.lua and asked to close the phase. Preview HTML saves a definition that does not fit in 256 KiB and calls Reports.Render with report_id and rev_id. The toast is Saved to preview. He did not quote the toast or a failed-save canvas. Phase 28 is already complete (2026-11-03). Phase 29 is complete (2026-11-04). Andrew applied `acuranzo_1457.lua` and asked to close the phase. He did not quote the two-row log line. A query datasource with no array in sources loads `query_type_a28` 3 and runs it on the session database. An array in sources still wins. Phase 38 (count, filled, unique, and sum) is complete (2026-11-04). Andrew said it works well, that 1457 has been applied, and that the aggregate functions were tested with no issues found. He did not name `acuranzo_1458.lua` or quote the fixture totals. The packet for this phase is `acuranzo_1458.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1458.lua` as written. Phase 39 (page totals) is complete (2026-11-04). Andrew said the latest migration applied is 1459, and that aggregate functions in page headers and footers work as indicated. He did not quote the page sums, the counts, or the names. The packet is `acuranzo_1459.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as written. The next free Acuranzo file is **1460**. QueryRef **175** stays unused. No new lookup. Phases 40, 41, and 42 are complete (2026-11-04). Andrew said they are fully operational and confirmed the values returned. He did not name `acuranzo_1460.lua`, `acuranzo_1461.lua`, or `acuranzo_1462.lua`, and he did not quote the fixture figures. On his eight amounts the mode tie printed -17.25 and the median was 65.995. A `#,##0.00` mask prints 66.00. The packets are those three files. Leave `acuranzo_1455.lua` through `acuranzo_1462.lua` as written. Phases 43 and 44 are complete (2026-11-04). Andrew said they look good. He did not name the files and he did not quote the fixture figures. That check is the apply report. The packets are `acuranzo_1463.lua` and `acuranzo_1464.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1464.lua` as written. The next free Acuranzo file is **1467**. QueryRef **175** stays unused. Lookup **078** is Report Shape Kinds. The next free lookup id is **079**. The next free lookup 054 key is **52**. **Phase 44a (Static shapes)** is complete (2026-11-04). Andrew said everything else looks good and asked to wrap the phase. He did not quote the fixture colours. Andrew says 1466 is the last migration he sees and it is applied. **Phase 44b (Shape fill and gradient)** is complete (2026-11-04). Andrew said the gradients render on Preview HTML, everything tested passes, and 1467 is the latest migration applied. He did not quote the fixture colours. **Phase 44c (Static number, datetime, image, and memo)** is complete (2026-11-05). Andrew said the new version renders a Font Awesome duotone icon and asked to close the phase. He did not name `acuranzo_1468.lua`, and he did not quote `1,234.50` or `2026-Mar-05 14:07`. The packet is `acuranzo_1468.lua`. Leave `acuranzo_1455.lua` through `acuranzo_1468.lua` as written. Phases 53 and 54 are in progress (2026-11-05). lua-zint and luatz are packed in the Hydrogen payload, not seeded into `scripts`. Andrew said all outstanding migrations have been applied, so `acuranzo_1469.lua` through `acuranzo_1474.lua` are applied. Leave `acuranzo_1455.lua` through `acuranzo_1474.lua` as written. `acuranzo_1475.lua` lists Datasource before Field on lookup 053 keys 20, 21, 22, 23, 24, and 26. `acuranzo_1476.lua` amends Reports.Render so a field memo, image, and shape read that pair, and so a text, number, or datetime field uses the datasource when it is set. `acuranzo_1477.lua` adds lookup 075 keys 13–16: Barcode Placement, Barcode Text, Barcode Colors, and Barcode Symbology. 1475, 1476, and 1477 are written and not applied. The next free Acuranzo file is **1478**. QueryRef **175** stays unused. Next free lookup id **080**. Next free lookup 054 key **53**. Phase 45 has not started. Naming Phase 45 starts it. Phases 33–37, 46–48, Phase 55, PDFs, and charts stay on the temporary skip. Barcodes 53–54 are in progress. Phase 18's body is in
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
12. Image elements carry their own display dpi. Bitmap import
    converts at once. The definition keeps one canonical copy in
    `doc.images`. Preview builds one display PNG per placement
    key. HTML and PDF each store that bitmap once and reference
    it. A supported SVG stays a vector in the file, in HTML, and
    as one PDF form XObject.
13. A number, date, aggregate, or parameter prints through a format
    mask. Null is blank. Align and font are honored. `fitHeight`
    `clip` keeps text inside its box. `grow`, `shrink`, and `both`
    change that printed row in line-height steps. The stored band
    height stays. A group header stays with its first detail row,
    using the used height.
14. A datasource may name a QueryRef and the report's parameters.
    The canvas still designs against sample rows. Phase 29
    runs the query.

## Out of scope until a later plan

- Crosstabs, subreports, packages, and the hidden twins of shape,
  image, and memo. The lookup rows can stay. The palette hides them.
  Static shapes are Phases 44a and 44b. Static number, datetime,
  image, and memo are Phase 44c, drafted and revised 2026-11-04
  and not started. Field memo stays here. Field image, reference,
  and URL stay in Phases 50–52. Chart keys 42 and 44, and barcode
  keys 43 and 45, are v1.
- Chart interactivity. The exported chart is static SVG or a PDF
    vector object; hover, zoom, and tooltips are a later idea.
- A user's own SVG that is outside the Phase 44c path subset
  entering the PDF as vectors. The subset is svg, g, path, rect,
  circle, ellipse, line, polyline, and polygon, with fill, stroke,
  stroke-width, opacity, and translate, scale, or rotate. The file
  and the HTML keep the SVG. PDF uses a display PNG for a placement
  outside that subset until the subset grows. Charts and barcodes
  stay on their own vector path. A zint PDF XObject is embedded as
  given.
- A native XLS writer. CSV is the spreadsheet stand-in.
- A SQL catalog inside the designer. A datasource is a JSON sample
  or a named QueryRef. The designer does not edit SQL.
- Browser PDF generation (`jsPDF`). The Preview PDF viewer is Phase 31-UI. It uses `pdfjs-dist` and does not generate a PDF. Crop stays out.
- Mailing a report. Mail Relay already exists for mail.
- A new Hydrogen subsystem, config letter, or C route. PDF, barcodes,
  and charts stay in Lua. Lithium calls the Reporting endpoint that
  already shipped when a raster is required.
- Rewriting CATCHUP, Course Builder, or Argent's financial queries.
- Measuring wrapped text with the report's Vanadium face. Phase
  44c measures with the shared Helvetica AFM table and grows or
  shrinks the printed instance in line-height steps. A grown item
  is drawn in Helvetica so the measured line fits. `embedFonts`
  can carry the face in the file and does not change that measure.
  Ellipsis and shrinking the font stay here. Vanadium widths are
  a later swap of that table.
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
  column layouts. `jsPDF` was loaded on the page. The 2026-10-10
  write-up left both libraries out. Phase 31-UI (2026-11-05) brings
  `pdfjs-dist` back for viewing only. It does not bring `jsPDF`
  back, and it does not bring crop back.
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
report. Image bytes live once in `doc.images`, keyed by content
hash. The item stores that hash. A step that moves the item stores
the hash. Replacing an image leaves the previous bytes only in the
undo step that removed them. Clear drops `history` and drops a
registry entry no item references. The other keys are the current
report. There is no cap.
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
| 1 | Static number. A finite JSON `value` and the number mask. `fitHeight` defaults to `clip`. |
| 2 | Static image. Bytes live once in `doc.images`. The item stores the hash. Preview builds one display PNG from the box, `dpi`, and fit. |
| 3 | Static memo. Plain text. `fitHeight` defaults to `both`, so a wrapping memo grows or shrinks its own row in line-height steps. |
| 4 | Static datetime. A civil string or a unix number, and the Luxon mask. No timezone of its own. `fitHeight` defaults to `clip`. |
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
and italic flags. The report flag `embedFonts` (default `false`)
decides whether a used face is carried inside the HTML and the
PDF. When it is false, HTML names the family and PDF keeps the
standard face the port already carries. When it is true, each
used family is included once and referenced by name. HTML uses
one `@font-face` whose source is a base64 WOFF2. PDF uses one
subset TrueType font object, the form the port embeds, and does
not wrap a WOFF2. Helvetica and the other standard faces are
never attached. The bytes are not stored in the definition or
in history. Lithium passes them as `params.fonts`, one entry per
family. The script does not read a font file. `io` stays off.
The product WOFF2 files are about 1.5 MB each, so the entry is a
subset of the glyphs the report uses. A full file is not the
attachment. A render with the flag on and no `params.fonts`
warns once and names the family without bytes. Rotation, link,
and conditional color stay in Later.

Lookup 054 keys 9, 10, 11, and 12 (Number Format, Blank if Zero,
Zero if Blank, Negative Parentheses) stay off every v1 attribute
list. The `mask` string is the format.

`fitHeight` `clip` keeps text inside the item box. That is the
default for static text, field text, static number, and static
datetime. The canvas shows the same cutoff. `grow`, `shrink`, and
`both` change the printed item and the printed band instance.
Growth and shrink move in whole line-heights. The used line height
is `lineHeight` (default `1.2`) times the font size, in the same
unit on both sides. One shared function breaks the text. Widths
are the Helvetica AFM advances for the item's style at that size.
The designer, HTML, and the later PDF paint those lines and do not
break again. A grown item is drawn in Helvetica until a width
table exists for its family, so a measured line fits the box it
was measured for. `embedFonts` does not switch that measure.
The stored band height in the file does not change. The band
instance's used height is the lowest item bottom after the shift,
and not below `0.15 in`. Shrink does not go below one line. Each
detail row uses its own text, so one wrapping invoice line grows
and the other lines stay at their own height. Paging uses the used
height. Static memo defaults to `both`. An unknown `fitHeight`
warns once and uses the type default. Ellipsis and shrinking the
font stay in Later.

A group header is not left alone at the bottom of a page. If the
header plus one detail row do not fit in the remaining content box,
both move to the next page. The heights in that test are the used
heights. If they do not fit on an empty page either, compose warns
and places them anyway.

A page header or footer sees the group that is open on that page and
the detail rows placed on that page. Page totals are reduced from
those rows after the page list exists. Both the page header and the
page footer can show them.

Compose is not a single pass. Group and report totals are known
before the headers that display them are formatted.

1. Resolve rows. An array in `sources` wins. A query datasource with
   no array runs that QueryRef (Phase 29). An empty or unconfigured
   session database warns and contributes no rows.
2. Evaluate each detail-row function. Store the raw value. Format
   each detail item from that raw value and the item mask. A detail
   `ctx` has no aggregates, so a total is a header or footer item.
3. Reduce each group, and the whole report, from raw field values
   and those raw function results. Format the group header, the
   group footer, the report header, and the report footer from
   those totals. A calculated item on those bands may read
   `ctx.aggregates`. Its return is masked by its own type, and it
   is not fed back into the totals.
4. Paginate on the used height of each band instance. `fitHeight`
   has already shifted items and set that height. The stored band
   height in the file is unchanged. A group header
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
a scalar (Phase 28 stores the value the designer ran), `{ "parameter": "<name>" }`,
or `{ "literal": "<value>" }`. The
`reports.parameters` column stays `{}`. The run strip's current
values are session state, passed as `params.parameters`, and they
override a same-named bind. Saved presets wait with the parameter form.

An array in `sources` wins, which is how fixtures and the canvas
sample run. When that array is absent, Phase 29 runs the
QueryRef. It loads `SELECT code, collection FROM queries WHERE query_ref = :QREF AND query_type_a28 = 3`,
then `H.altquery_sync(params._hydrogen.database, code, binds)`
with the resolved binds. `query_type_a28 = 3` is SQL - Reporting.
Type 1 does not find QueryRef 174. Andrew chose type 3 on
2026-11-04. The database name is the session claim. When that name is
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
| lua-zint | Hydrogen `payloads/lua/zint/` (lua-zint-svg 0.1.0 commit `89a8231386682109312c29d8b84dc525c271eea3`). The earlier port home was `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/`. | `zint.barcode(opts)` returns an SVG string. Symbologies in the port: QR, Code 39, Code 128, UPC-A, GS1-128. |
| d3.lua | Poseidon job artifact `/home/asimard/DA/poseidon1/jobs/lua-run10/` (pure Lua; its own repo planned). Validated byte-identical to D3.js on the reference pair `d3_chart.js` / `lua_chart.lua` | `d3.create_root`, `d3.scaleBand`, `d3.scaleLinear`, `d3.axisLeft` / `d3.axisRight` / `d3.axisBottom`, `d3.stack`, `d3.curveBasis`, `d3.line`, `d3.range`. Entry point pinned at the Phase 56 probe. |

luatz (Phase 22a) is a fourth pack: pure Lua, group `luatz`, seeded
as `luatz.*` rows. It reads zoneinfo from host tzdata
(`/usr/share/zoneinfo`) with read-only `io.open`; no zoneinfo is
bundled. Data refreshes with the host image's `tzdata` package; code
refreshes by replacing `payloads/lua/luatz/`. The `scripts` rows stay
and are unused, because the payload file wins (see Payload require
below). `io` stays off, so zoneinfo still falls back to UTC.

**Status:** lua-pdfkit and d3.lua are not in this repository.
lua-zint and luatz are packed under Hydrogen `payloads/lua/`
(2026-11-05). They are
pure-Lua ports Andrew owns, each moving to its own repository: the zint
and pdfkit ports ship `tests/test.sh` plus `PORTING-NOTES.md` /
`AUTHOR-NOTES.md`, and the D3 port's validation is byte-equivalence
against D3.js itself. Each library gets its own seed phase and probe
(Phases 30, 53, and 56), and the packet records the repo commit or
version it was generated from. For lua-pdfkit and d3.lua, the path
in the table is the fallback when that repository does not exist yet. The seed phase stops and
asks when neither the repository nor the path is there. If a port
reaches LuaRocks, that helps
only if Hydrogen's sandbox can read `package.path`; the seeded
`group.script` module is the path this plan builds on, and the probe
confirms which one is live.

**Payload require (2026-11-05).** Andrew rejected seeding lua-zint
into `scripts` and rejected raising DB2's CLOB limit. Hydrogen
installs a payload searcher at `package.searchers` index 2, always,
before the file searcher. `require("zint.barcode")` loads
`lua/zint/barcode.lua` from the in-memory payload.
`require("luatz.tzcache")` loads `lua/luatz/tzcache.lua` the same
way. A payload file wins over a scripts row of the same name.
`AllowDBModuleLoad` stays off for those two libraries. The editor
still calls invokable `zint.Svg`, because invoke is not `require`.
lua-pdfkit and d3.lua are not packed. Their seed phases stay as
written. The existing luatz `scripts` rows stay. The payload file
wins, so those rows are unused. `io` stays off, so zoneinfo still
falls back to UTC.

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
calls `image_scale` at preview to build `params.rasters` for a
static or field image, and for SVG rasterization of charts and
barcodes (Phase 67, until Phase 71). A PNG that comes back from
that call follows the same rule: one object, then references.

**SVG input and the raster paths.** `image_scale` documents SVG as a
supported input, with `dpi` applied before the read so rasterization
honors it, and `png` as a supported output. That call still carries
PNG emit (Phase 67) and the chart and barcode rasters that wait on
Phase 71. A static or field SVG is not that path. The file stores
the SVG. HTML keeps it. The supported path subset is one PDF form
XObject. A placement outside that subset uses a display PNG until
the subset grows. The SVG bytes stay in the file either way.

**Image element options.** Bitmap import converts immediately.
JPEG with no alpha stays JPEG. PNG, GIF, WebP, ICO, BMP, and TIFF
become PNG. GIF keeps the first frame. ICO keeps its largest frame.
EXIF orientation is applied, so the stored raster is upright. SVG
is stored as SVG and is not rasterized at import. The canonical
bytes live once in `doc.images`, keyed by content hash. The item
stores that hash on `image`, plus placement. It does not store
`src`. Display size is the box in inches times the item `dpi`
(default 72), rounded half away from zero, and at least 1 pixel
on a side. A banner and an icon set their own dpi. Changing the
box or the dpi does not write a new canonical copy. Preview builds
one display PNG per distinct source, pixel size, fit, anchor,
grayscale, and invert, and passes that map as `params.rasters`.
Those PNGs are not written into the definition or into history.
The script does not call `image_scale`. Lithium holds the JWT and
is the caller. A headless render with no `params.rasters` embeds
the canonical bitmap and warns once that display dpi was not
applied. The worker has no JWT for that endpoint, and this plan
does not add a route or a service token.

`fit` is `stretch`, `contain`, `cover`, `none`, or `scale-down`.
Cover is the smallest uniform size that covers the box. The PDF
port does not clip that overflow, so the display PNG is cropped to
the box and both emitters draw that bitmap at the box size.
Contain and scale-down letterbox with `fitAlign` (`Left`, `Center`,
`Right`, default `Center`) and `fitValign` (`Top`, `Middle`,
`Bottom`, default `Middle`). Stretch fills the box. `none` keeps
the intrinsic pixels, clipped to the box. An unknown fit warns
once and uses `contain`. Flip is `none`, `horizontal`, `vertical`,
or `both` (default `none`). An unknown flip warns once and uses
`none`. Rotation is lookup 054 key 14, clockwise about the box
center. Flip runs first. Both are draw transforms and do not make
another bitmap. HTML uses `opacity` (0 to 1, default 1). PDF uses
the port's image opacity, an extended graphics state. Grayscale
and invert are baked into the display PNG because the port has no
image color matrix. Blur, shadow, brightness, contrast, hue, and
sepia stay off.

HTML contains each display PNG once, in one SVG `<defs>` image,
and each placement references that id. PDF stores that same bitmap
once through `doc:image` and references the XObject. Fifty copies
of one icon at one size are one bitmap. The emitted file should
stay on the order of kilobytes. A full-page photograph at a high
dpi is the case that is still large. Field Image (Phase 50) uses
this same registry and these same placement properties. Reference
and URL stay in Phase 51. Phase 44c does not emit PDF.

**Static image placement (revised 2026-11-04, Phase 44c, drafted).**
The paragraph above is the placement. Phase 52 consumes
`params.rasters`. A supported SVG (svg, g, path, rect, circle,
ellipse, line, polyline, polygon, with fill, stroke, stroke-width,
opacity, and a translate, scale, or rotate) is one PDF form
XObject of those paths. Script, filters, text, images, and
foreignObject are outside the subset. A zint PDF XObject is a
later phase and is embedded as given.

**Where the bytes come from.** All three source kinds are v1, and they
are all resolved to bytes at the start of generation, before any item
is placed:

| kind | Stored on the item | Resolved how | Trade-off |
| --- | --- | --- | --- |
| `embedded` | One canonical copy in `doc.images`, keyed by content hash. The item stores the hash | Already in hand | Portable, one file. One large canonical image can still push the definition over the params cap. The emitted HTML and PDF store the display bitmap once |
| `reference` | A document id | `H.altquery_sync` on the session database reads `documents` (`acuranzo_1010.lua`: `(doc_id, rev_id)`, `file_data TEXT_BIG`, `file_name`) | Swapping the document changes the report without editing it; the report is no longer portable alone |
| `url` | A URL | `H.http.request_sync` at the start of generation, the same call the endpoint step uses | Live and small; the URL must stay available, and it slows every generation |

The designer's image browser loads a file from the local filesystem
or takes a paste from the clipboard. Import converts the bitmap
and stores one `doc.images` entry. A `reference` picker lists
documents through the Phase 17 read QueryRef.
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
`dpi` get their rows in Phase 57. An embedded image stores one
canonical copy in `doc.images`. Display PNGs are preview params,
not definition keys. There is no separate downsample-cap property.
Reuse is not a property. One registry entry is one emitted bitmap.

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
| Report | `embedFonts` | `false` | `false` |
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
| Image | `image` | omitted until import stores a hash | empty frame, no warn |
| Image | `fit` | `contain` | `contain`. An unknown value warns once and uses `contain` |
| Image | `fitAlign` | `Center` | `Center` |
| Image | `fitValign` | `Middle` | `Middle` |
| Image | `flip` | `none` | `none`. An unknown value warns once and uses `none` |
| Image | `opacity` | `1` | `1` |
| Image | `grayscale`, `invert` | false | false. Either one changes the display PNG |
| Image | box on create | `1 in` by `0.75 in` | `1 in` by `0.75 in` |
| Static number | `value` | `0` | blank print, no warn |
| Static number, static datetime | `wrap` | `nowrap` | `nowrap` |
| Static datetime | `value` | `2026-01-01` | blank print, no warn |
| Static memo | `text` | `Memo` | `Memo` |
| Static memo | `wrap` | `pre-wrap` | `pre-wrap` |
| Static memo | box on create | `1.5 in` by `0.6 in` | `1.5 in` by `0.6 in` |
| Static memo | `fitHeight` | `both` | `both` |
| Static text, field text, static number, static datetime | `fitHeight` | `clip` | `clip`. An unknown value warns once and uses the type default |
| Static number, static datetime, static memo | `valign` | `top` | `top` |
| Static number, static datetime, static memo, and any item whose `fitHeight` is not `clip` | `lineHeight` | `1.2` | `1.2`. Clip-mode static text keeps the current CSS line spacing |
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
  those buttons. Phases 10, 16, 22, 23, 28, 34, 38, 44c, 50, 54, and
  58 add a button. Phase 10 shipped Text on a single row.
  Phase 44c adds Number, DateTime, Image, and Memo on the Static row.
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
  Barcode SVG must survive that sanitizer (Phase 54). Phase 44c
  puts each display PNG in one SVG `<defs>` image, and each
  placement references that id, so the sanitizer has to keep the
  reference. This is
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
`sources` wins. A query datasource with no array runs that
QueryRef on the session database (Phase 29, `query_type_a28` 3):

- emits a static report when `sources` is empty and the detail band
  has no datasource
- repeats the detail band once per row
- substitutes field items from the row, already passed through the
  value-format rules, with null as an empty string
- honors Align and the font keys. `fitHeight` `clip` keeps text
  in the item box. `grow`, `shrink`, and `both` change the printed
  row in line-height steps
- prints system user from `params._hydrogen.username` (Phase 22)
- prints system datetime from the one generation instant, shifted
  by `params._hydrogen.tzoffset` minutes, then the item mask
  (Phase 22)
- prints page number and page count as integers through the number
  mask (Phase 23), and a Page label (053 key 64) from its
  `template` with `{n}` and `{c}` (default `Page {n} of {c}`)
- prints a parameter item from `params.parameters` or the definition
  default
- breaks pages on the used height of each band instance, keeping a
  group header with its first detail row
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
| Field memo, and calculated, aggregate, and crosstab memo | Static Memo is Phase 44c (lookup 053 key 3), drafted and revised 2026-11-04. It is plain text. `fitHeight` defaults to `both`, so the printed row grows or shrinks in line-height steps. Rich text stays here. Field memo is key 23. Keys 26, 30, 31, 33, 36, 37, 39, and 53 stay hidden. Shapes are Phases 44a and 44b. Static Image is Phase 44c. Field Image, reference, and URL stay in Phases 50–52. |
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
| Text measurement and stretching bands | Phase 44c measures with the shared Helvetica AFM table. `fitHeight` grows or shrinks the printed instance in line-height steps and leaves the stored band height alone. A grown item is drawn in Helvetica. `embedFonts` carries a face and does not change that measure. Ellipsis, shrinking the font, and Vanadium widths stay here. |
| Vanadium widths | The report flag `embedFonts` is Phase 44c. Phase 31 embeds a subset TrueType when the flag is on. The width table that lets a grown line use Vanadium stays here. |
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

### 2026-11-04 — Phase 27 in progress

- Andrew chose the toast and the scope. Title `Saved to preview`.
  Description `This report is larger than the 256 KiB script limit,
  so it was saved and previewed from the stored revision.` Preview
  HTML is the only path. Compose of an over-cap definition still
  fails with `Compose failed`.
- An over-cap Preview Run saves through QueryRef 168, including a
  clean row, then calls `Reports.Render` with `report_id` and
  `rev_id`. Sources stay inline. The call does not send
  `definition`, `params._hydrogen`, or the session JWT. A failed
  save does not call Render and does not show the path toast.
- The client measures compact UTF-8 against 256 KiB minus 4 KiB.
  It does not read `hydrogen.json`.
- `acuranzo_1456.lua` copies the 1455 body. A call with no
  definition loads QueryRef 167 through `H.altquery_sync` and
  parses the report text. An inline definition still wins. A
  loaded band that omits `datasource` takes the first declared
  name. Leave `acuranzo_1455.lua` as written.
- `npm test` was 92 files, 1464 passed, 12 skipped. The skipped
  file is the live auth check; the server check aborted. `npm run
  lint` and `npm run lint:css` exited 0. `luacheck` on 1456
  reported 0 warnings. Lua 5.5.1 matched `detail.html` on the
  inline path and on the `report_id` path.
- **Next:** Andrew applies `acuranzo_1456.lua`, then the browser
  check (27.2) on https://lithium.500courses.com. The phase stays
  in progress until he reports it. Next free file **1457**.
  QueryRef **175** unused. Phase 29 waits for a new conversation.

### 2026-11-04 — Phase 27 complete

- Andrew applied `acuranzo_1456.lua` and asked to mark Phase 27
  complete. He did not quote the toast or a failed-save canvas.
- Phase 28 was already complete on 2026-11-03. This sitting did not
  reopen it.
- Phase 29 was previewed and not started. A `report_id` render with
  no `sources` is the check. The written load uses
  `query_type_a28 = 1`. QueryRef 174 is type 3. The lock's binds are
  `{ "parameter" }` or `{ "literal" }`. The designer stores plain
  values on `params`. Next free file **1457**. QueryRef **175**
  unused. Do not edit 1455 or 1456.
- **Next:** Phase 29 in 04-pdf.md, in a new conversation. Do not
  start it here.

### 2026-11-04 — Phase 29 in progress

- Andrew chose type 3 only (SQL - Reporting). Type 1 does not find
  QueryRef 174. He chose stored `datasource.params` scalars, with
  `{parameter}` and `{literal}` still accepted, and
  `params.parameters` overriding a same-named bind.
- `acuranzo_1457.lua` copies the 1456 body. Reverse restores that
  body. Leave `acuranzo_1455.lua` and `acuranzo_1456.lua` as written.
  No new QueryRef. No new lookup. Next free file **1458**.
  QueryRef **175** unused.
- An array in `sources` wins, including an empty array. A query
  datasource with no array loads `query_type_a28 = 3` and runs that
  SQL with `H.altquery_sync` on the session database. Types come
  from `collection.reporting.parameters`.
- `luacheck` on 1457 reported 0 warnings. Lua 5.5.1 probe: QueryRef
  174 with `ROW_LIMIT` 2 returned Alpha and Bravo. A supplied array
  was not queried. Overrides and the lock's bind forms held. Empty
  database, a missing connection, a missing query, a query error, a
  missing `queryRef`, and an unset parameter each warned and
  returned no rows.
- Compose and Preview HTML still send `sources`. The check is a
  `report_id` call with no `sources` and no `definition`.
- **Next:** Andrew applies `acuranzo_1457.lua`, regenerates the
  payload, and restarts Hydrogen. Then `Reports.Render` with
  `report_id`, `rev_id`, `stage` `compose`, and no `sources`. The
  log line is `datasource <name> ran query 174; 2 rows`. Do not
  start Phase 30 in that report unless the same message names it.

### 2026-11-04 — Phase 29 complete

- Andrew applied `acuranzo_1457.lua` and asked to close Phase 29.
  He did not quote the two-row log line or the Alpha and Bravo
  bands.
- A query datasource with no array in `sources` loads
  `query_type_a28 = 3` and runs that SQL with `H.altquery_sync` on
  the session database. An array in `sources` still wins. Leave
  `acuranzo_1455.lua`, `acuranzo_1456.lua`, and `acuranzo_1457.lua`
  as written. Reverse of 1457 restores the 1456 body. Next free
  file **1458**. QueryRef **175** unused. No new lookup.
- Phase 30 was previewed and not started. One migration seeds
  every `lua-pdfkit.*` row, group `lua-pdfkit`, `invokable = 0`,
  `mcp_access = 0`. The lock path is on disk: 115 Lua modules and
  14 AFM files. `font/standard.lua` still `io.open`s
  `Helvetica.afm`, so work item 30.2 bundles that text before the
  packet. `require("lua-pdfkit")` has no dot. The probe is
  `require("lua-pdfkit.init")`, then `doc:text`, `doc:finish()`,
  and `doc:output()`, with `io` left off and `AllowDBModuleLoad`
  on. The probe returns a PDF byte length. No report output in
  this phase. Phases 31 and 52 wait for the apply.
- **Next:** Phase 30 in 04-pdf.md, in a new conversation. Do not
  start it here.

### 2026-11-04 — Phase 38 in progress

- Andrew named Phase 38 and left Phases 33–37 as they are. The
  written entry gate was Phase 37. A field total does not use the
  formula compiler. Phases 30–37 and 39 stay unstarted.
- `acuranzo_1458.lua` copies the 1457 `Reports.Render` body.
  Reverse restores that body. Leave `acuranzo_1455.lua`,
  `acuranzo_1456.lua`, and `acuranzo_1457.lua` as written. No new
  QueryRef. No new lookup. Next free file **1459**. QueryRef
  **175** unused.
- An aggregate number (lookup 053 key 35) stores `op` and either
  `field` or `function`. This phase reduces `count`, `filled`,
  `unique`, and `sum` of a field. The designer places it on a
  report header, group header, group footer, or report footer.
  The raw total is `ctx.aggregates[op][field]`. The item mask
  formats it. The map stays out of the emitted JSON. A function
  name warns and prints blank until Phase 33.
- `count` counts every row. `filled`, `unique`, and `sum` skip
  nulls. An empty count, filled, or unique is 0. An empty sum
  prints blank. A non-number in sum warns once per item and is
  skipped. A string `"10"` is not added. Unique keeps a number
  and that same text distinct. Consecutive spans stay separate
  when a group key returns later. The header and the footer of
  one scope share the total.
- The fixture is `totals.definition.json` and
  `totals.sources.json`. Mask `#,##0.00`. East sum `30.00`,
  second East sum `7.00`, West sum `5.00`, South sum blank,
  report sum `42.00`. Four warns name the sum items. The grouped
  fixture still matches `grouped.intermediate.json`.
- `luacheck` on 1458 reported 0 warnings. Lua 5.5.1 probe passed.
  `npm test` was 93 files, 1472 passed, 12 skipped. The skipped
  file is the live auth check; the server check aborted. `npm run
  lint` and `npm run lint:css` exited 0.
- **Next:** Andrew applies `acuranzo_1458.lua`, regenerates the
  payload, and restarts Hydrogen. Then import the totals
  definition and Run Preview HTML. The header matches the footer.
  Do not mark Phase 38 complete until he reports that preview.
  Do not start Phase 39 in that report unless the same message
  names it.

### 2026-11-04 — Phase 38 complete

- Andrew said it works well. He said 1457 has been applied and
  the aggregate functions were tested with no issues found. He
  did not name `acuranzo_1458.lua`. He did not quote East
  `30.00`, the report `42.00`, the four sum warns, or that the
  header matches the footer. The packet for this phase is
  `acuranzo_1458.lua`. `acuranzo_1457.lua` is the Phase 29 query
  script. Leave `acuranzo_1455.lua` through `acuranzo_1458.lua`
  as written. Next free file **1459**. QueryRef **175** unused.
  No new lookup. No new QueryRef.
- Work item 38.4 is checked. A named function still prints blank
  until Phase 33. His phrase "aggregate functions" is the four
  field ops this phase reduces.
- Phase 39 (Page totals) was previewed and not started. A page
  header or page footer shows the sum of the detail rows placed
  on that page. The page header and the page footer share that
  window. The fixture spans two pages so the page sum differs
  from the report sum. The entry gate is Phase 38 and Phase 21.
  Both are complete. Today a type 35 item on a page band prints
  blank, and the designer allows bands 12, 14, 16, and 18 only.
  The next packet would be a new file **1459**. It was not
  written. Phase 40 waits on Phase 39.
- **Next:** Phase 39 in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md),
  in a new conversation. Do not start it here. Phases 30–37 stay
  on the temporary skip (custom functions, PDFs, barcodes, and
  charts).

### 2026-11-04 — Phase 39 in progress

- Andrew said "please continue" after the Phase 39 peek. Phase
  38 and Phase 21 are complete. Phases 33–37 stay on the skip.
- `acuranzo_1459.lua` copies the 1458 `Reports.Render` body.
  Reverse restores that body. Leave `acuranzo_1455.lua` through
  `acuranzo_1458.lua` as written. No new QueryRef. No new lookup.
  Next free file **1460**. QueryRef **175** unused.
- A page header and a page footer reduce the detail rows placed
  on that page. They share one total. `count`, `filled`,
  `unique`, and `sum` use those rows. The text is filled after
  the page list exists. The designer allows bands 12, 13, 14,
  16, 17, and 18. A detail band is refused.
- The fixture is `pagetotals.definition.json` and
  `pagetotals.sources.json`. Page height `2 in`, margins `0 in`,
  detail `0.5 in`, chrome bands `0.25 in`. Page 1 is count
  `2.00` and sum `30.00`. Page 2 is count `1.00` and sum
  `7.00`. The report sum is `37.00`. Grouped, totals, detail,
  page number, and static compose match the 1458 body.
- `luacheck` on 1459 reported 0 warnings. Lua 5.5.1 probe
  passed. `npm test` was 93 files, 1472 passed, 12 skipped.
  `npm run lint` and `npm run lint:css` exited 0.
- **Next:** Andrew applies `acuranzo_1459.lua`, regenerates the
  payload, and restarts Hydrogen. Then import the page-totals
  definition and Run Preview HTML. The page header matches the
  page footer, and the page sum differs from the report sum.
  Do not mark Phase 39 complete until he reports that preview.
  Do not start Phase 40 unless the same message names it.

### 2026-11-04 — Phase 39 complete

- Andrew said the latest migration applied is 1459, and that
  aggregate functions in page headers and footers work as
  indicated. He did not quote page 1 `30.00`, page 2 `7.00`,
  the report `37.00`, the counts `2.00` and `1.00`, or the
  names Ann, Bo, and Cy. The packet is `acuranzo_1459.lua`.
  Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as
  written. Next free file **1460**. QueryRef **175** unused.
  No new lookup. No new QueryRef.
- Work item 39.3 is checked. A named function still prints blank
  until Phase 33. His phrase "aggregate functions" is the field
  ops on a page header and a page footer.
- Phase 40 (Average and weighted) was previewed and not started.
  `avg` and `mean` are the same arithmetic mean. `weighted`
  stores `weight` as a field and divides the sum of field times
  weight by the sum of the weights. A non-numeric value warns
  and is skipped. A zero sum of weights, or no numeric pair
  left, prints blank and adds one warn. An empty result is
  blank. The picker still offers `count`, `filled`, `unique`,
  and `sum`. The next packet would be a new file **1460**. It
  was not written. Phase 41 waits on Phase 40.
- **Next:** Phase 40 in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md),
  in a new conversation. Do not start it here. Phases 30–37
  stay on the temporary skip (custom functions, PDFs, barcodes,
  and charts).

### 2026-11-04 — Phases 40, 41, and 42 in progress

- Andrew named Phases 40, 41, and 42 together. Phase 39 is
  complete. Phases 33–37 stay on the skip. Phase 43 has not
  started.
- `acuranzo_1460.lua` copies the 1459 body. `avg` and `mean`
  are the arithmetic mean. `weighted` stores `weight`. Reverse
  restores the 1459 body. `acuranzo_1461.lua` adds `median`.
  An even count is the mean of the two central numbers. Reverse
  restores the 1460 body. `acuranzo_1462.lua` adds `mode`. A
  tie keeps the smallest value under Lua `<` and warns. Text
  mode uses lookup 053 key 34. Reverse restores the 1461 body.
  Leave `acuranzo_1455.lua` through `acuranzo_1459.lua` as
  written. No new QueryRef. No new lookup. Next free file
  **1463**. QueryRef **175** unused.
- The mean fixture is amount/weight `10/1`, `20/1`, `30/3`.
  Avg and mean are `20.00`. Weighted is `24.00`. The log is
  empty. The median fixture is Odd `20.00` and Even `25.00`.
  The mode fixture is Labels `A` and Numbers `1.00`, with one
  tie warning. A named function still prints blank until Phase
  33.
- `luacheck` on 1460, 1461, and 1462 reported 0 warnings. The
  Lua 5.5.1 probe passed, including grouped, totals, page
  totals, detail, page number, and static against the 1459
  body. `npm test` was 93 files, 1476 passed, 12 skipped.
  `npm run lint` and `npm run lint:css` exited 0.
- **Next:** Andrew applies `acuranzo_1460.lua`, then
  `acuranzo_1461.lua`, then `acuranzo_1462.lua`. He regenerates
  the payload, restarts Hydrogen, imports each fixture, and
  runs Preview HTML. Do not mark the phases complete until he
  reports that. Do not start Phase 43.

### 2026-11-04 — Phases 40, 41, and 42 complete

- Andrew said Phases 40, 41, and 42 are fully operational and
  confirmed the values returned. He did not name
  `acuranzo_1460.lua`, `acuranzo_1461.lua`, or
  `acuranzo_1462.lua`. He did not quote Avg `20.00`, Mean
  `20.00`, Weighted `24.00`, Odd `20.00`, Even `25.00`,
  Labels `A`, or Numbers `1.00`. On his eight amounts the
  mode tie printed -17.25. The median of 42.00 and 89.99 is
  65.995, and a `#,##0.00` mask prints 66.00. That check is
  the apply report. Leave `acuranzo_1455.lua` through
  `acuranzo_1462.lua` as written. Next free file **1463**.
  QueryRef **175** unused. No new lookup. No new QueryRef.
- Work items 40.2, 41.2, and 42.2 are checked. A named
  function still prints blank until Phase 33.
- Phase 43 (Min and max) was previewed and not started.
  Numbers compare as numbers. A datetime is a civil string
  or a unix number. The first non-null value picks the kind.
  A later value of the other kind warns once and is skipped.
  A number uses key 35. A datetime uses key 38. Text uses
  key 34 and Lua `<`. The fixture binds fields. The next
  packet would be a new file **1463**. It was not written.
  Phase 44 waits on Phase 43.
- **Next:** Phase 43 in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md),
  in a new conversation. Do not start it here. Phases 33–37,
  45–48, PDFs, barcodes, and charts stay on the temporary
  skip.

### 2026-11-04 — Phases 43 and 44 in progress

- Andrew named Phases 43 and 44 together. Phase 42 is complete.
  Naming both waives Phase 44's written gate. He also described
  Static shapes: line, rectangle, or ellipse, a line side, stroke,
  fill including a gradient, stacking by `band.items` order, raise
  and lower, and outline drag and drop. That stays on the Later
  row. No shape migration was written. Phase 45 has not started.
- `acuranzo_1463.lua` adds `min` and `max`. Reverse restores the
  1462 body. `acuranzo_1464.lua` adds `first` and `last`. Reverse
  restores the 1463 body. Leave `acuranzo_1455.lua` through
  `acuranzo_1462.lua` as written. No new QueryRef. No new lookup.
  Next free file **1465**. QueryRef **175** unused.
- The min and max fixture prints `1.00`, `3.00`, `2026-01-15`,
  and `2026-03-02`. The log is empty. The first and last fixture
  prints `10.00`, `7.00`, `2026-03-02`, and `2026-01-01`. The
  null in the middle is skipped. The log is empty. A named
  function still prints blank until Phase 33.
- `luacheck` on 1463 and 1464 reported 0 warnings. The Lua 5.5.1
  probe passed, including page totals, grouped, totals, detail,
  page number, static, mean, median, and mode against the 1462
  body. `npm test` was 93 files, 1477 passed, 12 skipped.
  `npm run lint` exited 0. `npm run lint:css` exited 0.
  `templates:copy` copied 139 templates.
- **Next:** Andrew applies `acuranzo_1463.lua`, then
  `acuranzo_1464.lua`. He regenerates the payload, restarts
  Hydrogen, imports each fixture, and runs Preview HTML. Do not
  mark the phases complete until he reports that. Do not start
  Phase 45.

### 2026-11-04 — Phases 43 and 44 complete

- Andrew said Phases 43 and 44 look good. He did not name
  `acuranzo_1463.lua` or `acuranzo_1464.lua`. He did not quote
  min `1.00`, max `3.00`, `2026-01-15`, `2026-03-02`, first
  `10.00`, last `7.00`, `2026-03-02`, or `2026-01-01`. That
  check is the apply report. Leave `acuranzo_1455.lua` through
  `acuranzo_1464.lua` as written. Next free file **1465**.
  QueryRef **175** unused. No new lookup.
- Work items 43.1, 43.2, 44.1, and 44.2 are checked. A named
  function still prints blank until Phase 33.
- He asked to rename the property row `Op` to `Summarize`, to
  sort the list, and to capitalize the entries. The list is
  `AGGREGATE_OPS` in the designer, not a lookup. Sorting lookup
  054 does not reorder it. Stored codes stay lowercase because
  the script matches `min`, `max`, `first`, and `last`. The
  label and the capitals were left as he previewed them.
- Phases 44a (Static shapes) and 44b (Shape fill and gradient)
  are written in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md)
  and have not started. They are not extra numbers in the 0–71
  count. Memos stay on the Later row.
- **Next:** Phase 44a in that file, in a new conversation. Do
  not start it here. Do not start Phase 45. Phases 33–37,
  45–48, PDFs, barcodes, and charts stay on the temporary skip.

### 2026-11-04 — Phase 44a in progress

- Phase 44 is complete. This sitting is Phase 44a. Phase 44b
  has not started. Phase 45 has not started.
- `acuranzo_1465.lua` seeds lookup 078, Report Shape Kinds.
  Keys 1–4 are line, rectangle, roundrect, and ellipse.
  Keys 5–9 are x, trapezoid, star, pentagon, and hexagon.
  Lookup 054 keys 41–51 are the shape attributes. Lookup 053
  key 25 drops key 24 and lists keys 41–51. Keys 26, 33, 39,
  and 53 stay as they are. Reverse restores the 1086 key 25
  collection, including key 24. No new QueryRef.
- `acuranzo_1466.lua` copies the 1464 `Reports.Render` body.
  typeKey 25 passes through compose. `stage=html` draws an
  SVG. PDF does not draw the shape. Reverse restores the 1464
  body. Leave `acuranzo_1455.lua` through `acuranzo_1464.lua`
  as written. Next free file **1467**. QueryRef **175**
  unused. Next free lookup id **079**. Next free lookup 054
  key **52**.
- The fixture is `tests/fixtures/reporting/shapes.definition.json`.
  Lua 5.5.1 printed the rectangle `#d3d3d3`, the rounded
  rectangle `#c8c8c8` with a cubic, the bottom line
  `M 0 24 L 96 24`, the hexagon `#778899` at
  `rotate(30 38.4 38.4)`, and `#112233` before `#abcdef`.
  `luacheck` on 1465 and 1466 reported 0 warnings. `npm test`
  was 94 files, 1491 passed, 12 skipped. `npm run lint`
  exited 0. `npm run lint:css` exited 0. `templates:copy`
  copied 140 templates.
- Work items 44a.1 through 44a.4 are checked. Work item 44a.5
  stays open until Andrew sees the preview. Dashed is four
  stroke widths on and two off, butt cap. Dotted is one on and
  two off, round cap. An outline drop takes the target row's
  index. Raise is `fa-bring-forward`. Lower is
  `fa-send-backward`.
- The Shape toolbar control is a button. Click and drag both
  add a rectangle. The closed Shape cell shows the list label.
  Stroke, Fill, and Corners are lookup 075 keys 10–12. The
  property header centers Move backward (`fa-send-backward`),
  Move to back (`fa-send-back`), Move forward
  (`fa-bring-forward`), and Move to front (`fa-bring-front`).
  Shapes omit Align. Object starts with a read-only id, and
  the outline shows that id with the name. Link is the last
  Object row. Notes is a client text block stored as `notes`.
  A blank value is omitted. The Notes editor uses the same
  transparent field and inherited text colour as the other
  property inputs. A new item id is a word and a number
  (`text_1`, `shape_1`, `textfield_1`, `number_1`,
  `datetime_1`, `date_1`, `user_1`, `agg_1`, `calctext_1`,
  `calcnum_1`, `calctime_1`, `chart_1`, `barcode_1`,
  `pagenum_1`, `pagecount_1`, `pagelabel_1`, `param_1`).
  Each word has its own count. Aggregate text, number, and
  datetime share `agg_`. Band ids stay `ph1` and `d1`. An
  id already stored is kept. The outline stays text. No
  migration. `acuranzo_1465.lua` is still the packet. If it
  was already applied, the next
  file is 1467. `npm test` was 94 files, 1499 passed, 12
  skipped. `npm run lint` and `npm run lint:css` exited 0.
  `luacheck` on 1465 was already 0 warnings and that file
  was not edited again.
- Andrew reports `acuranzo_1466.lua` is already applied
  (2026-11-04). Do not edit it and do not apply it again.
  The toolbar tweaks do not change `Reports.Render`. The next
  change to that script is a new file whose old body is the
  1466 body.
- **Next:** Andrew applies `acuranzo_1465.lua` if it is not
  applied. That file was edited in place for Stroke, Fill,
  and Corners. If 1465 was applied before that edit, the
  follow-up is **1467**. Otherwise 1467 stays free. He
  regenerates the payload, restarts Hydrogen, imports the
  shapes fixture, and runs Preview HTML on
  `https://lithium.500courses.com`. The phase stays in
  progress until he reports that preview. Do not start
  Phase 44b. Do not start Phase 45. Phases 33–37, 45–48,
  PDFs, barcodes, and charts stay on the temporary skip.

### 2026-11-04 — Phase 44a complete

- Andrew said everything else looks good and asked to wrap
  Phase 44a. He did not quote the fixture colours. Phase 44b
  has not started. Phase 45 has not started.
- The Notes editor names `--text-primary` for the text and
  the caret. The field stays transparent. `color: inherit`
  painted black text on the dark row. `reports.css` stays at
  1048 lines. The rule is in `reports-shape.css`.
- New item ids stay `text_1`, `shape_1`, and the other words
  from the in-progress entry. Band ids stay `ph1` and `d1`.
  An id already stored is kept.
- Andrew says 1466 is the last migration he sees and it is
  applied. There is no `acuranzo_1467.lua`. That number is
  the next free slot. Phase 44b writes it when it changes
  `Reports.Render`. Do not edit 1466. `acuranzo_1465.lua` is
  the lookup packet, the file before 1466. QueryRef **175**
  stays unused. Next free lookup id **079**. Next free
  lookup 054 key **52**.
- `npm test` reported 1498 passed, 1 failed, and 12 skipped.
  The failure was the JSON fold wait in
  `reports-chrome.test.js`. A rerun of that file passed 19
  tests. `npm run lint` exited 0. `npm run lint:css` exited
  0. happy-dom computed the notes text as `rgb(224, 224, 224)`
  on a transparent field while the cell colour was black.
  The designer was not opened in a browser.
- **Next:** Phase 44b in a new conversation. Do not start it
  here. Its script is **1467** when that phase writes one.
  There is no such file yet. Do not start Phase 45. Phases
  33–37, 45–48, PDFs, barcodes, and charts stay on the
  temporary skip.

### 2026-11-04 — Phase 44b in progress

- Andrew said the notes editor is much better and named Phase 44b.
  Phase 44a stays complete. Phase 45 has not started.
- `acuranzo_1467.lua` is written and not applied. `sql_old` is the
  1466 body. `sql_new` draws the fill. Lookup 054 key 46 lists
  none, solid, grid, dots, hatched, and noise. Reverse restores
  the 1466 body and the none, solid list. 1465 and 1466 were not
  edited. Do not write 1468. QueryRef **175** stays unused. Next
  free lookup id **079**. Next free lookup 054 key **52**.
- Work items 44b.1 through 44b.3 are checked. Work item 44b.4
  stays open until Andrew sees the preview. The fixture is
  `shape-fill.definition.json`. Hatch is linear `#1d4e89` to
  `#f0c808`. Speckle is a noise ellipse, solid `#2266aa`. A
  second run matches. PDF does not draw the shape.
- Lua 5.5.1 and the designer printed the same SVG for the hatch,
  the noise ellipse, a solid rectangle, a radial paint, a rotated
  hatch, and a line. `npm test` was 95 files, 1506 passed, 12
  skipped. `npm run lint` and `npm run lint:css` exited 0.
  `luacheck` on 1467 reported 0 warnings. The designer was not
  opened in a browser. The live preview was not opened.
- **Next:** Andrew applies `acuranzo_1467.lua`, regenerates the
  payload, restarts Hydrogen, imports the fill fixture, and runs
  Preview HTML on `https://lithium.500courses.com`. The phase
  stays in progress until he reports that preview. Do not start
  Phase 45. Phases 33–37, 45–48, PDFs, barcodes, and charts stay
  on the temporary skip.

### 2026-11-04 — Phase 44b complete

- Andrew said the gradients render on Preview HTML, everything
  tested passes, and 1467 is the latest migration applied. He did
  not quote the fixture colours. Phase 44b is complete. Phase 45
  has not started.
- Leave `acuranzo_1455.lua` through `acuranzo_1467.lua` as written.
  The next free Acuranzo file is **1468**. QueryRef **175** stays
  unused. Next free lookup id **079**. Next free lookup 054 key
  **52**.
- Work items 44b.1 through 44b.4 are checked. The fixture is
  `shape-fill.definition.json`. Hatch is linear `#1d4e89` to
  `#f0c808`. Speckle is a noise ellipse, solid `#2266aa`.
- Phase 45's written gate is met. The fixture groups by a text
  function. Phase 33 has not started, and a named function still
  prints blank. The one-field group stays `band.field`. Do not
  start Phase 45 until he names it.
- **Next:** a new conversation. Name Phase 33 to start the
  compiler, or name Phase 45 to waive that gate. Do not write
  1468 until that phase changes `Reports.Render`. Phases 33–37,
  46–48, PDFs, barcodes, and charts stay on the temporary skip.

### 2026-11-04 — Phase 44c drafted

- Andrew asked for the rest of the static catalog. Phase 44c is
  drafted in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md)
  and has not started. Naming it starts the phase. No source was
  edited. `acuranzo_1468.lua` was not written.
- The phase places lookup 053 key 1 Static Number, key 4 Static
  DateTime, key 2 Static Image, and key 3 Static Memo. Preview
  HTML draws them. PDF does not. Phase 31 and Phase 52 consume
  the same placement.
- A static image keeps the original data URI. JPEG, PNG, and SVG
  stay themselves. GIF and WebP stay for HTML. ICO, BMP, and TIFF
  are refused. Fit is stretch, contain, cover, none, or
  scale-down. Flip is none, horizontal, vertical, or both, and it
  runs before rotation. Filters are opacity, grayscale, and
  invert. Blur, shadow, brightness, contrast, hue, and sepia stay
  off. An imported SVG is an `img`, and Phase 52 rasterizes it.
- A memo is plain text. Wrap stays inside the box. The band does
  not grow. Shrink-to-fit, ellipsis, and rich text stay in Later.
- Field Image stays in Phase 50. Reference and URL stay in Phase
  51. Field memo stays unnumbered. Phase 45 is not gated on 44c.
- **Next:** name Phase 44c to build it, or correct a pin first.
  The pins to correct are the band-growth refusal, plain-text
  memo, the five image types, and the three filters. Do not write
  1468 until that phase changes `Reports.Render`. Phases 33–37,
  46–48, PDFs, barcodes, and charts stay on the temporary skip.

### 2026-11-04 — Phase 44c revised

- Andrew disagreed with the first draft. A wrapping line must be
  able to grow, and to shrink, its own row. An invoice line wraps.
  Extra height is not reserved on every line. Growth moves in whole
  line-heights from one shared break. HTML and PDF paint those lines.
- Image fit and transforms are the ones HTML and PDF can both draw.
  Cover is cropped into the display PNG because the PDF port does
  not clip it. Opacity is the port's extended graphics state. Flip
  and rotation are the draw.
- HTML and PDF need not match pixel for pixel. They consume the same
  lines and the same image ids.
- Import accepts JPEG, PNG, SVG, GIF, WebP, ICO, BMP, and TIFF.
  Bitmaps convert at import. JPEG with no alpha stays JPEG. The
  other bitmaps become PNG. GIF keeps the first frame. ICO keeps
  its largest frame. SVG stays SVG. Bytes live once in `doc.images`.
  A history step that moves the item stores the hash.
- Preview scales each bitmap to a display PNG at the item dpi. HTML
  embeds that PNG once and references it. PDF stores one XObject
  and references it. Emitted files should stay on the order of
  kilobytes. A full-page photograph at a high dpi is the case that
  is still large.
- A supported SVG stays a vector in the file, in HTML, and as one
  PDF form XObject. Outside that subset, PDF uses a display PNG
  until the subset grows. The file still holds the SVG. A zint
  XObject is later and is embedded as given.
- The phase body in
  [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md)
  already followed this review. The locks, the defaults, Phase 50,
  Phase 52, and Phase 31's text item now follow it. No source was
  edited. `acuranzo_1468.lua` was not written.
- **Next:** name Phase 44c to build it, or correct a pin. The names
  still open to a change are `fitHeight`, memo default `both`
  against field-text default `clip`, opaque JPEG staying JPEG,
  dpi `72`, `lineHeight` `1.2`, and Helvetica as the measure face.
  Do not write 1468 until that phase changes `Reports.Render`.
  Phases 33–37, 46–48, PDFs, barcodes, and charts stay on the
  temporary skip.

### 2026-11-04 — Phase 44c font embed

- Andrew asked for a Font Embed flag, the same kind of choice as
  carrying an image inside the output. The report property is
  `embedFonts`, default `false`. No new lookup. The bytes stay
  out of the definition and out of history.
- HTML attaches each used family once as a base64 WOFF2
  `@font-face`, from `params.fonts`. PDF, in Phase 31, attaches
  each used non-standard family once as the port's subset
  TrueType. A standard face is never attached. WOFF2 is the HTML
  container. The full Vanadium WOFF2 is about 1.5 MB, so the
  attached bytes are a subset of the glyphs the report uses.
- The flag does not change the Helvetica measure. A grown item
  stays in that face until its family's width table exists.
- No source was edited. `acuranzo_1468.lua` was not written.
- **Next:** name Phase 44c to build it, or correct a pin. The
  names still open to a change are `fitHeight`, `embedFonts`,
  memo default `both` against field-text default `clip`, opaque
  JPEG staying JPEG, dpi `72`, `lineHeight` `1.2`, and Helvetica
  as the measure face. Do not write 1468 until that phase changes
  `Reports.Render`. Phases 33–37, 46–48, PDFs, barcodes, and
  charts stay on the temporary skip.

### 2026-11-04 — Phase 44c in progress

- Andrew named Phase 44c. Phase 44b stays complete. Phase 45
  has not started.
- `acuranzo_1468.lua` is written and not applied. `sql_old` is
  the 1467 body. `sql_new` places static number, datetime,
  image, and memo, measures wrap with the Helvetica AFM table,
  and shares one SVG image. Lookup 053 keys 1, 2, 3, and 4
  point at the static-text attribute list. Reverse restores
  the 1467 body and the 1086 lists. 1455 through 1467 were not
  edited. The next free Acuranzo file is **1469**. Do not write
  it until a later `Reports.Render` change. QueryRef **175**
  stays unused. Next free lookup id **079**. Next free lookup
  054 key **52**.
- Work items 44c.1 through 44c.11 are checked. The fixture
  prints `1,234.50` and `2026-Mar-05 14:07`. One detail row is
  12 pt and the other is 24 pt. Two placements share one PNG.
  `embedFonts` false adds no `@font-face`. The detail HTML
  fixture matches the 1467 body.
- `npm test` was 96 files, 1517 passed, 12 skipped. `npm run
  lint` and `npm run lint:css` exited 0. `luacheck` on 1468
  reported 0 warnings. `luac -p` parsed the wrapper and the
  script body. The Lua 5.5.1 probe passed. The designer was
  not opened in a browser. The live preview was not opened.
- **Next:** Andrew applies `acuranzo_1468.lua`, regenerates the
  payload, restarts Hydrogen, and checks Preview HTML on
  `https://lithium.500courses.com`. The phase stays in progress
  until he reports that preview. Do not start Phase 45. Phases
  33–37, 46–48, PDFs, barcodes, and charts stay on the
  temporary skip.

### 2026-11-04 — Phase 44c image file and toolbar

- The static toolbar order is number, datetime, text, memo,
  shape, image. Dropping an image on a band opens the same
  file prompt as the Image button. The item stores `fileName`.
  Clicking that File row opens the prompt again. The script
  does not read `fileName`. No new lookup key. 1468 was not
  edited.
- `POST reporting/image_scale` returned 503 `Reporting
  disabled` / `Reporting subsystem is not enabled` because
  `~/pintura/hydrogen/hydrogen-pintura.json` had no Reporting
  section. `Reporting.Enabled` is now true. Hydrogen was not
  restarted. A restart is required before the endpoint accepts
  a file. AutoMigration is true on that config.
- A failed import toasts `details`, then `error`, then the
  generic message. Phase 44c stays in progress. Do not start
  Phase 45.

### 2026-11-05 — Phase 44c duotone SVG

- The preview shield keeps path `opacity`, `fill-opacity`,
  `stroke-opacity`, `fill-rule`, and `currentColor`. A duotone
  rule `.fa-secondary { opacity: .4 }` is copied onto the path.
  The style element is dropped. SVG text stays out. `defs` and
  `style` no longer make `acuranzo_1468.lua` skip the icon.
  1468 is still not applied. Do not start Phase 45.

### 2026-11-05 — Phase 44c image source note

- Choosing an image writes `Source: <path>` into Notes when
  Notes are empty or already only that line. A later image
  updates that line. Any other note stays. `fileName` stays
  the file name. The path is `file.path` when the host sets
  it, otherwise the dialog value. `C:\fakepath\` contributes
  the file name. The script does not read `notes`. No
  migration. Phase 44c stays in progress. Do not start
  Phase 45.

### 2026-11-05 — Phase 44c complete

- Andrew said the new version renders a Font Awesome duotone
  icon and asked to close the phase. He accepted the browser
  hiding the directory. He did not name `acuranzo_1468.lua`
  and he did not quote `1,234.50` or `2026-Mar-05 14:07`.
  Leave `acuranzo_1455.lua` through `acuranzo_1468.lua` as
  written. The next free Acuranzo file is **1469**. QueryRef
  **175** stays unused. Next free lookup id **079**. Next free
  lookup 054 key **52**.
- Phase 45 was previewed and not started. A group band may
  set `groupKey` to a text or number function. `band.field`
  stays the path with no Lua. The fixture groups by the
  pack-code month letter, and that compiler is Phase 33.
  Naming Phase 45 starts it. Phases 33–37, 46–48, PDFs,
  barcodes, and charts stay on the temporary skip.

### 2026-11-05 — Phases 53 and 54 in progress

- lua-zint and luatz are packed under Hydrogen `payloads/lua/`,
  not seeded into `scripts`. The payload searcher sits at
  `package.searchers` index 2, always, before the file searcher.
  `require("zint.barcode")` loads `lua/zint/barcode.lua`.
  `require("luatz.tzcache")` loads `lua/luatz/tzcache.lua`. A
  payload file wins over a scripts row. `AllowDBModuleLoad` stays
  off for those two. `io` stays off, so zoneinfo still falls back
  to UTC. `payload-generate.sh` is 4.6.0. `mkt` does not refresh
  the archive.
- `acuranzo_1469.lua` is invokable `zint.Svg`.
  `acuranzo_1470.lua` is lookup 079 and the symbology attribute.
  `acuranzo_1471.lua` amends Reports.Render. None is applied.
  Leave `acuranzo_1455.lua` through `acuranzo_1468.lua` as
  written. `acuranzo_1472.lua` is the free-rectangle barcode
  fit and is not applied. The next free Acuranzo file is
  **1473**. QueryRef
  **175** stays unused. Next free lookup id **080**. Next free
  lookup 054 key **53**.
- Static/Barcode sits immediately left of Image. The design
  canvas calls `zint.Svg`. Preview HTML calls
  `require("zint.barcode")`. The box keeps its aspect. Bar height
  is the zint `height` option. PDF Form XObject is a ready call
  and is not `stage=pdf`.
- `zsh -ic 'mkt'` exited 0: Unity build, shutdown test, 3244
  functions, 349 dead. The new payload functions are not on the
  dead list. `zsh -ic 'mkp'` exited 0: cppcheck, 2,262 files, no
  issues. `npm test` was 97 files, 1534 passed, 12 skipped.
  `npm run lint` and `npm run lint:css` exited 0.
  `npm run templates:copy` copied 142 templates. The Lua 5.5.1
  probe, luacheck of 1469–1471, and shellcheck of
  `payload-generate.sh` exited 0.
- Phases 53 and 54 stay in progress until the payload is
  rebuilt, Hydrogen is restarted, the three packets are applied,
  and the 500 Courses check shows a UPC on the canvas and in
  Preview HTML. Do not start Phase 55, Phase 30, or Phase 45.
  Do not deploy. Do not apply. Do not restart the pintura
  Hydrogen.

### 2026-11-05 — Barcode rectangle is free

- Andrew said the barcode works. The rectangle is no longer
  locked to the symbol ratio. Fit is contain, cover, none, or
  scale-down. The symbol keeps its aspect inside the box.
  Stretch is not a fit. `acuranzo_1472.lua` teaches Preview
  HTML the same fit and is not applied. The next free Acuranzo
  file is **1473**. QueryRef **175** stays unused.
- The payload generator does not watch lua-zint-svg. It packs
  `payloads/lua/zint`. That snapshot is still lua-zint-svg
  0.1.0 commit `89a8231`, plus `barcode.lua`. The library repo
  is at 0.2.0, and the TrueType handoff is uncommitted
  (`fonts_ttf.lua`, `pdf.lua`). A rebuild does not pick it up
  until that tree is copied in. GS1-128 data is square-bracket
  Application Identifiers, for example
  `[01]09501101020917[10]AB-123`. Phases 53 and 54 stay in
  progress. Do not start Phase 55.

### 2026-11-05 — Barcode samples and a field barcode

- Each symbology has an example Data value. Changing symbology
  writes that example only when Data is empty or still an
  example. A typed value stays. QR is `https://500courses.com`.
  Code 39 is `CODE-39`. Code 128 is `Ab-129`. UPC-A is
  `12345678901`. GS1-128 is `[01]09501101020917[10]AB-123`.
- Field Barcode is on the Field row. It uses the same symbol
  properties. Data comes from Datasource and Field. A new one
  takes the band's datasource, or the first datasource, and
  leaves Field empty. The design canvas encodes the first
  sample row of that datasource. Export is refused until both
  are set. Changing symbology does not write an example onto
  a field barcode.
- `acuranzo_1473.lua` adds attribute 8 to lookup 053 key 45.
  `acuranzo_1474.lua` reads `item.datasource` and `item.field`.
  The iterating datasource uses the current row. Any other
  datasource uses its first row. 1474 also copies `item.fit`
  onto the symbol. 1472 did not, so Preview HTML fit waits
  on 1474. Neither file is applied. The next free Acuranzo
  file is **1475**. QueryRef **175** stays unused. Next free
  lookup id **080**. Next free lookup 054 key **53**. Phases
  53 and 54 stay in progress. Do not start Phase 55.

### 2026-11-05 — Field memo, image, and shape

- The Field row matches the static row: Number 21, Datetime 24,
  Text 20, Memo 23, Shape 26, Barcode 45, Image 22. Each new
  item takes the band's datasource, or the first datasource, and
  leaves Field empty. Datasource and Field use the barcode row
  rule. The iterating datasource uses the current row. Any other
  datasource uses its first row. A text, number, or datetime
  field with no datasource still uses the current row. Export
  of 22, 23, 26, and 45 requires both. Export of 20, 21, and 24
  still requires only Field.
- A field image cell is `data:image/png;base64,...` or
  `data:image/svg+xml;base64,...`. The picture is that URI.
  The design canvas paints it. Preview HTML feeds it to
  `emit_image`. An invalid URI warns once and the picture is
  skipped. A field memo prints the cell as plain text, wraps,
  and can grow. A field shape cell replaces the designed kind
  when the value is a known kind code, including a lookup 078
  code `shape_source` accepts. Any other string keeps the
  designed kind, with no extra warning. A missing binding draws
  the designed kind and warns once.
- `acuranzo_1475.lua` is the lookup packet. `acuranzo_1476.lua`
  amends Reports.Render. `sql_old` is the 1474 body. Andrew
  applies 1475, then 1476. The next free Acuranzo file is
  **1477**. QueryRef **175** stays unused. Next free lookup id
  **080**. Next free lookup 054 key **53**. Phases 53 and 54
  stay in progress. Phase 50 stays not started. Do not start
  Phase 55. The designer was not opened in a browser.
- `npm test` was 98 files, 1539 passed, 12 skipped. `npm run
  lint` and `npm run lint:css` exited 0. luacheck on 1475 and
  1476 reported 0 warnings. Lua 5.5.1 `loadfile` of both
  migrations exited 0. The 500 Courses check is Andrew's.

### 2026-11-05 — Barcode property sections

- A barcode's zint options leave the shared Formatting section.
  Between Object and Position the headers are Barcode Placement,
  Barcode Text, Barcode Colors, and Barcode Symbology. The
  Symbology picker, Data, Reader init, and Escape stay in Object.
  Check digit sits with the other symbology options. Code 39 and
  GS1-128 hide Barcode Symbology. The names show before the
  lookup is applied.
- `acuranzo_1477.lua` adds lookup 075 keys 13–16. Apply it after
  1475 and 1476. The next free Acuranzo file is **1478**.
  QueryRef **175** stays unused. Next free lookup id **080**.
  Next free lookup 054 key **53**. Phases 53 and 54 stay in
  progress. Do not start Phase 55. The designer was not opened
  in a browser.
- `npm test` was 98 files, 1540 passed, 12 skipped. `npm run
  lint` and `npm run lint:css` exited 0. luacheck on 1477
  reported 0 warnings. Lua 5.5.1 `loadfile` exited 0.

### 2026-11-05 — Static barcode Data is multiline

- Static barcode Data uses the Notes editor. Enter inserts a
  newline. Ctrl or Cmd+Enter commits. Arrow keys, Home, End,
  Page Up, and Page Down stay in the field. A field barcode has
  no Data row. No migration. The designer was not opened in a
  browser. `npm test` was 98 files, 1540 passed, 12 skipped.
  `npm run lint` and `npm run lint:css` exited 0.

### 2026-11-05 — Barcode symbology list

- The picker lists the lua-zint-svg set plus PDF417, Aztec,
  MaxiCode, and USPS Intelligent Mail. USPS is `uspsimail`.
  POSTNET, PLANET, Compact PDF417, and Aztec Rune stay out.
  A symbology change still writes the sample only when Data is
  empty or still a sample. UPC-E check digit `supplied` calls
  `upcechk`. Guard descent also covers UPC-E, EAN-8, EAN-13,
  and ISBN. GS1 parentheses shows on GS1-128 and both DataBar
  Expanded symbols. Data Matrix has version, fast, square,
  DMRE, ISO 144, GS separator, GS1, dotty, and dot size.
  Expanded Stacked has columns and rows. Interleaved 2 of 5
  has a check-digit list. PDF417 has columns, rows, error
  correction, and fast. Aztec has size, error correction,
  fast, full range, GS1, and extra escape. MaxiCode has mode,
  SCM version, and primary. The local tree encodes the
  twenty-three. PDF417, Aztec, MaxiCode, and USPS return
  Error 206. `options.lua` does not read secure, mode,
  primary, scmvv, or azfull, and it stores rows only for
  DataBar Expanded Stacked. The panel still sends those keys.
  `acuranzo_1478.lua` adds lookup 079 keys 6-25.
  `acuranzo_1479.lua` amends Reports.Render. sql_old is the
  1476 body. Apply 1475, then 1476, then 1477, then 1478,
  then 1479. Do not apply 1476 after 1479. The next free
  file is 1480. The payload is still lua-zint-svg 0.1.0.
  The designer was not opened in a browser. `npm test` was
  98 files, 1541 passed, 12 skipped. `npm run lint` exited 0.
  luacheck on 1478 and 1479 reported 0 warnings. Lua 5.5.1
  `load` of both render bodies exited 0. The implemented
  samples encoded. Phases 53 and 54 stay in progress.

### 2026-11-05 — Symbology list sorted

- The picker lists the lua-zint-svg 1.0.0 rockspec set, sorted by
  name with digits as numbers. Compact PDF417 is `pdf417comp`.
  `pdf417trunc` is that same symbol. UPC-A and UPC-E with the
  check digit supplied stay the Check digit row (`upcachk`,
  `upcechk`). Code 128 (suppress C) stays its own row. EAN-2
  add-on, EAN-5 add-on, and NVE-18 (SSCC) use the rockspec names.
  Compact PDF417 shares columns, rows, error correction, and fast
  with PDF417. The sample is `Hello`. Aztec also shows Dotty and
  Dot size. `options.lua` reads secure, mode, primary, scmvv, and
  PDF417 rows, including Compact PDF417. It does not read
  `azfull`. Aztec does not read `--fast`. `acuranzo_1478.lua`
  adds lookup 079 keys 6-26 and sets `sort_seq`. Keys 1, 4, and
  5 keep their codes. `acuranzo_1479.lua` adds `pdf417comp` to
  Reports.Render. sql_old is still the 1476 body. Apply 1475,
  then 1476, then 1477, then 1478, then 1479. Do not apply 1476
  after 1479. The next free file is 1480. The payload source
  matches lua-zint-svg `97ded32`. The pintura binary from 14:55
  does not contain `pdf417comp`. The designer was not opened in
  a browser. `npm test` was 98 files, 1541 passed, 12 skipped.
  `npm run lint` exited 0. luacheck on 1478 and 1479 reported 0
  warnings. Lua 5.5.1 `load` of both render bodies exited 0. The
  samples encoded, including Compact PDF417. Phases 53 and 54
  stay in progress.

### 2026-11-05 — Phase 31-UI written

- A PDF page dictionary may set `/Rotate` to 0, 90, 180, or 270.
  That is which side is up. The content stream is written upright.
  An open action and `/ViewerPreferences` are not orientation. A
  scan stored already upside down, with `/Rotate` 0, does not
  announce that. Phase 31-UI honors `/Rotate`, then applies the
  remembered quarter-turns and flips on top. Those are view
  transforms. The file is not rewritten.
- Andrew asked for the Preview PDF viewer phase. Phases 44a, 44b,
  and 44c already name the shape and static-item work, so this
  one is Phase 31-UI in
  [`/docs/Li/plans/reporting/04-pdf.md`](/docs/Li/plans/reporting/04-pdf.md).
  It is not started. It does not change the count of 72. The page
  loads a fixture that lives with the report fixtures. The library
  is bundled `pdfjs-dist`. `jsPDF` stays out. The page does not
  call `Reports.Render` and does not emit `stage=pdf`. No
  migration. Naming Phase 31-UI in a new chat starts it.
- Phases 53 and 54 stay in progress. Do not start Phase 30, Phase
  31, Phase 45, Phase 50, or Phase 55. The next free Acuranzo file
  is 1480. QueryRef 175 stays unused.

### 2026-11-05 — Phase 31-UI in progress

- Preview PDF is built and the phase stays in progress.
  31-UI.12 stays unchecked. Andrew checks
  `https://lithium.500courses.com`: fit a page, zoom out, move
  through the thumbs, open the outline and jump, search, select
  text, and copy a marquee. PDF to text and OCR stay disabled.
- The fixture is
  `elements/003-lithium/public/assets/documents/test.pdf`,
  served at `/assets/documents/test.pdf`. It is untracked.
  pdf.js 6.5.438 reads 7 pages and 23 nested outline entries.
  Every destination is `/Fit`, so a click opens that page. A
  destination that names a point still scrolls to that point.
  Page 7 `/Rotate` is 90. Page 4's sideways line is content on
  an upright Legal page.
- The library is bundled `pdfjs-dist` 6.5.438. Lithium draws
  the chrome. The worker is bundled. `vite.config.js` isolates
  it as `vendor-pdfjs`. The page does not call
  `Reports.Render` and does not emit `stage=pdf`. No migration.
- Numeric 100% is 96 CSS pixels per inch (`96/72` on the pdf.js
  viewport scale). Fit page is the control that fills the pane,
  and that scale stays unclamped. The designer's 100% stays on
  Editor.
- From `elements/003-lithium`, `npm test` was 99 files, 1552
  passed, 12 skipped. `npm run lint` and `npm run lint:css`
  exited 0. `npm run templates:copy` copied the report HTML and
  CSS. The viewer was not opened in a browser. One earlier full
  run saw the JSON fold count stay 0. The chrome file alone and
  the second full run both passed. The assertion was left as
  written.
- Phases 53 and 54 stay in progress. Do not start Phase 30,
  Phase 31, Phase 45, Phase 50, or Phase 55. The next free
  Acuranzo file is 1480. QueryRef 175 stays unused. Lookup 080
  and lookup 054 key 53 stay free.

### 2026-11-05 — Preview PDF worker path

- The fixture was already `/assets/documents/test.pdf`. Deploy
  copies `public/assets/documents/test.pdf`. Vite does not hash
  that file. The failure was the pdf.js worker. Vite had emitted
  `/assets/pdf.worker.min-CIqJB0yW.mjs`. Hydrogen serves the
  file and does not label `.mjs` as JavaScript, so the browser
  refused the module.
- `scripts/copy-pdf-worker.js` copies the worker to
  `assets/pdf.worker.min.js`. `deploy:local`, `deploy`, and
  `deploy:500courses` run it. `npm run dev` runs it into
  `public/` first. The viewer sets `workerSrc` to that path.
- Log sits before Preview HTML and Preview PDF.
- The running copy at `~/pintura/web` was patched in place.
  Its service worker cache is 3498 so the old worker URL is
  dropped. A refresh loads the stable worker. The phase stays
  in progress. 31-UI.12 stays unchecked.

### 2026-11-05 — Preview PDF thumbnail frame

- The thumbnail strip uses the property panel's padding, header,
  and footer. Zoom, in 5 percent steps, is in the header. Column
  count is in the footer. The upper toolbar keeps one thumbnails
  toggle.
- Every page uses one cell. The page is contained and centered.
  The page number sits on the bottom row. At 100% one row fills
  the frame's body, so that row has no gap around it. A larger
  percent scrolls.
- A saved column count of 0 closes the strip and keeps one
  column. A saved thumb size of 64, 96, 128, or 176 pixels
  becomes 100%.
- From `elements/003-lithium`, `npm test` was 99 files, 1553
  passed, 12 skipped. `npm run lint` and `npm run lint:css`
  exited 0. `npm run templates:copy` copied 143 templates.
  The viewer was not opened in a browser. The phase stays in
  progress. 31-UI.12 stays unchecked. A refresh of the running
  copy at `~/pintura/web` does not show this until he deploys.
