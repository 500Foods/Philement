/**
 * Report page fields that already exist.
 * `name` and `title` are on the definition. `summary` is a column
 * on the reports table. There is no group column and no group key.
 */

/**
 * Name and title stored on a definition. A missing string is blank.
 * @param {object|null} doc
 * @returns {{ name: string, title: string }}
 */
export function definitionMeta(doc) {
  return {
    name: doc && typeof doc.name === 'string' ? doc.name : '',
    title: doc && typeof doc.title === 'string' ? doc.title : '',
  };
}

/**
 * Copy of `doc` with name and title replaced when those keys are present.
 * @param {object} doc
 * @param {{ name?: string, title?: string }} fields
 * @returns {object}
 */
export function withDefinitionMeta(doc, fields) {
  const next = JSON.parse(JSON.stringify(doc));
  if (Object.hasOwn(fields, 'name')) next.name = String(fields.name);
  if (Object.hasOwn(fields, 'title')) next.title = String(fields.title);
  return next;
}

/**
 * Summary text on a list row. The definition does not store it.
 * @param {object|null} row
 * @returns {string}
 */
export function rowSummary(row) {
  if (!row || typeof row.summary !== 'string') return '';
  return row.summary;
}
