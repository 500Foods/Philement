# Scripting Manager

Menu manager for the Lua scripts stored in Helium and run by Hydrogen.

---

## Overview

| Property | Value |
|----------|-------|
| **ID** | 33 |
| **Location** | `src/managers/scripting/` |
| **Type** | Menu manager |
| **Cleanup** | `cleanup()` |

The left pane is a LithiumTable of `scripts`. The right pane has Lua, Preview, and Run tabs. Preview renders the row's **summary** as sanitized markdown, then the Lua source as plain text.

---

## Files

| File | Purpose |
|------|---------|
| `scripting.js` | Manager, toolbar, row selection, font prefs |
| `scripting.html` / `scripting.css` | Template and layout |
| `scripting-editors.js` | CodeMirror Lua editor, StyLua, summary preview |
| `scripting-format.js` | StyLua load and format |
| `scripting-preview.js` | Summary markdown to sanitized HTML |
| `scripting-font.js` | Font prefs in `lithiumSettings` section `33` |
| `scripting-save.js` | Insert, update, duplicate, delete payloads |
| `scripting-invoke.js` | Run tab |
| `scripting-footer.js` | Save, duplicate, delete actions |

---

## QueryRefs

| Ref | Use |
|-----|-----|
| 87 | Detail, including `code` and `summary` |
| 89 | List |
| 90 | Search |
| 129 | Insert |
| 130 | Update |
| 131 | Delete by `GROUP_NAME` and `SCRIPT_NAME` |

Save and update send `SCRIPT_TYPE`, `STATUS`, `USERID`, and `INVOKABLE` as integers. Text fields stay in `STRING`. A blank schedule or run time is `''`. Acuranzo 1395 and 1396 store that blank as SQL NULL. A persisted group and name cannot change.

---

## Run

Run posts `POST /api/conduit/script` for an invokable `Group.Name`. The button stays disabled unless `invokable` is `1` and the name is non-empty. Wait defaults to on. Timeout is 1–60 seconds, default 15. A top-level `params._hydrogen` key is removed before the request. With Wait off, the client polls `GET /api/conduit/script/{job_id}` at 250, 500, 1000, then 2000 ms.

---

## Format and fonts

Ctrl+P and the tulip button format the Lua buffer with StyLua 2.6.0 defaults: tabs, indent width 4, column 120, Unix line endings. The button is disabled off the Lua tab. A parse error is toasted and the buffer stays as it was.

Font size, family, and weight are stored with `window.lithiumSettings` at the dotted path `33.font.<tab>`. That object takes one path on `get`, `set`, and `removeSection`. An older `lithium_scripting_font_*` value is copied once and then removed. The font button is enabled on Lua and Preview, and disabled on Run. Save writes `lithium_preferences` at once and flushes the account-settings update. A reload that still has the previous server copy uses that local value.

---

## Related

- [CATCHUP.md](/elements/003-lithium/CATCHUP.md) — active plan, Phase 10
- [conduit/script.md](/docs/H/api/conduit/script.md) — Hydrogen script invoke
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — the same preview and toolbar pattern
- [LITHIUM-MGR.md](LITHIUM-MGR.md) — manager index
