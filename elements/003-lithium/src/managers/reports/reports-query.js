/**
 * Phase 28: Query datasources. A `query` datasource names a Reporting
 * QueryRef (lookup 028 key 3) and its parameter values. The Report
 * Manager loads the Reporting list (QueryRef 173) with the panel and
 * on Refresh. Run calls each query through conduit authQuery and
 * stores the rows on that datasource's `rows`, the same place Load
 * Sample writes, so Compose sends them in `sources`.
 */

import { authQuery } from '../../shared/conduit.js';

/** QueryRef that lists the active Reporting queries. */
export const REPORTING_LIST_REF = 173;

const clone = (value) => JSON.parse(JSON.stringify(value));

function parseJson(value) {
  if (value && typeof value === 'object') return value;
  if (typeof value !== 'string' || value.trim() === '') return {};
  try {
    const parsed = JSON.parse(value);
    return parsed && typeof parsed === 'object' ? parsed : {};
  } catch {
    return {};
  }
}

/**
 * Normalize one QueryRef 173 row.
 * @returns {{ queryRef: number, name: string, summary: string, group: string, fields: object[], parameters: object[] }}
 */
export function reportingQuery(row) {
  const meta = parseJson(row && row.collection).reporting || {};
  const list = (value) => (Array.isArray(value) ? value.filter((e) => e && typeof e.name === 'string') : []);
  return {
    queryRef: Number(row && row.query_ref),
    name: String((row && row.name) || ''),
    summary: String((row && row.summary) || ''),
    group: String(meta.group || ''),
    fields: list(meta.fields).map((f) => ({ name: f.name, type: String(f.type || 'string'), kind: String(f.kind || 'field') })),
    parameters: list(meta.parameters).map((p) => ({
      name: p.name, type: String(p.type || 'string'), kind: String(p.kind || 'field'),
      default: Object.hasOwn(p, 'default') ? p.default : null,
    })),
  };
}

/** Picker label for a Reporting query. */
export const queryLabel = (q) => `#${q.queryRef} ${q.name}`;

/** Load and normalize the Reporting list, sorted by name. */
export async function loadReportingQueries(api) {
  const rows = await authQuery(api, REPORTING_LIST_REF);
  return (Array.isArray(rows) ? rows : [])
    .map(reportingQuery)
    .filter((q) => Number.isInteger(q.queryRef))
    .sort((a, b) => a.name.localeCompare(b.name) || a.queryRef - b.queryRef);
}

/** Find a Reporting query by label, "#ref", or number. */
export function findReportingQuery(queries, value) {
  const list = Array.isArray(queries) ? queries : [];
  const text = String(value ?? '').trim();
  const ref = Number(text.replace(/^#/, '').split(/\s/)[0]);
  return list.find((q) => queryLabel(q) === text) || list.find((q) => q.queryRef === ref) || null;
}

/** Default parameter values from metadata. */
export function defaultParams(query) {
  const out = {};
  for (const p of (query && query.parameters) || []) {
    if (p.kind === 'calculated') continue;
    out[p.name] = p.default;
  }
  return out;
}

const TYPE_GROUPS = Object.freeze({
  integer: 'INTEGER', int: 'INTEGER', float: 'FLOAT', number: 'FLOAT', boolean: 'BOOLEAN', bool: 'BOOLEAN',
  string: 'STRING', text: 'STRING', memo: 'STRING', date: 'STRING', datetime: 'STRING',
});

/**
 * Typed conduit params from a datasource's `params` and the query's
 * parameter metadata. Unknown names are sent as STRING.
 */
export function conduitParams(params, query) {
  const types = new Map(((query && query.parameters) || []).map((p) => [p.name, p.type]));
  const out = {};
  for (const [name, raw] of Object.entries(params || {})) {
    const group = TYPE_GROUPS[String(types.get(name) || 'string').toLowerCase()] || 'STRING';
    let value;
    if (group === 'INTEGER') value = Number.parseInt(raw, 10);
    else if (group === 'FLOAT') value = Number(raw);
    else if (group === 'BOOLEAN') value = raw === true || raw === 'true' || raw === 1 || raw === '1';
    else value = raw == null ? '' : String(raw);
    if ((group === 'INTEGER' || group === 'FLOAT') && !Number.isFinite(value)) {
      throw new Error(`Parameter ${name} must be a number.`);
    }
    (out[group] = out[group] || {})[name] = value;
  }
  return out;
}

/**
 * Point datasource `index` at a Reporting query. Params reset to the
 * query's defaults; `fields` keeps the metadata so Field lists columns
 * before a run.
 */
export function setDatasourceQuery(doc, index, value, queries) {
  const query = findReportingQuery(queries, value);
  if (!query) throw new Error(`Reporting query "${value}" is not in the list. Refresh the query list.`);
  const next = clone(doc);
  const ds = next.datasources[index];
  if (!ds || ds.kind !== 'query') throw new Error(`Datasource ${index + 1} is not a Query datasource.`);
  Object.assign(ds, {
    queryRef: query.queryRef, queryName: query.name, params: defaultParams(query),
    fields: query.fields.map((f) => ({ ...f })),
  });
  return next;
}

/** Replace datasource `index` params from JSON object text. */
export function setDatasourceParams(doc, index, text) {
  let value;
  try {
    value = typeof text === 'string' ? JSON.parse(text || '{}') : text;
  } catch {
    throw new Error('Params must be a JSON object, e.g. {"ROW_LIMIT": 5}.');
  }
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('Params must be a JSON object.');
  const next = clone(doc);
  const ds = next.datasources[index];
  if (!ds || ds.kind !== 'query') throw new Error(`Datasource ${index + 1} is not a Query datasource.`);
  ds.params = value;
  return next;
}

/** Set one parameter value on query datasource `index`. */
export function setDatasourceParam(doc, index, name, value) {
  const next = clone(doc);
  const ds = next.datasources && next.datasources[index];
  if (!ds || ds.kind !== 'query') throw new Error(`Datasource ${index + 1} is not a Query datasource.`);
  ds.params = { ...(ds.params || {}), [name]: value };
  return next;
}

/**
 * Run every query datasource that names a QueryRef and store the rows.
 * A failed query keeps its previous rows and is reported in `errors`.
 * @returns {Promise<{ doc: object, ran: { name: string, rows: number }[], errors: { name: string, message: string }[] }>}
 */
export async function runQueryDatasources(api, doc, queries, run = authQuery) {
  const next = clone(doc);
  const ran = [];
  const errors = [];
  for (const ds of Array.isArray(next.datasources) ? next.datasources : []) {
    if (!ds || ds.kind !== 'query' || !Number.isInteger(Number(ds.queryRef)) || ds.queryRef == null) continue;
    const query = findReportingQuery(queries, ds.queryRef);
    try {
      const rows = await run(api, Number(ds.queryRef), conduitParams(ds.params, query));
      ds.rows = Array.isArray(rows) ? rows : [];
      ran.push({ name: ds.name, rows: ds.rows.length });
    } catch (error) {
      errors.push({ name: ds.name, message: error instanceof Error ? error.message : String(error) });
    }
  }
  return { doc: next, ran, errors };
}

/**
 * Query datasources that name a QueryRef but hold no rows yet. Compose
 * runs these first so an un-Run Query datasource is not sent empty.
 */
export function pendingQueryDatasources(doc) {
  return (Array.isArray(doc && doc.datasources) ? doc.datasources : []).filter((ds) => ds
    && ds.kind === 'query' && ds.queryRef != null && Number.isInteger(Number(ds.queryRef))
    && !(Array.isArray(ds.rows) && ds.rows.length));
}
