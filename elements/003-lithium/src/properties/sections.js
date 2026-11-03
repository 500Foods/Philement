/**
 * LithiumProperties section grouping and label resolver.
 *
 * Pure helpers for the shared property panel (Reporting Phase 14).
 * They take rows and sections as data and know nothing about reports.
 *
 * - A row's section is its numeric `section` key. A row without one
 *   falls back to the section whose label equals its `Group` string.
 * - Sections are ordered by their own `sortSeq`; rows keep their
 *   `sortSeq` order inside a section (ties by id).
 * - Labels resolve from `lang`: exact locale, then language, then the
 *   plain label. The caller passes the profile language first.
 *
 * @module properties/sections
 */

/**
 * @typedef {{ id: number, label: string, sortSeq?: number, collection?: object }} PropertyRow
 * @typedef {{ id: number, label: string, sortSeq?: number, icon?: string, collection?: object }} PropertySection
 */

const bySortThenId = (a, b) => (a.sortSeq ?? 0) - (b.sortSeq ?? 0) || a.id - b.id;

/**
 * Section id for one row, or null when neither `section` nor `Group` matches.
 * @param {PropertyRow} row
 * @param {Map<string, number>} idByLabel
 * @returns {number|null}
 */
export function rowSectionId(row, idByLabel) {
  const c = row && row.collection ? row.collection : {};
  if (Number.isInteger(c.section)) return c.section;
  if (typeof c.Group === 'string' && idByLabel.has(c.Group)) return idByLabel.get(c.Group);
  return null;
}

/**
 * Group rows into ordered sections. Rows with no known section land in
 * a trailing group with id null so nothing silently disappears.
 * @param {PropertyRow[]} rows
 * @param {PropertySection[]} sections
 * @returns {Array<{ section: PropertySection|null, rows: PropertyRow[] }>}
 */
export function groupBySection(rows, sections) {
  const ordered = [...(sections || [])].sort(bySortThenId);
  const idByLabel = new Map(ordered.map((s) => [s.label, s.id]));
  const buckets = new Map(ordered.map((s) => [s.id, []]));
  const orphans = [];
  for (const row of rows || []) {
    const id = rowSectionId(row, idByLabel);
    if (id !== null && buckets.has(id)) buckets.get(id).push(row);
    else orphans.push(row);
  }
  const out = ordered
    .map((section) => ({ section, rows: buckets.get(section.id).sort(bySortThenId) }))
    .filter((g) => g.rows.length > 0);
  if (orphans.length) out.push({ section: null, rows: orphans.sort(bySortThenId) });
  return out;
}

/**
 * Display label for a row. `lang` maps locale tags to a string or to
 * `{ Label, List }`. Tries each locale exactly, then its language part.
 * @param {{ label: string, collection?: object }} row
 * @param {string[]} locales - profile language first, then browser
 * @returns {string}
 */
export function resolveLabel(row, locales = []) {
  const c = (row && row.collection) || {};
  const plain = typeof c.Label === 'string' ? c.Label : (row && row.label) || '';
  const entry = langEntry(c.lang, locales);
  if (typeof entry === 'string') return entry;
  if (entry && typeof entry.Label === 'string') return entry.Label;
  return plain;
}

/**
 * Captions for a row's stored `List`, matched by position under `lang`.
 * Stored values are never changed; a missing caption shows the value.
 * @param {{ collection?: object }} row
 * @param {string[]} locales
 * @returns {string[]}
 */
export function resolveListCaptions(row, locales = []) {
  const c = (row && row.collection) || {};
  const values = Array.isArray(c.List) ? c.List : [];
  const entry = langEntry(c.lang, locales);
  const captions = entry && Array.isArray(entry.List) ? entry.List : [];
  return values.map((v, i) => (typeof captions[i] === 'string' ? captions[i] : String(v)));
}

function langEntry(lang, locales) {
  if (!lang || typeof lang !== 'object') return null;
  for (const loc of locales) if (loc && Object.hasOwn(lang, loc)) return lang[loc];
  for (const loc of locales) {
    const base = loc ? loc.split('-')[0] : '';
    if (base && Object.hasOwn(lang, base)) return lang[base];
  }
  return null;
}
