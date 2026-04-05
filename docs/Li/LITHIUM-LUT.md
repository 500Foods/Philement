# Lithium Lookup Tables (LUT)

This document describes how lookup tables work in the Philement platform, how they are stored in the database, how they are loaded and cached in Lithium, and how they integrate with Tabulator column types.

---

## Overview

Lookup tables (LUTs) are reference data used throughout the application. They map integer keys to human-readable labels and associated metadata. Lookups are used for:

- **Table columns** — A column stores an integer ID (e.g., `query_status_a27 = 1`) which is resolved at display time to a label like "Active".
- **Dropdown editors** — When editing a lookup column, the editor presents a list of valid values from the lookup table.
- **UI labels** — Status badges, type indicators, and other UI elements resolve integers to text via lookups.
- **Configuration** — Themes, icons, modules, and system settings are all stored as lookups.

Lookups are intentionally simple in structure but extremely flexible. A single `lookups` table in the database holds all reference data for the entire application.

---

## Database Schema

The `lookups` table is defined in the Acuranzo schema (migration `1001`):

```sql
CREATE TABLE lookups (
    lookup_id     INTEGER   NOT NULL,   -- Which lookup this belongs to
    key_idx       INTEGER   NOT NULL,   -- Key within the lookup
    status_a1     INTEGER   NOT NULL,   -- Status (1=Active, 0=Inactive)
    value_txt     TEXT,                 -- Human-readable label / text value
    value_int     INTEGER,              -- Numeric value (context-dependent)
    sort_seq      INTEGER   NOT NULL,   -- Display sort order
    code          TEXT,                 -- Code/CSS/SQL (context-dependent)
    summary       TEXT,                 -- Markdown documentation
    collection    JSON,                 -- Extended metadata (JSON object)
    -- ... common audit fields (created_id, created_at, updated_id, updated_at, etc.)
    PRIMARY KEY (lookup_id, key_idx)
);
```

### Key Design Principles

1. **Composite primary key** — `(lookup_id, key_idx)`. Every lookup entry is uniquely identified by its lookup number and its key within that lookup.
2. **Self-documenting** — Lookup `000` (key_idx = 0) is the master index. Each lookup's definition row (at `lookup_id = 0, key_idx = N`) contains the name and description of lookup N.
3. **Flexible value storage** — `value_txt` for labels, `value_int` for numeric values, `code` for SQL/CSS/scripts, `collection` for complex JSON metadata. Each lookup uses whichever columns suit its purpose.
4. **Active/Inactive** — `status_a1` controls whether an entry is active. Queries typically filter `WHERE status_a1 = 1`.
5. **Sort order** — `sort_seq` controls display order, independent of `key_idx`.

### The Naming Convention

When a table column references a lookup, we use the convention `fieldname_aNN` where `NN` is the lookup ID. For example:

- `query_status_a27` → references lookup 027 (Query Status)
- `query_type_a28` → references lookup 028 (Query Type)
- `status_a1` → references lookup 001 (Lookup Status — the universal active/inactive flag)

This naming convention makes it immediately clear which lookup a column references, without requiring external documentation.

---

## Lookup Index (Lookup 000)

Lookup 000 is the master index. It contains one row per lookup, where:

- `lookup_id = 0`
- `key_idx = N` (the lookup ID being described)
- `value_txt` = the human-readable name of lookup N
- `summary` = Markdown documentation for the lookup
- `collection` = JSON metadata (e.g., which editors to show)

This is populated by migration `1025` (the seed) and extended by each subsequent lookup migration. The master index is queried by QueryRef 030.

---

## Acuranzo Lookup Registry

The following lookups are defined in the Acuranzo schema:

| ID  | Name | Migration | Description |
|-----|------|-----------|-------------|
| 000 | Lookup Items | 1025 | Master index — one row per lookup |
| 001 | Lookup Status | 1026 | Active/Inactive status (used by `status_a1`) |
| 002 | Language Status | 1027 | Language activation status |
| 003 | Dictionary Status | 1028 | Dictionary item status |
| 004 | Connection Type | 1029 | Database connection types |
| 005 | Connected Status | 1030 | Connection status values |
| 006 | Session Activity | 1031 | Session log activity types |
| 007 | Session Type | 1032 | Session types (login, API, etc.) |
| 008 | Server Component Type | 1033 | Server component categories |
| 020 | Migration Type | 1035 | Migration query types |
| 025 | Session Icon | 1038 | Icons for session log entries |
| 026 | Session Log Status | 1039 | Session log entry statuses |
| 027 | Query Status | 1040 | Query status (Active, Inactive, Draft) |
| 028 | Query Type | 1041 | Query types (SQL, Migration, Diagram, etc.) |
| 029 | Migration Workflow Status | 1044 | Workflow status for migrations |
| 030 | SQL Dialect | 1045 | Database dialects (SQLite, MySQL, PostgreSQL, DB2) |
| 031 | Server Boot Status | 1046 | Server startup stages |
| 032 | Password Status | 1047 | Password validation status |
| 033 | API Key Status | 1048 | API key statuses |
| 034 | User Status | 1050 | User account statuses |
| 035 | User Role | 1051 | User roles (admin, user, guest) |
| 036 | Token Status | 1052 | JWT token statuses |
| 037 | Icons | 1053 | Icon set configuration and definitions |
| 038 | System Preferences | 1054 | System preference keys |
| 039 | Design Status | 1056 | Schema design statuses |
| 040 | Schema Version Status | 1057 | Schema versioning status |
| 041 | Themes | 1066–1074 | CSS themes (Dark, Light, Print, etc.) |
| 042 | Modules | 1075 | Application modules / managers |
| 043 | Tours | 1076 | Guided tours configuration |
| 044 | Server Version History | 1077 | Server release history |
| 045 | Client Version History | 1078 | Client release history |
| 046 | Macro Expansion | 1079 | Template macro definitions |
| 047 | Translations | 1080 | i18n / localisation strings |
| 048 | Module Groups | 1081 | Module grouping for menus |
| 049 | Document Libraries | 1082 | Document library definitions |
| 050 | Document Status | 1083 | Document lifecycle statuses |
| 051 | Document Types | 1084 | Document type categories |
| 052 | AI Summary Engines | 1085 | AI engines for summarisation |
| 053 | Report Object Types | 1086 | Report builder object types |
| 054 | Report Object Attributes | 1087 | Report builder object attributes |
| 055 | Canvas LMS Links | 1088 | Learning management system integration |
| 056 | Acuranzo Systems | 1089 | Known client implementations |
| 057 | Report Page Sizes | 1090 | Report page size options |
| 058 | Query Queues | 1091 | Query execution queue types (Fast, Slow) |

---

## How Lookups Are Loaded in Lithium

### Startup Sequence

When the application starts ([`app.js`](../../elements/003-lithium/src/app.js)):

1. **From localStorage** — If cached lookup data exists and is less than 24 hours old, it is loaded immediately into memory. An event (`lookups:category:loaded`) is emitted per category with `source: 'cache'`.
2. **From server** — A batch request fetches fresh lookup data using QueryRefs 001, 030, 053, 054. When the response arrives, the in-memory cache and localStorage are updated, and events are emitted with `source: 'server'`.

This dual strategy means the UI has lookup data available almost instantly (from localStorage) while fresh data arrives in the background.

### ⚠️ Critical: QueryRef vs Lookup Table

> **This is the #1 source of confusion when working with lookups.**

A **QueryRef** is a REST API endpoint that returns data. A **Lookup Table** (`lookup_id = N`) is data stored in the database `lookups` table.

**They are NOT the same thing:**

| QueryRef | Example Returns | How to Call |
|---------|-----------------|-------------|
| 46 | Menu items (manager list) | `authQuery(api, 46)` |
| 26 + LOOKUPID: 46 | Rows from lookup table 46 | `authQuery(api, 26, { INTEGER: { LOOKUPID: 46 } })` |

**Common mistakes:**
- ❌ `authQuery(api, 46)` — Returns menu data, NOT lookup table 46
- ✅ `authQuery(api, 26, { INTEGER: { LOOKUPID: 46 } })` — Returns lookup table 46

**How to know which you're dealing with:**
1. Check the SQL query the QueryRef runs — some QueryRefs return lookup table data directly
2. If a QueryRef has a `LOOKUPID` parameter, it returns lookup table data
3. The lookup table ID (e.g., 44, 45, 46) is NOT the same as the QueryRef number

See also: **[Debugging Lookup Issues](#debugging-lookup-issues)**

### Module: `shared/lookups.js`

The [`lookups.js`](../../elements/003-lithium/src/shared/lookups.js) module manages the lookup lifecycle:

| Function | Purpose |
|----------|---------|
| `fetchLookups()` | Load lookups from cache or server |
| `getLookup(category, key)` | Get a specific lookup value |
| `getLookupCategory(category)` | Get all entries in a category |
| `hasLookup(category)` | Check if a category is loaded |
| `isLoaded()` | Check if any lookups are loaded |
| `refreshLookups()` | Force a server refresh |
| `clearCache()` | Clear both memory and localStorage |

### Currently Loaded Categories

The open (pre-login) lookups loaded at startup:

| Category | QueryRef | Contents |
|----------|----------|----------|
| `system_info` | 001 | System information (license, version) |
| `lookup_names` | 030 | Names of all lookups (the master index) |
| `themes` | 053 | Available themes (CSS) |
| `icons` | 054 | Icon definitions |

### Event Bus Integration

Lookups emit events via the event bus:

| Event | When |
|-------|------|
| `LOOKUPS_LOADING` | Before server fetch begins |
| `LOOKUPS_LOADED` | After lookups arrive from server (not from cache) |
| `lookups:{category}:loaded` | Per-category load event (from any source) |
| `LOOKUPS_ERROR` | When fetch fails and no cache is available |

When a new lookup is loaded from the server, the `LOOKUPS_LOADED` event should trigger any necessary updates — for example, re-rendering table columns that display lookup labels.

---

## Lookups in Tabulator Tables

### The `lookup` Column Type

When a column has `coltype: "lookup"` in its table definition ([`coltypes.json`](../../elements/003-lithium/config/tabulator/coltypes.json)), the column:

1. **Stores** an integer ID in the data (e.g., `query_status_a27 = 1`)
2. **Displays** the human-readable label from the lookup table (e.g., "Active")
3. **Edits** via a dropdown list pre-filled with lookup entries

### Table Definition Example

From [`query-manager.json`](../../elements/003-lithium/config/tabulator/queries/query-manager.json):

```json
{
  "query_status_a27": {
    "display": "Status",
    "field": "query_status_a27",
    "coltype": "lookup",
    "lookupRef": "a27",
    "description": "References lookup table A27 (Query Status)"
  }
}
```

### The `lookupRef` Property

The `lookupRef` string (e.g., `"a27"`) identifies which lookup table the column references. The prefix `a` denotes an Acuranzo-schema lookup. The number maps directly to the `lookup_id` in the database (e.g., `a27` → `lookup_id = 27`).

### Resolution at Runtime

The column resolution engine in [`lithium-table.js`](../../elements/003-lithium/src/core/lithium-table.js) provides:

| Function | Purpose |
|----------|---------|
| `loadLookup(lookupRef, api)` | Fetch and cache a specific lookup table |
| `getLookup(lookupRef)` | Get cached lookup data |
| `resolveLookupLabel(id, lookupRef)` | Resolve an integer ID to its label |
| `createLookupFormatter(lookupRef)` | Create a Tabulator formatter function |
| `createLookupEditor(lookupRef, data)` | Create a Tabulator list editor config |
| `preloadLookups(lookupRefs, api)` | Pre-fetch multiple lookups in parallel |

### Pre-Loading Strategy

Before a table is displayed, the manager should pre-load all lookups referenced by its columns:

```javascript
// Collect all lookupRefs from the table definition
const lookupRefs = Object.values(tableDef.columns)
  .filter(col => col.lookupRef)
  .map(col => col.lookupRef);

// Pre-load in parallel
await preloadLookups(lookupRefs, api);
```

This ensures that when the table renders, the lookup formatter has cached data available and can resolve integer IDs to labels synchronously.

---

## API Access Patterns

### Retrieving a Lookup's Entries

**QueryRef 030** returns the master index (all lookup names). For individual lookup entries, use **QueryRef 034** (Get Lookup List):

```javascript
// Get all entries for lookup 27 (Query Status)
const entries = await authQuery(api, 34, {
  INTEGER: { LOOKUPID: 27 },
});
// Returns: [{ lookup_id: 27, key_idx: 1, value_txt: "Active", ... }, ...]
//          key_idx = the lookup entry ID (used for column values)
//          value_txt = the display label
```

The lookup entries are transformed by `loadLookup()` into a simplified format:

```javascript
// Transformed to: [{ id: 1, label: "Active" }, { id: 2, label: "Inactive" }, ...]
```

### Retrieving a Single Entry

```javascript
// Get a specific key within a lookup
const entry = await authQuery(api, queryRef, {
  INTEGER: { LOOKUPID: 27, KEYIDX: 1 },
});
// Returns: [{ lookup_id: 27, key_idx: 1, value_txt: "Active", ... }]
```

### Creating a New Lookup Entry

QueryRef from migration `1133` auto-increments the `key_idx` within a lookup:

```javascript
await authQuery(api, createQueryRef, {
  INTEGER: { LOOKUPID: 27 },
  STRING: { VALUETXT: "New Status" },
  // ... other fields
});
```

### Updating a Lookup Entry

QueryRef from migration `1134` updates an existing entry:

```javascript
await authQuery(api, updateQueryRef, {
  INTEGER: {
    LOOKUPID: 27,
    KEYIDX: 3,
    ORIGLOOKUPID: 27,
    ORIGKEYIDX: 3,
  },
  STRING: { VALUETXT: "Updated Label" },
  // ... other fields
});
```

---

## Client-Side Accessor Functions

For code that needs to access lookup values:

```javascript
// From shared/lookups.js — for startup lookups (system_info, themes, etc.)
import { getLookup, getLookupCategory } from '../../shared/lookups.js';
const themeName = getLookup('themes', themeId);

// From core/lithium-table.js — for table column lookups
import { resolveLookupLabel, getLookup } from '../../core/lithium-table.js';
const statusLabel = resolveLookupLabel(statusId, 'a27');
```

Note: There are two lookup systems currently in Lithium:

1. **`shared/lookups.js`** — For pre-login / startup lookups (system_info, themes, icons, lookup_names). Uses batch QueryRefs.
2. **`core/lithium-table.js`** — For table column lookups (a27, a28, etc.). Uses individual QueryRefs per lookup.

As we fully implement the lookup column type in Tabulator tables, these two systems should be unified or at least share a common cache layer.

---

## Future Considerations

- **Unified cache** — Merge the `shared/lookups.js` in-memory cache with the `core/lithium-table.js` `_lookupCache` so all lookups are accessed through one consistent API.
- **Event-driven refresh** — When a lookup is updated on the server (e.g., admin adds a new status value), push a `LOOKUPS_UPDATED` event via the event bus to trigger cache invalidation and table re-render.
- **Lookup Editor Manager** — The Lookups manager (ID: 4) in the sidebar will provide a CRUD interface for all lookup tables.
- **Detailed docs** — For complex lookups (like Themes, 041), create `LITHIUM-LUT-041.md` with full documentation of the JSON collection schema and CSS integration.

---

## Debugging Lookup Issues

When lookups aren't working as expected, use this systematic approach:

### 1. Verify the Lookup Table Has Data

Run SQL directly on the database:
```sql
SELECT * FROM lithium.lookups WHERE lookup_id = 46;  -- Your lookup ID here
```

### 2. Identify the Correct QueryRef

- **If the QueryRef takes a `LOOKUPID` parameter**: It returns lookup table data
  ```javascript
  authQuery(api, 26, { INTEGER: { LOOKUPID: 46 } })  // Returns lookup_id=46 rows
  ```
- **If the QueryRef has NO parameter**: Check what it returns — it might be something else entirely (e.g., QueryRef 46 = menu data)

### 3. Check Pre-Login vs Post-Login

Pre-login lookups:
- Loaded in batch at startup: QueryRefs 001, 030, 053, 054, 060
- Available via `shared/lookups.js`

Post-login lookups (require authentication):
- Must be loaded per-manager using `authQuery(api, ref, params)`
- Not cached in `shared/lookups.js` by default

### 4. Common Issues and Solutions

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| `value_txt` is empty | Using wrong QueryRef (e.g., QueryRef 46 for menu instead of QueryRef 26) | Use QueryRef 26 + LOOKUPID parameter |
| Lookup returns 0 rows | Wrong LOOKUPID or lookup is empty in DB | Verify with SQL first |
| Data cached but old | localStorage has stale data | `window.lithiumApp.lookups.clearCache()` then reload |
| 422 HTTP error | QueryRef requires authentication | Use `authQuery` (post-login), not batch fetch |

### 5. Debug Logging Pattern

```javascript
// In your manager:
const rows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 46 } });
log(Subsystems.MANAGER, Status.INFO, `[MyManager] Loaded ${rows.length} rows`);
log(Subsystems.MANAGER, Status.DEBUG, `[MyManager] First row:`, rows[0]);
log(Subsystems.MANAGER, Status.DEBUG, `[MyManager] value_txt samples:`, rows.slice(0, 3).map(r => r.value_txt));
```

---

## Related Documentation

| Document | Relevance |
|----------|-----------|
| [LITHIUM-TAB.md](LITHIUM-TAB.md) | Column configuration system including `lookup` coltype |
| [LITHIUM-TAB-PLAN.md](LITHIUM-TAB-PLAN.md) | Phase 3: Lookup column support implementation plan |
| [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) | Query Manager uses lookup columns (status, type, dialect, queue) |
| [LITHIUM-API.md](LITHIUM-API.md) | Hydrogen API endpoints for fetching lookups |
| [LITHIUM-OTH.md](LITHIUM-OTH.md) | Utilities including event bus |

---

Last updated: March 2026
