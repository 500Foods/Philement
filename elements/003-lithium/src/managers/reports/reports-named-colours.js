/**
 * Named colours lookup 080 (Phase 77). CSS + xkcd names resolve to hex.
 * Stored property values stay #rrggbb; names are entry helpers only.
 */

/** #rgb / #rrggbb → lowercase #rrggbb; '' when not hex. */
function hexColor(value) {
  const text = String(value ?? '').trim();
  const short = /^#([0-9a-f]{3})$/i.exec(text);
  if (short) {
    const [r, g, b] = short[1];
    return `#${r}${r}${g}${g}${b}${b}`.toLowerCase();
  }
  const long = /^#([0-9a-f]{6})([0-9a-f]{2})?$/i.exec(text);
  return long ? `#${long[1].toLowerCase()}` : '';
}

/** @typedef {{ name: string, hex: string, swatch: boolean, key: number }} NamedColour */

/** @type {NamedColour[]} */
let entries = [];

/**
 * Parse QueryRef 26 rows for lookup 080.
 * value_txt = name, code = #rrggbb, value_int = swatch flag.
 * @param {object[]} rows
 * @returns {NamedColour[]}
 */
export function parseNamedColourRows(rows) {
  const list = Array.isArray(rows) ? rows : [];
  const out = [];
  for (const row of list) {
    const key = Number(row && row.key_idx);
    if (!Number.isFinite(key) || key <= 0) continue;
    const name = typeof row.value_txt === 'string' ? row.value_txt.trim() : '';
    const hex = hexColor(row && row.code);
    if (!name || !hex) continue;
    const swatch = Number(row.value_int) === 1;
    out.push({ name, hex, swatch, key });
  }
  out.sort((a, b) => a.name.localeCompare(b.name, 'en'));
  return out;
}

/**
 * Merge CSS-first then xkcd-style lists; CSS wins on duplicate names.
 * Used by unit tests and offline seed checks.
 * @param {{ name: string, hex: string }[]} css
 * @param {{ name: string, hex: string }[]} xkcd
 * @returns {{ name: string, hex: string, source: string }[]}
 */
export function mergeNamedColourSources(css, xkcd) {
  const byName = new Map();
  for (const row of Array.isArray(css) ? css : []) {
    const name = String(row.name || '').trim();
    const hex = hexColor(row.hex);
    if (!name || !hex) continue;
    byName.set(name.toLowerCase(), { name, hex, source: 'css' });
  }
  for (const row of Array.isArray(xkcd) ? xkcd : []) {
    const name = String(row.name || '').trim();
    const hex = hexColor(row.hex);
    if (!name || !hex) continue;
    const key = name.toLowerCase();
    if (byName.has(key)) continue;
    byName.set(key, { name, hex, source: 'xkcd' });
  }
  return [...byName.values()].sort((a, b) => a.name.localeCompare(b.name, 'en'));
}

/** @param {object[]} rows */
export function setNamedColoursFromLookup(rows) {
  entries = parseNamedColourRows(rows);
  return entries;
}

/** @returns {NamedColour[]} */
export function namedColourEntries() {
  return entries;
}

/** Entries flagged for the Coloris swatch row. */
export function namedSwatchColours() {
  return entries.filter((entry) => entry.swatch).map((entry) => entry.hex);
}

/** Named swatches with titles for hover. @returns {{ hex: string, name: string }[]} */
export function namedSwatchEntries() {
  return entries.filter((entry) => entry.swatch).map((entry) => ({ hex: entry.hex, name: entry.name }));
}

/**
 * Resolve typed text to hex: exact #hex, or case-insensitive name match.
 * @param {unknown} value
 * @returns {string} lowercase #rrggbb or '' when unresolved
 */
export function resolveNamedColour(value) {
  const text = String(value ?? '').trim();
  if (!text) return '';
  const asHex = hexColor(text);
  if (asHex) return asHex;
  const key = text.toLowerCase();
  const hit = entries.find((entry) => entry.name.toLowerCase() === key);
  return hit ? hit.hex : '';
}

/**
 * Filter names by substring (case-insensitive). Empty query → all.
 * @param {string} query
 * @param {number} [limit=50]
 * @returns {NamedColour[]}
 */
export function filterNamedColours(query, limit = 50) {
  const q = String(query ?? '').trim().toLowerCase();
  const max = Number.isFinite(limit) ? Math.max(1, limit) : 50;
  const src = entries;
  if (!q) return src.slice(0, max);
  const out = [];
  for (const entry of src) {
    if (entry.name.toLowerCase().includes(q) || entry.hex.includes(q)) {
      out.push(entry);
      if (out.length >= max) break;
    }
  }
  return out;
}
