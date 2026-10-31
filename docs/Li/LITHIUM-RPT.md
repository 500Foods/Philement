# Report contract

Format version 1, as approved in Phase 0 of
[`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
on 2026-10-10. This note restates the definition, the intermediate
document, the HTML emit, the `Reports.Render` call, and the Defaults
table. The checked-in fixtures are the files Phases 2, 19, and 20
test against.

The locks in that plan are the behavior. Where a lock named an object
and not the JSON key, this note records the key Phase 1 used. Reading
this note accepts those keys. After that, the fixtures are the oracle.
If this note and the plan disagree about a behavior, the plan wins and
this note gets a correction.

Phase 1 does not add an HTML file, a page-split fixture, a PDF, or a
chart fixture. HTML is Phase 24. Page split is Phase 21. PDF is
Phase 31. Chart fixtures are Phase 57. The rest of the mask table,
beyond `1234.5` and a null, is a second fixture in Phase 20.

## Fixtures

Directory:
[`/elements/003-lithium/tests/fixtures/reporting/`](/elements/003-lithium/tests/fixtures/reporting/).

A sources file is the object passed as `params.sources`. It is not
wrapped in a call. Each definition is US Letter portrait, `8.5 in` by
`11 in`, with `0.25 in` margins. That is the page size Phase 2 parses.
Every case is one page. `log` on these files is `[]`.

| Case | Definition | Sources | Structural intermediate | Formatted intermediate |
| --- | --- | --- | --- | --- |
| Static | [`static.definition.json`](/elements/003-lithium/tests/fixtures/reporting/static.definition.json) | [`static.sources.json`](/elements/003-lithium/tests/fixtures/reporting/static.sources.json) | [`static.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/static.intermediate.json) | |
| Detail | [`detail.definition.json`](/elements/003-lithium/tests/fixtures/reporting/detail.definition.json) | [`detail.sources.json`](/elements/003-lithium/tests/fixtures/reporting/detail.sources.json) | [`detail.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/detail.intermediate.json) | [`detail.intermediate.formatted.json`](/elements/003-lithium/tests/fixtures/reporting/detail.intermediate.formatted.json) |
| Grouped | [`grouped.definition.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.definition.json) | [`grouped.sources.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.sources.json) | [`grouped.intermediate.json`](/elements/003-lithium/tests/fixtures/reporting/grouped.intermediate.json) | |

The static report has no datasources. Its sources file is `{}`. The
detail band has no `datasource` and appears once. Its text item is the
literal `Static`.

The detail report keeps the plan's `customer` field and adds `amount`
(`1234.5`, mask `#,##0.00`) and `note` (JSON `null`, the same mask).
The structural file prints `1234.5`. The formatted file prints
`1,234.50`. Both print `note` as an empty string. The mask is not
applied to null, and the page does not show `null`.

The grouped report has one group level on `region`. Both rows are
`North`, so the page is page header, one group header, two detail
bands, one group footer, page footer. The group header stores
`field`. The group footer does not. It shares that grouping.

Align and item font are omitted on every fixture item. The
intermediate resolves align to `Left` and font to the definition font.
`field`, `mask`, and `datasource` stay on the definition. They are not
copied onto the intermediate item.

A repeated detail band keeps the definition id `detail` on each
instance.

## Definition

The file the designer exports. `format` is `1`. `typeKey` is lookup
053 `key_idx`. Lengths are strings with a unit suffix: `in`, `mm`,
`cm`, `pt`, or `px`. The designer converts at 96 CSS pixels per inch
for the unzoomed view. Emitted HTML uses the physical unit.

The plan's example is the shape. The detail fixture follows it and
adds the masked number, the null, and a page footer at `0.4 in`.

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
  "bands": [],
  "functions": []
}
```

A field text item stores `field` and `box`. Omitted `align` means
`Left`. A number, datetime, aggregate, or parameter item also stores
`mask`. An item stores `font` only when it overrides the definition
font, in the same `{ family, size, style }` shape. Style is `Normal`,
`Bold`, `Italic`, or `Bold + Italic`.

A static text item stores `text`. A group header stores `field`, one
field name. A detail band stores `datasource`, one datasource name.
`functions` is `[]` until a later phase stores author Lua.

`kind` `json` takes an array in `sources`, or the session sample while
designing. That array is not stored in the definition. `kind` `query`
names a QueryRef and binds. Sample rows still design the canvas.

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
| 29 | Calculated number. Names a function whose `returns` is `number`. |
| 32 | Calculated datetime. Names a function whose `returns` is `datetime`. |
| 34 | Aggregate text. Mode, min, max, first, and last of a text value. |
| 35 | Aggregate number. |
| 38 | Aggregate datetime. Min, max, first, and last of a datetime. |

Rulers, Report Top, and Report Bottom are canvas chrome. They are not
bands in the file. Grid and ruler settings are view defaults, stored
so the next session opens the same page.

## Intermediate

Compose output is JSON. Emit reads `text`, `align`, `font`, and `box`,
and clips to the box. Emit does not format again. Boxes stay in
physical units, relative to the band.

Phase 1 pins the document keys the lock described as pages of bands of
items:

```json
{
  "pages": [
    {
      "bands": [
        {
          "id": "detail",
          "typeKey": 15,
          "height": "0.25 in",
          "items": [
            {
              "id": "amount",
              "typeKey": 21,
              "text": "1,234.50",
              "align": "Left",
              "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "Normal" },
              "box": {
                "left": "2.25 in",
                "top": "0 in",
                "width": "1.5 in",
                "height": "0.2 in"
              }
            }
          ]
        }
      ]
    }
  ],
  "log": []
}
```

A page object has `bands` only. It has no page number, page count, or
row list. Those arrive with paging and the page items. Page size and
margins stay on the definition. The same call still has that
definition when it emits.

A band object has `id`, `typeKey`, `height`, and `items`. A group
break starts a group header and ends with a group footer. Page header
and page footer repeat. Report header is on page 1. Report footer is
on the last page. These three fixtures are one page, so the page
header and the page footer each appear once.

`log` is the generation log, an array of `{ "level", "message" }`.
`level` is `info`, `warn`, or `error`. The `stage=compose` result is
this document. These happy-path files include an empty log.

### Structural and formatted

Phase 19 returns the structural intermediate. Field text is the plain
value, so `1234.5` prints `1234.5` even though the item stores a mask.
A null prints as `""`.

Phase 20 returns the formatted intermediate. The mask table in the
plan's Value format section is the oracle. The detail pair locks two
rows of it:

| Value | Mask | Structural text | Formatted text |
| --- | --- | --- | --- |
| `1234.5` | `#,##0.00` | `1234.5` | `1,234.50` |
| `null` | `#,##0.00` | empty string | empty string |

`customer` prints `North` in both files. A number rounds half away
from zero. An omitted number mask prints a plain decimal. Null skips
the mask. The full table, including dates, lives in the plan. Phase 20
adds a fixture that walks the rest of it.

## HTML

`stage=html` emits one section per page of the composed document.
Phase 24 writes the skeleton file for the detail fixture. This note
locks the properties that file has to show.

- Each page is one section, and the section uses `break-after: page`.
- The section contains the composed text.
- Positions use the physical unit already stored on the box. They do
  not use the designer's 96 CSS pixels per inch.
- Align is the item's `align`. Font family, size, and style come from
  the item, then the definition font.
- Text clips to the item box.
- The same intermediate is the oracle for the Lua result and the
  preview.

The preview is that HTML in a sandboxed iframe. DOMPurify runs before
`srcdoc`. The parent page does not assign report HTML to `innerHTML`.
That is Phase 25.

There is no script stage `png`. PDF is the default stage. A call that
omits `stage` still runs `compose` until Phase 31 switches the default
to `pdf`.

## Call

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

The Phase 19 fixture call passes `stage` `compose`, the definition
file as `definition`, and the sources file as `sources`. It does not
send `parameters` for these three reports. None of them declare any.

A stored report may send `report_id` and `rev_id` instead of
`definition`. The script loads that row through the Phase 17 QueryRef.
`stage` is optional and defaults to `pdf`. `params.now`, when it is a
unix number, is the generation instant. A fixture may send it. A
preview omits it, and compose uses `H.system.now()`. These Phase 1
files do not send `params.now`.

The client must not send `params._hydrogen`. The server injects it.

Client params stay under `ClientInvokeMaxParamsBytes` (256 KiB unless
the operator changed it). These fixtures fit. The result cap is 1 MiB.
A PDF that fits returns `{ "pdf_base64": "...", "log": [] }`. A PDF
that does not fit returns
`{ "error": "result_too_large", "cap": N, "size": N, "log": [] }`.

Script stages are `pdf` (default), `html`, `svg`, `csv`, `compose`,
and `check`.

## Defaults

Compose and the designer share this table. The property panel shows
the effective value. When the file omits the key, the control still
shows the default and notes the omission. Import accepts the omission.
Creating a report, a band, or an item writes the stored defaults into
the file. A key whose default is "omit" stays off the object. Import
and export leave an omitted key omitted until the operator edits that
control. Editing writes the key. Minimum band height stays `0.15 in`.
That is a clamp, not a default.

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

The three Phase 1 definitions write the page, grid, and font rows, and
they write the band heights. Their items omit `align` and `font`, which
is the "omit" row, not the create row. The static item's text is
`Static`, chosen so the fixture is obvious. A new item from the
designer still writes `Text`.
