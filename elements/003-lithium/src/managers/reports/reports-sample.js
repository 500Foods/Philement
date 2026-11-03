/**
 * Phase 15: field items and the session-only sample datasource.
 * The sample is an array of objects the operator pastes or imports.
 * It lives on the manager for the session and never enters the
 * definition export. Column names are the union of its keys.
 */

/** Field item types: lookup 053 keys 20 text, 21 number, 24 datetime. */
export const FIELD_TYPES = Object.freeze([20, 21, 24]);

const CAPTIONS = Object.freeze({ 20: 'Field text', 21: 'Field number', 24: 'Field datetime' });

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
      if (!item || !isFieldType(item.typeKey)) continue;
      if (typeof item.field === 'string' && item.field.trim() !== '') continue;
      out.push({ bandId: band.id, itemId: item.id, typeKey: item.typeKey });
    }
  }
  return out;
}
