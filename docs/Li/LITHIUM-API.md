# Lithium API Integration

This document outlines the Hydrogen API endpoints currently utilized by the Lithium frontend application.

---

## Authentication (`/api/auth`)

### `POST /api/auth/login`

- **Purpose:** Authenticates a user and returns a JWT.
- **Payload:** `{ "username": "...", "password": "..." }`
- **Response:** `{ "token": "...", "expires_at": "..." }`
- **Usage:** `src/managers/login/login.js`

### `POST /api/auth/renew`

- **Purpose:** Renews an existing, valid JWT before it expires.
- **Payload:** None (uses existing token in Authorization header).
- **Response:** `{ "token": "...", "expires_at": "..." }`
- **Usage:** `src/app.js` (automatic renewal)

### `POST /api/auth/logout`

- **Purpose:** Invalidates the current session on the server.
- **Payload:** None.
- **Response:** `{ "message": "Logged out successfully" }`
- **Usage:** `src/app.js`

---

## System (`/api/system`)

### `GET /api/system/health`

- **Purpose:** Checks the health and availability of the Hydrogen backend.
- **Response:** `{ "status": "ok", "version": "..." }`
- **Usage:** `src/app.js` (startup check)

---

## Lookups (`/api/lookups`)

### `GET /api/lookups`

- **Purpose:** Retrieves system-wide lookup data (e.g., languages, themes, roles).
- **Response:** `{ "languages": [...], "themes": [...], ... }`
- **Usage:** `src/shared/lookups.js`

---

## Styles/Themes (`/api/styles`)

### `GET /api/styles`

- **Purpose:** Retrieves a list of available themes/styles.
- **Response:** Array of style objects.
- **Usage:** `src/managers/style-manager/style-manager.js`

### `GET /api/styles/:id`

- **Purpose:** Retrieves the CSS content for a specific style.
- **Response:** `{ "css": "..." }`
- **Usage:** `src/managers/style-manager/style-manager.js`

### `PUT /api/styles/:id`

- **Purpose:** Updates an existing style.
- **Payload:** `{ "css": "...", "name": "..." }`
- **Usage:** `src/managers/style-manager/style-manager.js`

### `POST /api/styles`

- **Purpose:** Creates a new style.
- **Payload:** `{ "css": "...", "name": "..." }`
- **Usage:** `src/managers/style-manager/style-manager.js`

### `DELETE /api/styles/:id`

- **Purpose:** Deletes a style.
- **Usage:** `src/managers/style-manager/style-manager.js`

---

## Queries (`/api/conduit`)

The Conduit service provides endpoints for executing pre-defined database queries by reference ID.

### `POST /api/conduit/query`

- **Purpose:** Execute a single public query.
- **Payload:**

  ```json
  {
    "query_ref": 1,
    "database": "Acuranzo",
    "params": {
      "INTEGER": { "limit": 10 },
      "STRING": { "status": "active" }
    }
  }
  ```

### `POST /api/conduit/queries`

- **Purpose:** Execute multiple public queries in a single request.
- **Payload:**

  ```json
  {
    "database": "Acuranzo",
    "queries": [
      { "query_ref": 1, "params": {} },
      { "query_ref": 25, "params": { "STRING": { "status": "active" } } }
    ]
  }
  ```

### `POST /api/conduit/auth_query`

- **Purpose:** Execute a single authenticated query (requires JWT).
- **Payload:**

  ```json
  {
    "query_ref": 8,
    "params": {
      "STRING": { "username": "testuser" }
    }
  }
  ```

- **Usage:** `src/managers/queries/queries.js` (via `src/shared/conduit.js`)

### `POST /api/conduit/auth_queries`

- **Purpose:** Execute multiple authenticated queries in a single request (requires JWT).
- **Payload:**

  ```json
  {
    "queries": [
      { "query_ref": 1, "params": {} },
      { "query_ref": 25, "params": { "STRING": { "status": "active" } } }
    ]
  }
  ```

---

## Conduit Client Library

**File:** `src/shared/conduit.js`

All conduit API calls should use the shared conduit module rather than hand-crafting `api.post()` calls. The module ensures correct payload structure (always includes `params`), extracts rows from responses, and is fully unit-tested.

### Pure Helpers (No Network)

| Function | Purpose |
|----------|---------|
| `buildQueryPayload(queryRef, params)` | Build a single-query payload object |
| `buildBatchPayload(queries, database?)` | Build a multi-query batch payload |
| `extractRows(response)` | Normalise response → plain rows array |
| `extractBatchRows(response)` | Normalise batch response → `Map<queryRef, rows>` |
| `extractError(response)` | Extract error info from `success: false` response (returns `null` if no error) |

### API Wrappers

| Function | Endpoint | Returns |
|----------|----------|---------|
| `authQuery(api, queryRef, params?)` | `conduit/auth_query` | `Promise<Array>` — rows |
| `authQueries(api, queries)` | `conduit/auth_queries` | `Promise<Map>` — queryRef → rows |
| `query(api, queryRef, params?, db?)` | `conduit/query` | `Promise<Array>` — rows |
| `queries(api, queryList, db?)` | `conduit/queries` | `Promise<Map>` — queryRef → rows |

### Error Handling

`authQuery()` handles errors from two paths and always enriches them with `error.serverError`:

1. **HTTP error (4xx/5xx):** `json-request.js` throws with `error.data` containing the JSON body. `authQuery()` catches this, calls `extractError(error.data)`, and attaches the result as `error.serverError`.
2. **200 OK with `success: false`:** `authQuery()` calls `extractError(response)` directly and throws a new error with `serverError` attached.

In both cases, callers can rely on `error.serverError` containing `{ message, error, queryRef, database }`.

### Usage Examples

```javascript
import { authQuery, authQueries } from '../../shared/conduit.js';

// Single authenticated query — returns rows array
const rows = await authQuery(app.api, 25);

// With typed parameters
const searchRows = await authQuery(app.api, 32, {
  STRING: { SEARCH: 'USERS' },
});

// Batch queries — returns Map<queryRef, rows>
const results = await authQueries(app.api, [
  { queryRef: 25 },
  { queryRef: 32, params: { STRING: { SEARCH: 'X' } } },
]);
const list = results.get(25);   // rows for QueryRef 25
const search = results.get(32); // rows for QueryRef 32
```

### Parameter Types

Conduit parameters are typed. Wrap values in their type key:

| Type | Example |
|------|---------|
| `STRING` | `{ STRING: { status: "active", name: "test" } }` |
| `INTEGER` | `{ INTEGER: { limit: 10, offset: 0 } }` |
| `BOOLEAN` | `{ BOOLEAN: { active: true } }` |

Multiple types can be combined in a single params object:

```javascript
const rows = await authQuery(app.api, 50, {
  STRING: { status: 'active' },
  INTEGER: { limit: 25 },
});
```
