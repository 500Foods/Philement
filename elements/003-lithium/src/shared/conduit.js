/**
 * Conduit - Shared helper functions for Hydrogen Conduit API queries
 *
 * Provides reusable, testable functions for building conduit payloads,
 * executing queries, and extracting results. All managers that interact
 * with conduit endpoints should use these helpers instead of hand-crafting
 * API calls.
 *
 * Conduit endpoints (see LITHIUM-API.md):
 *   POST /api/conduit/query        — single public query
 *   POST /api/conduit/queries       — batch public queries
 *   POST /api/conduit/auth_query    — single authenticated query (JWT)
 *   POST /api/conduit/auth_queries  — batch authenticated queries (JWT)
 *
 * Payload format:
 *   { query_ref: 25, params: { STRING: { status: "active" }, INTEGER: { limit: 10 } } }
 *
 * Batch payload:
 *   { queries: [ { query_ref: 1, params: {} }, { query_ref: 25, params: {} } ] }
 *   Optionally includes database: "DatabaseName" for public endpoints.
 */

// ─── Pure Functions (testable, no side-effects) ──────────────────────────

/**
 * Build a single-query payload for conduit endpoints.
 * Always includes a `params` key (defaults to empty object).
 *
 * @param {number} queryRef - The QueryRef number to execute
 * @param {Object} [params={}] - Typed parameters, e.g. { STRING: { key: "val" }, INTEGER: { n: 10 } }
 * @returns {Object} Payload ready for POST body
 *
 * @example
 *   buildQueryPayload(25);
 *   // → { query_ref: 25, params: {} }
 *
 *   buildQueryPayload(32, { STRING: { SEARCH: "USERS" } });
 *   // → { query_ref: 32, params: { STRING: { SEARCH: "USERS" } } }
 */
export function buildQueryPayload(queryRef, params = {}) {
  if (queryRef == null || typeof queryRef !== 'number') {
    throw new Error(`buildQueryPayload: queryRef must be a number, got ${typeof queryRef}`);
  }

  return {
    query_ref: queryRef,
    params,
  };
}

/**
 * Build a batch payload for conduit multi-query endpoints.
 *
 * @param {Array<{queryRef: number, params?: Object}>} queries - Array of query descriptors
 * @param {string} [database] - Optional database name (required for public endpoints)
 * @returns {Object} Payload ready for POST body
 *
 * @example
 *   buildBatchPayload([
 *     { queryRef: 1, params: {} },
 *     { queryRef: 25, params: { STRING: { status: "active" } } },
 *   ]);
 *   // → { queries: [ { query_ref: 1, params: {} }, { query_ref: 25, params: { STRING: { status: "active" } } } ] }
 *
 *   buildBatchPayload([ { queryRef: 1 } ], 'Acuranzo');
 *   // → { database: "Acuranzo", queries: [ { query_ref: 1, params: {} } ] }
 */
export function buildBatchPayload(queries, database) {
  if (!Array.isArray(queries) || queries.length === 0) {
    throw new Error('buildBatchPayload: queries must be a non-empty array');
  }

  const payload = {
    queries: queries.map(q => buildQueryPayload(q.queryRef, q.params)),
  };

  if (database) {
    payload.database = database;
  }

  return payload;
}

/**
 * Extract the rows array from a single-query conduit response.
 *
 * Conduit returns different shapes depending on success/failure:
 *   Success: { data: [...] }  or  { rows: [...] }  or  { results: [{ rows: [...] }] }
 *
 * This function normalises all known shapes into a plain rows array.
 *
 * @param {Object} response - The parsed JSON response from the conduit endpoint
 * @returns {Array} The rows array, or an empty array if none found
 */
export function extractRows(response) {
  if (!response) return [];

  // Direct data array (most common for single query)
  if (Array.isArray(response.data)) return response.data;

  // Direct rows array
  if (Array.isArray(response.rows)) return response.rows;

  // Wrapped in results array (batch-style single response)
  if (Array.isArray(response.results) && response.results.length > 0) {
    const first = response.results[0];
    if (first.success && Array.isArray(first.rows)) return first.rows;
  }

  // Response is itself an array
  if (Array.isArray(response)) return response;

  return [];
}

/**
 * Extract rows from a batch (multi-query) conduit response.
 * Returns a Map keyed by query_ref → rows array.
 *
 * @param {Object} response - The parsed JSON response from the batch endpoint
 * @returns {Map<number, Array>} Map of queryRef → rows
 */
export function extractBatchRows(response) {
  const map = new Map();

  if (!response || !Array.isArray(response.results)) return map;

  for (const result of response.results) {
    const ref = result.query_ref;
    if (result.success) {
      map.set(ref, result.rows || []);
    } else {
      // Store error information so callers can handle failures per-query
      map.set(ref, { error: result.error || 'Unknown error', rows: [] });
    }
  }

  return map;
}

/**
 * Extract error information from a conduit response.
 * Returns null if no error, or an object with error details.
 *
 * @param {Object} response - The parsed JSON response from the conduit endpoint
 * @returns {Object|null} Error object with message, error, query_ref, database or null
 */
export function extractError(response) {
  if (!response) return null;

  // Check for explicit success flag
  if (response.success === false) {
    return {
      message: response.message || response.error || 'Unknown error',
      error: response.error || 'Unknown error',
      queryRef: response.query_ref,
      database: response.database,
    };
  }

  return null;
}


// ─── API Functions (thin wrappers using the api client) ──────────────────

/**
 * Execute a single authenticated conduit query.
 * Requires a valid JWT (uses auth_query endpoint).
 *
 * @param {Object} api - The API client (from createRequest / app.api)
 * @param {number} queryRef - The QueryRef number
 * @param {Object} [params={}] - Typed parameters
 * @returns {Promise<Array>} The rows from the query result
 * @throws {Error} If the query fails, contains the server error message
 *
 * @example
 *   const rows = await authQuery(app.api, 25);
 *   const rows = await authQuery(app.api, 32, { STRING: { SEARCH: "USERS" } });
 */
export async function authQuery(api, queryRef, params = {}) {
  const payload = buildQueryPayload(queryRef, params);
  const response = await api.post('conduit/auth_query', payload);

  // Check for error response before extracting rows
  const error = extractError(response);
  if (error) {
    const err = new Error(error.message);
    err.serverError = error;
    throw err;
  }

  return extractRows(response);
}

/**
 * Execute multiple authenticated conduit queries in one request.
 *
 * @param {Object} api - The API client
 * @param {Array<{queryRef: number, params?: Object}>} queries - Query descriptors
 * @returns {Promise<Map<number, Array>>} Map of queryRef → rows
 *
 * @example
 *   const results = await authQueries(app.api, [
 *     { queryRef: 25 },
 *     { queryRef: 32, params: { STRING: { SEARCH: "USERS" } } },
 *   ]);
 *   const queryList = results.get(25);  // rows array
 */
export async function authQueries(api, queries) {
  const payload = buildBatchPayload(queries);
  const response = await api.post('conduit/auth_queries', payload);
  return extractBatchRows(response);
}

/**
 * Execute a single public conduit query (no JWT required).
 *
 * @param {Object} api - The API client
 * @param {number} queryRef - The QueryRef number
 * @param {Object} [params={}] - Typed parameters
 * @param {string} [database] - Database name
 * @returns {Promise<Array>} The rows from the query result
 */
export async function query(api, queryRef, params = {}, database) {
  const payload = buildQueryPayload(queryRef, params);
  if (database) payload.database = database;
  const response = await api.post('conduit/query', payload);
  return extractRows(response);
}

/**
 * Execute multiple public conduit queries in one request.
 *
 * @param {Object} api - The API client
 * @param {Array<{queryRef: number, params?: Object}>} queryList - Query descriptors
 * @param {string} [database] - Database name
 * @returns {Promise<Map<number, Array>>} Map of queryRef → rows
 */
export async function queries(api, queryList, database) {
  const payload = buildBatchPayload(queryList, database);
  const response = await api.post('conduit/queries', payload);
  return extractBatchRows(response);
}


// ─── Default Export ──────────────────────────────────────────────────────

export default {
  // Pure helpers
  buildQueryPayload,
  buildBatchPayload,
  extractRows,
  extractBatchRows,
  extractError,
  // API wrappers
  authQuery,
  authQueries,
  query,
  queries,
};
