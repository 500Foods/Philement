/**
 * Phase 15: field items and sample data. Phase 19b stores sample rows
 * on a datasource in the definition (see the Phase 19b block below).
 * Column names are the union of row keys.
 */

/** Field item types: lookup 053 keys 20 text, 21 number, 24 datetime. */
export const FIELD_TYPES = Object.freeze([20, 21, 24]);

/**
 * Items whose printed value comes from a datasource and a field.
 * Text, number, and datetime still print from the current row when
 * datasource is omitted, so an older file keeps working.
 */
export const QUERY_VALUE_TYPES = Object.freeze([20, 21, 22, 23, 24, 26, 45]);

/** These refuse export until both datasource and field are set. */
export const SOURCED_VALUE_TYPES = Object.freeze([22, 23, 26, 45]);

const CAPTIONS = Object.freeze({
  20: 'Field text', 21: 'Field number', 22: 'Field image', 23: 'Field memo', 24: 'Field datetime', 26: 'Field shape',
});

/**
 * @param {unknown} typeKey
 * @returns {boolean}
 */
export function isFieldType(typeKey) {
  return FIELD_TYPES.includes(typeKey);
}

/**
 * Parse sample JSON. Throws with an operator-facing message.
 * @param {string|unknown[]} input JSON text or a parsed array.
 * @returns {{ rows: object[], columns: string[] }}
 */
export function parseSample(input) {
  let value = input;
  if (typeof input === 'string') {
    try {
      value = JSON.parse(input);
    } catch {
      throw new Error('Sample data is not valid JSON.');
    }
  }
  if (!Array.isArray(value)) throw new Error('Sample data must be an array of objects.');
  const columns = [];
  const seen = new Set();
  value.forEach((row, index) => {
    if (!row || typeof row !== 'object' || Array.isArray(row)) {
      throw new Error(`Sample row ${index + 1} is not an object.`);
    }
    for (const key of Object.keys(row)) {
      if (!seen.has(key)) {
        seen.add(key);
        columns.push(key);
      }
    }
  });
  return { rows: value, columns };
}

/**
 * Datasource names from the definition's `datasources` array.
 * @param {object} doc
 * @returns {string[]}
 */
export function datasourceNames(doc) {
  const list = Array.isArray(doc && doc.datasources) ? doc.datasources : [];
  return list
    .map((entry) => (typeof entry === 'string' ? entry : entry && entry.name))
    .filter((name) => typeof name === 'string' && name !== '');
}

/** System Date (5), System User (6), lookup 053 (Phase 22); Page Number (61), Page Count (62), Page Label (64) (Phase 23). */
export const SYSTEM_TYPES = Object.freeze([5, 6, 61, 62, 64]);
/** Page Label (053 key 64): template with {n} page number and {c} page count. */
export const PAGE_LABEL_TYPE = 64;
export const PAGE_LABEL_DEFAULT = 'Page {n} of {c}';
const SYSTEM_LABELS = Object.freeze({ 5: '[System Date]', 6: '[System User]', 61: '[Page #]', 62: '[Page Count]' });
export function systemItemLabel(item) {
  if (item && item.typeKey === PAGE_LABEL_TYPE) {
    const t = typeof item.template === 'string' && item.template !== '' ? item.template : PAGE_LABEL_DEFAULT;
    return `[${t}]`;
  }
  return SYSTEM_LABELS[item && item.typeKey] || 'System';
}

/** System Parameter item: lookup 053 key 63 (Phase 28.3). */
export const PARAM_TYPE = 63;

/** Canvas label for a Parameter item: its param name, else "Parameter". */
export function paramItemLabel(item) {
  return item && typeof item.param === 'string' && item.param !== '' ? `[${item.param}]` : 'Parameter';
}

/** Parameter names across datasources, "ds.NAME" when two share a name. */
export function parameterNames(doc) {
  const list = Array.isArray(doc && doc.datasources) ? doc.datasources : [];
  const seen = new Map();
  for (const ds of list) {
    for (const name of Object.keys((ds && ds.params) || {})) seen.set(name, (seen.get(name) || 0) + 1);
  }
  const out = [];
  for (const ds of list) {
    for (const name of Object.keys((ds && ds.params) || {})) {
      const label = seen.get(name) > 1 ? `${ds.name}.${name}` : name;
      if (!out.includes(label)) out.push(label);
    }
  }
  return out;
}

/**
 * Canvas label: the bound field name, or the type caption when unbound.
 * @param {object} item
 * @returns {string}
 */
export function fieldItemLabel(item) {
  if (item && typeof item.field === 'string' && item.field !== '') return item.field;
  return CAPTIONS[item && item.typeKey] || 'Field';
}

/**
 * Field items with no binding. Export is refused while any remain.
 * @param {object} doc
 * @returns {{ bandId: string, itemId: string, typeKey: number }[]}
 */
export function unboundFields(doc) {
  const out = [];
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  for (const band of bands) {
    const items = Array.isArray(band && band.items) ? band.items : [];
    for (const item of items) {
      if (!item) continue;
      if (SOURCED_VALUE_TYPES.includes(item.typeKey)) {
        const fieldOk = typeof item.field === 'string' && item.field.trim() !== '';
        const sourceOk = typeof item.datasource === 'string' && item.datasource.trim() !== '';
        if (fieldOk && sourceOk) continue;
        out.push({ bandId: band.id, itemId: item.id, typeKey: item.typeKey });
        continue;
      }
      if (!isFieldType(item.typeKey)) continue;
      if (typeof item.field === 'string' && item.field.trim() !== '') continue;
      out.push({ bandId: band.id, itemId: item.id, typeKey: item.typeKey });
    }
  }
  return out;
}

/*
 * Phase 19b: datasources live on the Report. A `json` datasource keeps
 * its rows in the definition (`rows`), so they save, revision, and
 * export. The first datasource drives iteration; bands with no
 * `datasource` use it. `kindKey` is a lookup 076 key; `kind` keeps the
 * lookup code ("json", "query", "csv") for Render and readers.
 */

/** Lookup 076 id (Report Datasource Kinds). */
export const DS_KIND_LOOKUP = 76;

/** Client fallback matching lookup 076 until the migration is applied. */
export const DS_KINDS_FALLBACK = Object.freeze([
  Object.freeze({ key: 1, label: 'JSON', code: 'json' }),
  Object.freeze({ key: 2, label: 'Query', code: 'query' }),
  Object.freeze({ key: 3, label: 'CSV', code: 'csv' }),
]);

/** JSON (19b) and Query (Phase 28) are live; CSV waits for an importer. */
export const LIVE_DS_CODES = Object.freeze(['json', 'query']);

const DS_NAME = /^[A-Za-z_][A-Za-z0-9_]*$/;

/**
 * Kinds from lookup 076 rows, or the fallback list when none loaded.
 * @param {object[]} [rows]
 * @returns {{ key: number, label: string, code: string }[]}
 */
export function datasourceKinds(rows) {
  if (!Array.isArray(rows) || !rows.length) return DS_KINDS_FALLBACK.map((k) => ({ ...k }));
  return rows.map((row) => ({
    key: Number(row.key_idx),
    label: String(row.value_txt),
    code: String(row.code || row.value_txt || '').toLowerCase(),
  }));
}

/** @returns {object[]} datasource entries (objects only). */
export function datasourceList(doc) {
  const list = Array.isArray(doc && doc.datasources) ? doc.datasources : [];
  return list.filter((entry) => entry && typeof entry === 'object');
}

/** @returns {object|null} */
export function findDatasource(doc, name) {
  return datasourceList(doc).find((entry) => entry.name === name) || null;
}

/** Iterating datasource for a band: its own, else the first. */
export function bandDatasource(doc, band) {
  if (band && typeof band.datasource === 'string' && band.datasource !== '') return band.datasource;
  return datasourceNames(doc)[0] || '';
}

/** Stored rows of a json datasource, or []. */
export function datasourceRows(doc, name) {
  const ds = findDatasource(doc, name);
  return ds && Array.isArray(ds.rows) ? ds.rows : [];
}

/**
 * Put Datasource (8) before Field (7). Inserts either key when the
 * lookup list does not have it yet.
 * @param {number[]} keys
 * @returns {number[]}
 */
export function orderBindingKeys(keys) {
  const next = Array.isArray(keys) ? keys.slice() : [];
  for (const key of [8, 7]) {
    if (next.includes(key)) continue;
    const at = next.indexOf(14);
    if (at >= 0) next.splice(at, 0, key);
    else next.push(key);
  }
  const source = next.indexOf(8);
  const field = next.indexOf(7);
  if (source > field && field >= 0) {
    next.splice(source, 1);
    next.splice(field, 0, 8);
  }
  return next;
}

/**
 * First sample row of the item's datasource. Empty when the
 * datasource or the field is missing, or the cell is blank.
 * @param {object} doc
 * @param {object} item
 * @returns {string}
 */
export function boundFieldText(doc, item) {
  const name = item && typeof item.datasource === 'string' ? item.datasource.trim() : '';
  const field = item && typeof item.field === 'string' ? item.field.trim() : '';
  if (name === '' || field === '') return '';
  const rows = datasourceRows(doc, name);
  const row = rows.find((entry) => entry && typeof entry === 'object' && !Array.isArray(entry));
  if (!row || row[field] == null || row[field] === '') return '';
  return String(row[field]);
}

/** Union of row keys, in first-seen order; then query metadata fields. */
export function datasourceColumns(doc, name) {
  const out = [];
  const seen = new Set();
  const ds = findDatasource(doc, name);
  const meta = ds && Array.isArray(ds.fields) ? ds.fields.filter((f) => f && typeof f.name === 'string').map((f) => ({ [f.name]: null })) : [];
  for (const row of [...datasourceRows(doc, name), ...meta]) {
    if (!row || typeof row !== 'object') continue;
    for (const key of Object.keys(row)) {
      if (!seen.has(key)) {
        seen.add(key);
        out.push(key);
      }
    }
  }
  return out;
}

const clone = (value) => JSON.parse(JSON.stringify(value));

function uniqueName(doc) {
  const names = new Set(datasourceNames(doc));
  let n = names.size + 1;
  while (names.has(`data${n}`)) n += 1;
  return `data${n}`;
}

/** New doc with one more `json` datasource. */
export function addDatasource(doc, name) {
  const next = clone(doc);
  if (!Array.isArray(next.datasources)) next.datasources = [];
  const chosen = name || uniqueName(next);
  checkDatasourceName(next, chosen, -1);
  next.datasources.push({ name: chosen, kind: 'json', kindKey: 1, shape: 'list', rows: [] });
  return next;
}

/** New doc without datasource `index`. Bands naming it lose the name. */
export function removeDatasource(doc, index) {
  const next = clone(doc);
  const list = Array.isArray(next.datasources) ? next.datasources : [];
  if (index < 0 || index >= list.length) throw new Error(`Datasource ${index + 1} does not exist.`);
  const [gone] = list.splice(index, 1);
  for (const band of Array.isArray(next.bands) ? next.bands : []) {
    if (gone && band && band.datasource === gone.name) delete band.datasource;
  }
  return next;
}

/** Throws unless `name` is one word and unique among the others. */
export function checkDatasourceName(doc, name, index) {
  if (!DS_NAME.test(String(name))) throw new Error(`Datasource name "${name}" must be one word (letters, digits, _).`);
  const clash = datasourceList(doc).some((entry, at) => at !== index && entry.name === name);
  if (clash) throw new Error(`Datasource name "${name}" is already used.`);
}

/** Rename datasource `index`; bands and items naming it follow. */
export function renameDatasource(doc, index, name) {
  const value = String(name ?? '').trim();
  checkDatasourceName(doc, value, index);
  const next = clone(doc);
  const ds = next.datasources[index];
  const old = ds.name;
  ds.name = value;
  for (const band of Array.isArray(next.bands) ? next.bands : []) {
    if (band.datasource === old) band.datasource = value;
    for (const item of Array.isArray(band.items) ? band.items : []) {
      if (item && item.datasource === old) item.datasource = value;
    }
  }
  return next;
}

/** Set kind of datasource `index`. Only live kinds are accepted. */
export function setDatasourceKind(doc, index, value, kinds = DS_KINDS_FALLBACK) {
  const kind = kinds.find((k) => k.label === value || k.code === value || k.key === Number(value));
  if (!kind) throw new Error(`Datasource kind "${value}" is not known.`);
  if (!LIVE_DS_CODES.includes(kind.code)) {
    throw new Error(`Datasource kind ${kind.label} is not available yet${kind.code === 'query' ? ' (Phase 28)' : ''}.`);
  }
  const next = clone(doc);
  const ds = next.datasources[index];
  Object.assign(ds, { kind: kind.code, kindKey: kind.key });
  if (kind.code === 'query') {
    if (!Object.hasOwn(ds, 'queryRef')) ds.queryRef = null;
    if (!ds.params || typeof ds.params !== 'object') ds.params = {};
  }
  return next;
}

/**
 * New doc with `rows` stored on datasource `name`. With no name, the
 * first json datasource; with none at all, a new `data1` is added.
 * @returns {{ doc: object, name: string }}
 */
export function setDatasourceRows(doc, name, rows) {
  let next = clone(doc);
  let target = name ? findDatasource(next, name) : datasourceList(next).find((d) => (d.kind || 'json') === 'json');
  if (!target) {
    if (name) throw new Error(`Datasource "${name}" does not exist.`);
    next = addDatasource(next);
    target = datasourceList(next)[datasourceList(next).length - 1];
  }
  if (!['json', 'query'].includes(target.kind || 'json')) {
    throw new Error(`Datasource "${target.name}" is ${target.kind}; rows go on a JSON or Query datasource.`);
  }
  target.rows = clone(rows);
  return { doc: next, name: target.name };
}
